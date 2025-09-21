#include "config.h"
#include "RaceMef.h"
#include "web.h"

// ===============================================
// DEFINES
// ===============================================
#define SEMAPHORE_RED_LIGHT 0
#define SEMAPHORE_YELLOW_LIGHT 1
#define SEMAPHORE_GREEN_LIGHT 2
#define SEMAPHORE_PERIODE_1S 1000
#define SEMAPHORE_PERIODE_2S 2000
#define SEMAPHORE_PERIODE_3S 3000

// ===============================================
// DEFINICIONES DE VARIABLES
// ===============================================
static RaceState sState = RaceState::EST_RACE_RESET; // Estados de la MEF
static uint32_t sSemStep = 0;                        // Contador de secuencia de semáforo
static const uint32_t kSemSteps = 3;                 // Cantidad de pasos del semáforo (ajustable)
static uint32_t raceStartTick = 0;                   // Cuenta el inicio de la carrera
static uint32_t raceEndTick = 0;                     // Cuenta el fin de la carrera
static bool endSemaphoreSeq = false;

TimerHandle_t xSemaphoreTimer = nullptr;
TimerHandle_t xWebTimer = nullptr;
QueueHandle_t raceQ = nullptr;
// ===============================================
// DECLARACIONES DE FUNCIONES
// ===============================================
static void gotoState(RaceState ns) { sState = ns; } // Cambia el estado de la MEF
static void timerInit(void);
static void timerReset(void);
static void semaphoreTimerCallback(TimerHandle_t xTimer);
static void semaphoreSequence(uint32_t n);
static void semaphoreReset(void);
static void raceReset(void);
static void raceRunning(void);
static void webTimerCallback(TimerHandle_t xTimer);

// ===============================================
// DECLARACIONES DE FUNCIONES PARA TEST
// ===============================================
static void testSimularCruceMeta(void);
static void testSemaphoreSequence(void);

// ==============================================
// VARIABLES PARA TEST
// ==============================================


// ===============================================
// DEFINICIONES DE FUNCIONES
// ===============================================
static void onEntry(RaceState st)
{
    switch (st)
    {
    case RaceState::EST_RACE_RESET:
        // Debug...
        Serial.println("[RESET] onEntry");

        // Acciones...
        semaphoreReset();
        timerReset();
        raceReset();
        gotoState(RaceState::EST_RACE_IDLE);

        break;

    case RaceState::EST_RACE_IDLE:
        // Debug...
        Serial.println("[IDLE] onEntry");

        break;

    case RaceState::EST_RACE_SEMAPHORE:
        // Debug...
        Serial.println("[SEMAPHORE] onEntry");

        // Acciones...
        semaphoreSequence(sSemStep);     // Inicia la secuencia de semáforo
        xTimerStart(xSemaphoreTimer, 0); // webUpdateTemp()

        break;

    case RaceState::EST_RACE_RUNNING:
        // Debug...
        Serial.println("[RUNNING] onEntry");

        // Acciones...
        raceRunning();
        xTimerStart(xWebTimer, 0);

        break;

    case RaceState::EST_RACE_FINISH:
        // Debug...
        Serial.println("[FINISH] onEntry");

        // Acciones...
        gotoState(RaceState::EST_RACE_IDLE);

        break;
    }
}

