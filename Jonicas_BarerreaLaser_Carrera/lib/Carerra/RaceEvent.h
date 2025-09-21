#ifndef RACE_EVENTS_H
#define RACE_EVENTS_H

#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// -------- Eventos de la MEF Carrera --------
enum class RaceEventType : uint8_t {
  EV_RACE_START,        // startRace
  EV_RACE_FINISH_REQ,   // finishRace
  EV_RACE_RESET_REQ,    // reset general
  EV_RACE_SEMAPHORE_REQ,// semáforo
  EV_TICK               // tick periódico (driver de SEMAPHORE/RUNNING)
};

struct RaceEvent {
  RaceEventType type;
  uint32_t arg;         // opcional (p.ej., paso de semáforo, datos)
};

// Cola global de la MEF
extern QueueHandle_t gRaceQ;

// Helpers
inline bool postRace(RaceEventType t, uint32_t arg = 0) {
  if (!gRaceQ) return false;
  RaceEvent e{t, arg};
  return xQueueSend(gRaceQ, &e, 0) == pdTRUE;
}

// API pública: mantené los nombres del PDF
inline void startRace()    { postRace(RaceEventType::EV_RACE_START); }
inline void finishRace()   { postRace(RaceEventType::EV_RACE_FINISH_REQ); }
inline void resetRace()    { postRace(RaceEventType::EV_RACE_RESET_REQ); }

// -------- Hooks (stubs): implementalos en tu app --------
void userResetData();

void webStartRace();                 
void webInitRace();                   
void tempSemStart();
void tempRunStart();
void tempRunStop();
void tempStop();
void webFinishRace(uint32_t userDataInfo);  
uint32_t userGetDataInfo();

void webRaceStop();
void webResetRace();
void contVueltaInc();

#endif // RACE_EVENTS_H