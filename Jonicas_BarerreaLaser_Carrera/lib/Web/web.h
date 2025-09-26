#ifndef WEB_H
#define WEB_H

#include <Arduino.h>

// ===============================================
// DECLARACIONES DE FUNCIONES PARA LA TAREA WEB
// ===============================================
void webTask(void *pv);

// ===============================================
// FUNCIONES DE INTERFAZ PARA COMUNICACIÓN CON LA MEF
// ===============================================

// Funciones llamadas desde RaceTask para verificar comandos web
bool webRaceStart(void);        // Retorna true si se solicitó inicio de carrera
bool webRaceReset(void);        // Retorna true si se solicitó reset

// Funciones llamadas desde RaceTask para notificar cambios de estado
void webRaceInit(void);         // Llamar cuando la carrera inicia (RUNNING)
void webRaceFinish(void);       // Llamar cuando la carrera termina (FINISH) 
void webReset(void);            // Llamar cuando se resetea el sistema (RESET->IDLE)
void webTempUpdate(uint32_t time); // Llamar periódicamente con el tiempo transcurrido
void webFalseStart(void);   // Llamar en caso de largada falsa (BARRIER en SEMAPHORE)

// Funciones adicionales para integración
void webSemaphoreState(uint32_t step);    // Notificar paso del semáforo (opcional)
void webUpdateRaceState(const char* state); // Sincronizar estado general

#endif // WEB_H