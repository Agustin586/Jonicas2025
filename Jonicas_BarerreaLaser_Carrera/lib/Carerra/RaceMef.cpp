#include "config.h"
#include "RaceMef.h"
#include "web.h"
#include "RaceEvent.h"

#include <Arduino.h>

// ===============================================
// DEFINES
// ===============================================
#define SEMAPHORE_RED_LIGHT     2
#define SEMAPHORE_YELLOW_LIGHT  41
#define SEMAPHORE_GREEN_LIGHT   42
#define SEMAPHORE_PERIODE_1S   1000u
#define SEMAPHORE_PERIODE_2S   2000u
#define SEMAPHORE_PERIODE_3S   3000u
#define BARRIER_PIN             4

// ===============================================
// VARIABLES DE MEF
// ===============================================
static RaceState sState = RaceState::EST_RACE_RESET;
static uint32_t sSemStep = 0;
static const uint32_t kSemSteps = 2; // 0=rojo, 1=amarillo, 2=verde
static uint32_t raceStartTick = 0;
static uint32_t raceEndTick = 0;
static bool endSemaphoreSeq = false;
static uint32_t lastBarrierTick = 0; // Para debounce 10s

TimerHandle_t xSemaphoreTimer = nullptr;
TimerHandle_t xWebTimer = nullptr;
TimerHandle_t xBlinkTimer = nullptr;
QueueHandle_t raceQ = nullptr;

SemaphoreHandle_t xBarrierSemaphore = nullptr;

// ===============================================
// DECLARACIONES LOCALES
// ===============================================
static void gotoState(RaceState ns) { sState = ns; }
static void timerInit(void);
static void timerReset(void);
static void semaphoreInit(void);
static void semaphoreTimerCallback(TimerHandle_t xTimer);
static void semaphoreSequence(uint32_t n);
static void semaphoreReset(void);
static void raceReset(void);
static void raceRunning(void);
static void webTimerCallback(TimerHandle_t xTimer);
static void blinkTimerCallback(TimerHandle_t xTimer);
static void blinkInit(void);
static void barrierInit(void);
void IRAM_ATTR barrierISR();
static void testSemaphoreSequence(void);

// ===============================================
// IMPLEMENTACIÓN
// ===============================================
static void onEntry(RaceState st)
{
    switch (st)
    {
    case RaceState::EST_RACE_RESET:
        Serial.println("[RESET] onEntry");
        if (xSemaphoreTimer) xTimerStop(xSemaphoreTimer, portMAX_DELAY);
        if (xWebTimer) xTimerStop(xWebTimer, portMAX_DELAY);
        semaphoreReset();
        timerReset();
        raceReset();
        webReset();
        gotoState(RaceState::EST_RACE_IDLE);
        onEntry(RaceState::EST_RACE_IDLE);
        break;

    case RaceState::EST_RACE_IDLE:
        Serial.println("[IDLE] onEntry");
        webUpdateRaceState("WAITING");
        break;

    case RaceState::EST_RACE_SEMAPHORE:
        Serial.println("[SEMAPHORE] onEntry");
        webUpdateRaceState("SEMAPHORE");
        sSemStep = 0;
        endSemaphoreSeq = false;
        semaphoreSequence(sSemStep);
        if (xSemaphoreTimer) {
            xTimerChangePeriod(xSemaphoreTimer, pdMS_TO_TICKS(SEMAPHORE_PERIODE_1S), 0);
            xTimerStart(xSemaphoreTimer, 0);
        }
        break;

    case RaceState::EST_RACE_RUNNING:
        Serial.println("[RUNNING] onEntry");
        raceRunning();
        if (xWebTimer) xTimerStart(xWebTimer, 0);
        webRaceInit();
        break;

    case RaceState::EST_RACE_FINISH:
        Serial.println("[FINISH] onEntry");
        webRaceFinish();
        if (xWebTimer) xTimerStop(xWebTimer, portMAX_DELAY);
        break;
    }
}

