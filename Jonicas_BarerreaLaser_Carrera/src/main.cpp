#include <Arduino.h>
#include "RaceEvent.h"
#include "RaceMef.h"

QueueHandle_t gRaceQ = nullptr;
static TaskHandle_t hRaceTask = nullptr;

// --------- Hooks de ejemplo: reemplazá por tu lógica real ----------
void userResetData()                  { Serial.println("[RESET] userResetData"); }

void webStartRace()                   { Serial.println("[SEMAPHORE] webStartRace"); }
void webInitRace()                    { Serial.println("[RUNNING] webInitRace"); }
void tempSemStart()                   { Serial.println("[SEMAPHORE] tempSemStart"); }
void tempRunStart()                   { Serial.println("[RUNNING] tempRunStart"); }
void tempRunStop()                    { Serial.println("[STOP] tempRunStop"); }
void tempStop()                       { Serial.println("[FINISH] tempStop"); }
void webFinishRace(uint32_t info)     { Serial.printf("[FINISH] webFinishRace info=%lu\n", (unsigned long)info); }
uint32_t userGetDataInfo()            { return 42; }
void webRaceStop()                    { Serial.println("[STOP] webRaceStop"); }
void webResetRace()                   { Serial.println("[FINISH] webResetRace"); }
void contVueltaInc()                  { Serial.println("[FINISH] contVueltaInc"); }

// ---------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nMEF Carrera (ESP32 + FreeRTOS)");

  gRaceQ = xQueueCreate(16, sizeof(RaceEvent));
  xTaskCreatePinnedToCore(RaceTask, "RaceTask", 4096, (void*)gRaceQ, 2, &hRaceTask, APP_CPU_NUM);

  // --- Escenario de prueba: start -> semáforo -> running -> finish ---
  // Lanzá eventos desde tu UI real; esto es solo demo:
  // startRace();

  // // Simular un finish a los ~5 segundos
  // xTaskCreatePinnedToCore([](void*) {
  //   vTaskDelay(pdMS_TO_TICKS(5000));
  //   finishRace();
  //   vTaskDelete(nullptr);
  // }, "DemoFinish", 2048, nullptr, 1, nullptr, APP_CPU_NUM);
}

void loop() {
  // nada: la MEF corre en su tarea
}