static void handleEvent(const RaceEvent &ev)
{
    switch (sState)
    {
    // ESTADO DE ESPERRA
    case RaceState::EST_RACE_IDLE:
        if (ev.type == RaceEventType::EV_RACE_START)
        {
            gotoState(RaceState::EST_RACE_SEMAPHORE); // Transición a SEMÁFORO
            onEntry(RaceState::EST_RACE_SEMAPHORE);   // onEntry de SEMÁFORO
        }
        else if (ev.type == RaceEventType::EV_RACE_RESET_REQ)
        {
            gotoState(RaceState::EST_RACE_RESET);
            onEntry(RaceState::EST_RACE_RESET);
        }

        break;

    // ESTADO DE SEMAFORO
    case RaceState::EST_RACE_SEMAPHORE:
        if (ev.type == RaceEventType::EV_RACE_SEMAPHORE_REQ)
        {
            gotoState(RaceState::EST_RACE_RUNNING);
            onEntry(RaceState::EST_RACE_RUNNING);
        }
        // if (ev.type == RaceEventType::EV_RACE_FINISH_REQ)
        // {
        //     gotoState(RaceState::EST_RACE_FINISH);
        //     onEntry(RaceState::EST_RACE_FINISH);
        // }
        // else if (ev.type == RaceEventType::EV_RACE_RESET_REQ)
        // {
        //     gotoState(RaceState::EST_RACE_RESET);
        //     onEntry(RaceState::EST_RACE_RESET);
        // }

        break;

    // ESTADO DE RUNNING
    case RaceState::EST_RACE_RUNNING:
        if (ev.type == RaceEventType::EV_RACE_FINISH_REQ)
        {
            gotoState(RaceState::EST_RACE_FINISH);
            onEntry(RaceState::EST_RACE_FINISH);
        }
        else if (ev.type == RaceEventType::EV_RACE_RESET_REQ)
        {
            // si preferís reset duro desde running
            gotoState(RaceState::EST_RACE_RESET);
            onEntry(RaceState::EST_RACE_RESET);
        }
        break;

    // ESTADO DE FINISH
    case RaceState::EST_RACE_FINISH:
        // entry ya nos llevó a IDLE
        break;

    // ESTADO DE RESET
    case RaceState::EST_RACE_RESET:
        // entry ya nos llevó a IDLE
        break;
    }
}

void RaceTask(void *pv)
{
    raceQ = (QueueHandle_t)pv;
    sState = RaceState::EST_RACE_RESET;
    onEntry(RaceState::EST_RACE_RESET);

    const TickType_t period = pdMS_TO_TICKS(100); // 10 Hz
    RaceEvent ev;

    // Inicializaciones
    timerInit();

    for (;;)
    {
        // --- EVENTOS RECIBIDOS DESDE LA COLA DE DATOS ---
        if (xQueueReceive(raceQ, &ev, period) == pdTRUE)
            handleEvent(ev);

        // --- DETECTA INICIO DESDE VIA WEB ---
        if (webRaceStart() && sState == RaceState::EST_RACE_IDLE)
        {
            RaceEvent evToSend = {
                .type = RaceEventType::EV_RACE_START,
                .arg = 0
            };

            xQueueSendToBack(raceQ, &evToSend, portMAX_DELAY);
        }

        // --- TESTS ---
        // --- SIMULAR CRUCE DE META ---
        // testSimularCruceMeta();

        // --- SIMULAR SECUENCIA DE SEMAFORO ---
        // testSemaphoreSequence();
    }
}

static void timerInit(void)
{
    xSemaphoreTimer = xTimerCreate(
        "SemTimer",
        pdMS_TO_TICKS(SEMAPHORE_PERIODE_1S),
        pdFALSE,
        NULL,
        semaphoreTimerCallback);

    xWebTimer = xTimerCreate(
        "WebTimer",
        pdMS_TO_TICKS(WEB_TIMER_UPDATE_PERIOD),
        pdTRUE,
        NULL,
        webTimerCallback);
}

static void timerReset(void)
{
    xTimerStop(xSemaphoreTimer, portMAX_DELAY);
    xTimerStop(xWebTimer, portMAX_DELAY);
}

static void semaphoreTimerCallback(TimerHandle_t xTimer)
{
    // --- Avanzar un paso en la secuencia de semáforo ---
    sSemStep++;

    // --- Realiza la secuencia del semaforo con leds ---
    semaphoreSequence(sSemStep);

    // --- Verificar si se completó la secuencia ---
    if (sSemStep >= kSemSteps)
        endSemaphoreSeq = true;
}

