#ifndef WEB_H
#define WEB_H

#include <Arduino.h>

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

#endif // WEB_H