static void handleEvent(const RaceEvent &ev)
{
    switch (sState)
    {
    case RaceState::EST_RACE_IDLE:
        if (ev.type == RaceEventType::EV_RACE_START) {
            gotoState(RaceState::EST_RACE_SEMAPHORE);
            onEntry(RaceState::EST_RACE_SEMAPHORE);
        } else if (ev.type == RaceEventType::EV_RACE_RESET_REQ) {
            gotoState(RaceState::EST_RACE_RESET);
            onEntry(RaceState::EST_RACE_RESET);
        }
        break;

    case RaceState::EST_RACE_SEMAPHORE:
        if (ev.type == RaceEventType::EV_RACE_SEMAPHORE_REQ) {
            gotoState(RaceState::EST_RACE_RUNNING);
            onEntry(RaceState::EST_RACE_RUNNING);
        } else if (ev.type == RaceEventType::EV_RACE_RESET_REQ) {
            Serial.println("[SEMAPHORE] Reset solicitado");
            gotoState(RaceState::EST_RACE_RESET);
            onEntry(RaceState::EST_RACE_RESET);
        }
        break;

    case RaceState::EST_RACE_RUNNING:
        if (ev.type == RaceEventType::EV_RACE_FINISH_REQ) {
            raceEndTick = xTaskGetTickCount();
            uint32_t elapsedTicks = (raceEndTick - raceStartTick);
            uint32_t elapsedMs = elapsedTicks * portTICK_PERIOD_MS;
            webTempUpdate(elapsedMs);
            gotoState(RaceState::EST_RACE_FINISH);
            onEntry(RaceState::EST_RACE_FINISH);
        } else if (ev.type == RaceEventType::EV_RACE_RESET_REQ) {
            Serial.println("[RUNNING] Reset solicitado");
            gotoState(RaceState::EST_RACE_RESET);
            onEntry(RaceState::EST_RACE_RESET);
        }
        break;

    case RaceState::EST_RACE_FINISH:
        if (ev.type == RaceEventType::EV_RACE_RESET_REQ) {
            gotoState(RaceState::EST_RACE_RESET);
            onEntry(RaceState::EST_RACE_RESET);
        }
        break;

    case RaceState::EST_RACE_RESET:
        if (ev.type == RaceEventType::EV_RACE_RESET_REQ)
            Serial.println("[RESET] Reset ya en progreso");
        break;
    }
}