static void semaphoreSequence(uint32_t n)
{
    // Debug...
    Serial.printf("[SEMAPHORE] step=%lu\n", (unsigned long)n);

    // Acciones...
    switch (n)
    {
    case 0:
        sSemStep = 0;
        endSemaphoreSeq = false;

        // Encender luz roja
        digitalWrite(SEMAPHORE_RED_LIGHT, HIGH);
        digitalWrite(SEMAPHORE_YELLOW_LIGHT, LOW);
        digitalWrite(SEMAPHORE_GREEN_LIGHT, LOW);

        break;
    case 1:
        // Encender luz amarilla
        digitalWrite(SEMAPHORE_RED_LIGHT, HIGH);
        digitalWrite(SEMAPHORE_YELLOW_LIGHT, HIGH);
        digitalWrite(SEMAPHORE_GREEN_LIGHT, LOW);

        // Timer para segunda secuencia
        xTimerChangePeriod(xSemaphoreTimer, pdMS_TO_TICKS(SEMAPHORE_PERIODE_3S), 0);
        xTimerStart(xSemaphoreTimer, 0);

        break;
    case 2:
        // Encender luz verde, apagar roja y amarilla
        digitalWrite(SEMAPHORE_RED_LIGHT, LOW);
        digitalWrite(SEMAPHORE_YELLOW_LIGHT, LOW);
        digitalWrite(SEMAPHORE_GREEN_LIGHT, HIGH);

        // Timer para tercera secuencia
        xTimerChangePeriod(xSemaphoreTimer, pdMS_TO_TICKS(SEMAPHORE_PERIODE_2S), 0);
        xTimerStart(xSemaphoreTimer, 0);

        // Transición a RUNNING se maneja en handleEvent al completar secuencia
        RaceEvent evToSend;
        evToSend.type = RaceEventType::EV_RACE_SEMAPHORE_REQ;
        xQueueSendToBack(raceQ, &evToSend, portMAX_DELAY);

        break;
    case 3:
        // Debug...

        // Acciones...
        digitalWrite(SEMAPHORE_RED_LIGHT, LOW);
        digitalWrite(SEMAPHORE_YELLOW_LIGHT, LOW);
        digitalWrite(SEMAPHORE_GREEN_LIGHT, LOW);

        xTimerStop(xSemaphoreTimer, portMAX_DELAY);

        break;
    default:
        // No hacer nada o manejar error
        break;
    }
}

static void semaphoreReset(void)
{
    sSemStep = 0;
    endSemaphoreSeq = false;

    // Configura todas las luces
    pinMode(SEMAPHORE_RED_LIGHT, OUTPUT);
    pinMode(SEMAPHORE_YELLOW_LIGHT, OUTPUT);
    pinMode(SEMAPHORE_GREEN_LIGHT, OUTPUT);

    // Apagar todas las luces
    digitalWrite(SEMAPHORE_RED_LIGHT, LOW);
    digitalWrite(SEMAPHORE_YELLOW_LIGHT, LOW);
    digitalWrite(SEMAPHORE_GREEN_LIGHT, LOW);
}

static void raceReset(void)
{
    raceStartTick = 0;
    raceEndTick = 0;
}

static void raceRunning(void)
{
    // --- TOMA EL VALOR INICIAL DEL TICK ---
    raceStartTick = xTaskGetTickCount();
}

static void webTimerCallback(TimerHandle_t xTimer)
{
    uint32_t raceIntTick = xTaskGetTickCount();
    uint32_t elapsedTicks = raceIntTick - raceStartTick;
    uint32_t elapsedMs = elapsedTicks * portTICK_PERIOD_MS;
    
    webTempUpdate(elapsedMs);
}

// ===============================================
// FUNCIONES PARA TEST
// ===============================================
static void testSimularCruceMeta(void)
{
    // Debug...
    Serial.println("[TEST] Simular cruce de meta");

    // Acciones...
    RaceEvent ev = {
        .type = RaceEventType::EV_RACE_FINISH_REQ,
        .arg = 0
    };

    xQueueSendToBack(raceQ, &ev, portMAX_DELAY);
}

static void testSemaphoreSequence(void)
{
    // Obtiene el valor de los pines de salida
    Serial.println("Estado de los pines del semáforo:");

    Serial.print("Rojo: ");
    Serial.println(digitalRead(SEMAPHORE_RED_LIGHT));

    Serial.print("Amarillo: ");
    Serial.println(digitalRead(SEMAPHORE_YELLOW_LIGHT));

    Serial.print("Verde: ");
    Serial.println(digitalRead(SEMAPHORE_GREEN_LIGHT));
}