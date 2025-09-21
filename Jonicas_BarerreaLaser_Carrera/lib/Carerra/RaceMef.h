#ifndef CARRERA_H
#define CARRERA_H   

#pragma once
#include "RaceEvent.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

enum class RaceState : uint8_t {
  EST_RACE_RESET,
  EST_RACE_IDLE,
  EST_RACE_SEMAPHORE,
  EST_RACE_RUNNING,
  EST_RACE_FINISH
};

void RaceTask(void* pv);
// Test
void testTask(void *pv);

#endif // CARRERA_H