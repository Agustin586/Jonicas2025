#include "web.h"
#include "config.h"
#include "RaceMef.h"
#include "RaceEvent.h"

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <ArduinoJson.h>

const char* ssid = "Valentina";
const char* password = "43732258";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Variables globales para el estado de la carrera
volatile bool webStartRaceRequested = false;
volatile bool webResetRaceRequested = false;
volatile uint32_t currentRaceTime = 0;
String currentWebState = "WAITING"; // Para sincronizar con la interfaz web

// Cola para comunicarse con la MEF
extern QueueHandle_t raceQ;

unsigned long lastStatusUpdate = 0;
const unsigned long STATUS_UPDATE_INTERVAL = 100; // ms

void initLittleFS() {
    if (!LittleFS.begin(true)) {
        Serial.println("An error has occurred while mounting LittleFS");
    }
    Serial.println("LittleFS mounted successfully");
    
    // Debug: Listar archivos disponibles
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    Serial.println("Archivos en LittleFS:");
    while(file) {
        Serial.print("  ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
}

void initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }
    Serial.println(WiFi.localIP());
}

void notifyClients(String message) {
    ws.textAll(message);
}

String createStatusMessage() {
    StaticJsonDocument<200> doc;
    doc["raceState"] = currentWebState;
    
    if (currentWebState == "RUNNING" && currentRaceTime > 0) {
        doc["raceTime"] = currentRaceTime;
    }
    
    String message;
    serializeJson(doc, message);
    return message;
}

void webFalseStart(void) {
    Serial.println("[WEB] Largada falsa detectada");
    currentWebState = "FALSE_START";
    currentRaceTime = 0;

    StaticJsonDocument<200> doc;
    doc["raceState"] = currentWebState;
    String message;
    serializeJson(doc, message);
    notifyClients(message);
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char*)data;
        
        Serial.printf("[WEB] Mensaje recibido: %s\n", message.c_str());
        
        if (message == "getStatus") {
            // Enviar estado actual
            String statusMessage = createStatusMessage();
            notifyClients(statusMessage);
            
        } else if (message == "startRace") {
            Serial.println("[WEB] Comando: Iniciar carrera");
            
            // Marcar bandera para que RaceTask la detecte
            webStartRaceRequested = true;
            
        } else if (message == "resetRace") {
            Serial.println("[WEB] Comando: Reiniciar sistema");
            
            // Marcar bandera para que RaceTask la detecte
            webResetRaceRequested = true;
        }
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT: {
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            // Enviar estado actual al cliente que se conecta
            String statusMessage = createStatusMessage();
            client->text(statusMessage);
            break;
        }
            
        case WS_EVT_DISCONNECT: {
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        }
            
        case WS_EVT_DATA: {
            handleWebSocketMessage(arg, data, len);
            break;
        }
            
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

void webTask(void *pv) {
    // Inicializaciones...
    initWiFi();
    initLittleFS();
    initWebSocket();

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Intenta servir desde LittleFS primero
        if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            // Fallback: HTML embebido (temporal)
            Serial.println("Sirviendo HTML embebido - archivos no encontrados en LittleFS");
            
            
        }
    });

    server.serveStatic("/", LittleFS, "/");

    // Start server
    server.begin();
    
    Serial.println("Servidor web iniciado");

    for(;;) {
        // Enviar actualizaciones periódicas del cronómetro si la carrera está en curso
        unsigned long currentTime = millis();
        if (currentWebState == "RUNNING" && 
            (currentTime - lastStatusUpdate) >= STATUS_UPDATE_INTERVAL) {
            
            String statusMessage = createStatusMessage();
            notifyClients(statusMessage);
            lastStatusUpdate = currentTime;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // 50ms delay para actualizaciones fluidas
    }
}

// ===============================================
// FUNCIONES DE INTERFAZ PARA COMUNICACIÓN CON LA MEF
// ===============================================

bool webRaceStart(void) {
    // Esta función es llamada desde RaceTask para verificar si se solicitó inicio
    if (webStartRaceRequested) {
        webStartRaceRequested = false; // Reset de la bandera
        Serial.println("[WEB] Confirmando inicio de carrera");
        return true;
    }
    return false;
}

bool webRaceReset(void) {
    // Esta función es llamada desde RaceTask para verificar si se solicitó reset
    if (webResetRaceRequested) {
        webResetRaceRequested = false; // Reset de la bandera
        Serial.println("[WEB] Confirmando reset de carrera");
        return true;
    }
    return false;
}

void webTempUpdate(uint32_t elapsedMs) {
    currentRaceTime = elapsedMs;
    StaticJsonDocument<200> doc;
    doc["raceState"] = currentWebState;
    doc["raceTime"] = elapsedMs;  // en ms
    String message;
    serializeJson(doc, message);
    notifyClients(message);
}

void webRaceInit(void) {
    // Esta función es llamada cuando la carrera realmente inicia (estado RUNNING)
    Serial.println("[WEB] Carrera iniciada - cambiando a estado RUNNING");
    
    currentWebState = "RUNNING";
    currentRaceTime = 0;
    
    String statusMessage = createStatusMessage();
    notifyClients(statusMessage);
}

void webRaceFinish(void) {
    // Esta función es llamada cuando la carrera termina (estado FINISH)
    Serial.println("[WEB] Carrera finalizada - cambiando a estado FINISHED");
    
    currentWebState = "FINISHED";
    
    String statusMessage = createStatusMessage();
    notifyClients(statusMessage);
}

void webReset(void) {
    // Esta función es llamada cuando se resetea el sistema (estado RESET -> IDLE)
    Serial.println("[WEB] Sistema reseteado - cambiando a estado WAITING");
    
    currentWebState = "WAITING";
    currentRaceTime = 0;
    
    String statusMessage = createStatusMessage();
    notifyClients(statusMessage);
}

// ===============================================
// FUNCIONES ADICIONALES PARA INTEGRACIÓN
// ===============================================

void webSemaphoreState(uint32_t step) {
    StaticJsonDocument<200> doc;
    doc["raceState"] = "SEMAPHORE";

    switch (step) {
        case 0: doc["light"] = "RED"; break;
        case 1: doc["light"] = "YELLOW"; break;
        case 2: doc["light"] = "GREEN"; break;
    }

    String message;
    serializeJson(doc, message);
    notifyClients(message);
}


void webUpdateRaceState(const char* state) {
    // Función para sincronizar el estado web con la MEF
    String newState = String(state);
    if (currentWebState != newState) {
        currentWebState = newState;
        Serial.printf("[WEB] Actualizando estado web a: %s\n", state);
        
        String statusMessage = createStatusMessage();
        notifyClients(statusMessage);
    }
}