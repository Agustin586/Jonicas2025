#ifndef WEB_H
#define WEB_H

#include <Arduino.h>

/**
 * @brief Tarea donde se ejecuta todo lo relacionado con la web.
 */
void webTask(void *pv);
/**
 * @brief Reincia condiciones en la web
 */
void webReset(void);
/**
 * @brief Manda el inicio de la carrera via web
 * @return True/Inicia la carrera, False/No inicia la carrera
 */
bool webRaceStart(void);
/**
 * @brief Actualiza el temporizador de la web cada cierto en config.h
 * @param time Tiempo en ms
 */
void webTempUpdate(uint32_t time);
/**
 * @brief Inicia la carrera desde la web. Utilizada solo para información visual
 */
void webRaceInit(void);
/**
 * @brief Resete la carrera cuando esta es finalizada
 * @return True: resetea la mef / False: sigue en finish
 */
bool webRaceReset(void);

#endif // WEB_H