void RaceTask(void *pv)
{
    Serial.println("[RACE_TASK] Tarea creada");

    timerInit();
    blinkInit();
    semaphoreInit();
    barrierInit();

    raceQ = (QueueHandle_t)pv;
    sState = RaceState::EST_RACE_RESET;
    onEntry(RaceState::EST_RACE_RESET);

    RaceEvent ev;
    for (;;)
    {
        if (xQueueReceive(raceQ, &ev, pdMS_TO_TICKS(100)) == pdTRUE)
            handleEvent(ev);

        if (webRaceStart() && sState == RaceState::EST_RACE_IDLE) {
            RaceEvent evToSend = { .type = RaceEventType::EV_RACE_START, .arg = 0 };
            xQueueSendToBack(raceQ, &evToSend, portMAX_DELAY);
        }

        if (xSemaphoreTake(xBarrierSemaphore, 0) == pdTRUE) {
            uint32_t now = xTaskGetTickCount();
            uint32_t elapsedMs = (now - lastBarrierTick) * portTICK_PERIOD_MS;
            if (elapsedMs < 10000) {
                Serial.println("[BARRIER] Ignorado por debounce");
            } else {
                lastBarrierTick = now;
                if (sState == RaceState::EST_RACE_SEMAPHORE && !endSemaphoreSeq) {
                    Serial.println("[BARRIER] Largada falsa!");
                    webFalseStart();
                    RaceEvent evToSend = { .type = RaceEventType::EV_RACE_RESET_REQ, .arg = 0 };
                    xQueueSendToBack(raceQ, &evToSend, portMAX_DELAY);
                }
                else if (sState == RaceState::EST_RACE_SEMAPHORE && endSemaphoreSeq) {
                    RaceEvent evToSend = { .type = RaceEventType::EV_RACE_SEMAPHORE_REQ, .arg = 0 };
                    xQueueSendToBack(raceQ, &evToSend, portMAX_DELAY);
                }
                else if (sState == RaceState::EST_RACE_RUNNING) {
                    RaceEvent evToSend = { .type = RaceEventType::EV_RACE_FINISH_REQ, .arg = 0 };
                    xQueueSendToBack(raceQ, &evToSend, portMAX_DELAY);
                }
            }
        }

        if (webRaceReset()) {
            RaceEvent evToSend = { .type = RaceEventType::EV_RACE_RESET_REQ, .arg = 0 };
            xQueueSendToBack(raceQ, &evToSend, portMAX_DELAY);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


// ------------------ TIMERS ---------------------
static void timerInit(void)
{
    xSemaphoreTimer = xTimerCreate("SemTimer", pdMS_TO_TICKS(SEMAPHORE_PERIODE_1S), pdFALSE, NULL, semaphoreTimerCallback);
    xWebTimer = xTimerCreate("WebTimer", pdMS_TO_TICKS(WEB_TIMER_UPDATE_PERIOD), pdTRUE, NULL, webTimerCallback);
    xBlinkTimer = xTimerCreate("BlinkTimer", pdMS_TO_TICKS(500), pdTRUE, NULL, blinkTimerCallback);
}

static void timerReset(void)
{
    if (xSemaphoreTimer) xTimerStop(xSemaphoreTimer, portMAX_DELAY);
    if (xWebTimer) xTimerStop(xWebTimer, portMAX_DELAY);
}

// ------------------ SEMÁFORO --------------------
static void semaphoreInit(void)
{
    sSemStep = 0;
    endSemaphoreSeq = false;

    pinMode(SEMAPHORE_RED_LIGHT, OUTPUT);
    pinMode(SEMAPHORE_YELLOW_LIGHT, OUTPUT);
    pinMode(SEMAPHORE_GREEN_LIGHT, OUTPUT);

    digitalWrite(SEMAPHORE_RED_LIGHT, LOW);
    digitalWrite(SEMAPHORE_YELLOW_LIGHT, LOW);
    digitalWrite(SEMAPHORE_GREEN_LIGHT, LOW);
}

static void semaphoreSequence(uint32_t n)
{
    Serial.printf("[SEMAPHORE] step=%lu\n", (unsigned long)n);
    webSemaphoreState(n);

    switch (n)
    {
    case 0:
        // Rojo ON
        digitalWrite(SEMAPHORE_RED_LIGHT, HIGH);
        digitalWrite(SEMAPHORE_YELLOW_LIGHT, LOW);
        digitalWrite(SEMAPHORE_GREEN_LIGHT, LOW);

        // Preparar siguiente periodo (amarillo) en 1s
        if (xSemaphoreTimer) {
            xTimerChangePeriod(xSemaphoreTimer, pdMS_TO_TICKS(SEMAPHORE_PERIODE_1S), 0);
            xTimerStart(xSemaphoreTimer, 0);
        }
        testSemaphoreSequence();
        break;

    case 1:
        // Rojo + Amarillo ON
        digitalWrite(SEMAPHORE_RED_LIGHT, HIGH);
        digitalWrite(SEMAPHORE_YELLOW_LIGHT, HIGH);
        digitalWrite(SEMAPHORE_GREEN_LIGHT, LOW);

        // Preparar siguiente periodo (verde) en 3s
        if (xSemaphoreTimer) {
            xTimerChangePeriod(xSemaphoreTimer, pdMS_TO_TICKS(SEMAPHORE_PERIODE_3S), 0);
            xTimerStart(xSemaphoreTimer, 0);
        }
        testSemaphoreSequence();
        break;

    case 2:
        // Verde ON -> mantener hasta primer cruce
        digitalWrite(SEMAPHORE_RED_LIGHT, LOW);
        digitalWrite(SEMAPHORE_YELLOW_LIGHT, LOW);
        digitalWrite(SEMAPHORE_GREEN_LIGHT, HIGH);

        // Marcamos que la secuencia terminó y ahora esperamos cruce
        endSemaphoreSeq = true;

        // No rearmamos más timers del semáforo; queda a la espera de cruce
        if (xSemaphoreTimer) xTimerStop(xSemaphoreTimer, portMAX_DELAY);

        testSemaphoreSequence();
        break;

    default:
        // Aseguramos estado seguro (apagado)
        digitalWrite(SEMAPHORE_RED_LIGHT, LOW);
        digitalWrite(SEMAPHORE_YELLOW_LIGHT, LOW);
        digitalWrite(SEMAPHORE_GREEN_LIGHT, LOW);
        if (xSemaphoreTimer) xTimerStop(xSemaphoreTimer, portMAX_DELAY);
        break;
    }
}

static void semaphoreTimerCallback(TimerHandle_t xTimer)
{
    // Avanzar paso
    sSemStep++;

    // Si se pasa del último paso, no avanzar más
    if (sSemStep > kSemSteps) {
        sSemStep = kSemSteps;
    }

    semaphoreSequence(sSemStep);
}

static void semaphoreReset(void)
{
    sSemStep = 0;
    endSemaphoreSeq = false;
    digitalWrite(SEMAPHORE_RED_LIGHT, LOW);
    digitalWrite(SEMAPHORE_YELLOW_LIGHT, LOW);
    digitalWrite(SEMAPHORE_GREEN_LIGHT, LOW);
}

// ------------------ RACE -----------------------
static void raceReset(void)
{
    raceStartTick = 0;
    raceEndTick = 0;
}

static void raceRunning(void)
{
    // Guardar tick de inicio
    raceStartTick = xTaskGetTickCount();
}

// ------------------ WEB TIMER -------------------
static void webTimerCallback(TimerHandle_t xTimer)
{
    if (raceStartTick == 0) return; // protección

    uint32_t raceIntTick = xTaskGetTickCount();
    uint32_t elapsedTicks = raceIntTick - raceStartTick;
    uint32_t elapsedMs = elapsedTicks * portTICK_PERIOD_MS;

    webTempUpdate(elapsedMs);
}

// ------------------ BLINK ----------------------
static void blinkTimerCallback(TimerHandle_t xTimer)
{
    // Toggle internal LED si querés
    // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

static void blinkInit(void)
{
    pinMode(LED_BUILTIN, OUTPUT);
    if (xBlinkTimer) xTimerStart(xBlinkTimer, 0);
}

// ------------------ BARRERA --------------------
static void barrierInit(void)
{
    xBarrierSemaphore = xSemaphoreCreateBinary();

    pinMode(BARRIER_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BARRIER_PIN), barrierISR, FALLING);
}

void IRAM_ATTR barrierISR()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xBarrierSemaphore) {
        xSemaphoreGiveFromISR(xBarrierSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// ------------------ TEST ------------------------
static void testSemaphoreSequence(void)
{
    // función vacía para debug; se dejó para posibles prints
}

void testTask(void *pv)
{
    // Implementación de test por consola deshabilitada (queda como placeholder)
    (void)pv;
    vTaskDelay(pdMS_TO_TICKS(1000));
    for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}
