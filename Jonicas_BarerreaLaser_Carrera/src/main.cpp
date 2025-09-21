#include <Arduino.h>
#include "RaceEvent.h"
#include "RaceMef.h"
#include "web.h"

QueueHandle_t gRaceQ = nullptr;
static TaskHandle_t hRaceTask = nullptr;

// --------- Hooks de ejemplo: reemplazá por tu lógica real ----------
// void userResetData()                  { Serial.println("[RESET] userResetData"); }
// void webStartRace()                   { Serial.println("[SEMAPHORE] webStartRace"); }
// void webInitRace()                    { Serial.println("[RUNNING] webInitRace"); }
// void tempSemStart()                   { Serial.println("[SEMAPHORE] tempSemStart"); }
// void tempRunStart()                   { Serial.println("[RUNNING] tempRunStart"); }
// void tempRunStop()                    { Serial.println("[STOP] tempRunStop"); }
// void tempStop()                       { Serial.println("[FINISH] tempStop"); }
// void webFinishRace(uint32_t info)     { Serial.printf("[FINISH] webFinishRace info=%lu\n", (unsigned long)info); }
// uint32_t userGetDataInfo()            { return 42; }
// void webRaceStop()                    { Serial.println("[STOP] webRaceStop"); }
// void webResetRace()                   { Serial.println("[FINISH] webResetRace"); }
// void contVueltaInc()                  { Serial.println("[FINISH] contVueltaInc"); }

// ---------------------------------------------------------------
void setup()
{
  BaseType_t statusTaskCreate;

  Serial.begin(115200);
  Serial.println("\nMEF Carrera (ESP32 + FreeRTOS)");

  gRaceQ = xQueueCreate(16, sizeof(RaceEvent));

  statusTaskCreate = xTaskCreatePinnedToCore(
      RaceTask,       // Funcion donde se ejecuta la tarea
      "RaceTask",     // Nombre de la tarea
      4096,           // Cantidad de stack prevista
      (void *)gRaceQ, // Datos a enviar
      2,              // Prioridad de la tarea
      &hRaceTask,     // Handle de la tarea (no hace falta)
      APP_CPU_NUM);   // Nucleo en el que se ejecuta

  if (statusTaskCreate != pdPASS)
  {
    Serial.println("[ERROR] No se puede crear la tarea RaceTask");
    while (1)
      ; // No se pudo crear la tarea
  }

  // statusTaskCreate = xTaskCreatePinnedToCore(
  //     testTask,       // Funcion donde se ejecuta la tarea
  //     "TestTask",     // Nombre de la tarea
  //     4096,           // Cantidad de stack prevista
  //     (void *)gRaceQ, // Datos a enviar
  //     3,              // Prioridad de la tarea
  //     NULL,           // Handle de la tarea (no hace falta)
  //     APP_CPU_NUM);   // Nucleo en el que se ejecuta

  // if (statusTaskCreate != pdPASS)
  // {
  //   Serial.println("[ERROR] No se puede crear la tarea TestTask");
  //   while (1)
  //     ; // No se pudo crear la tarea
  // }

  statusTaskCreate = xTaskCreatePinnedToCore(
    webTask,
    "WebTask",
    4096,
    NULL,
    1,
    NULL,
    APP_CPU_NUM);

  if (statusTaskCreate != pdPASS)
  {
    Serial.println("[ERROR] No se puede crear la tarea WebTask");
    while (1)
      ; // No se pudo crear la tarea
  }
}

void loop()
{
  // nada: la MEF corre en su tarea
}
