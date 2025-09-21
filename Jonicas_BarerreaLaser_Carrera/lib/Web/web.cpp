#include "web.h"
#include "config.h"

void webReset(void)
{
    // Debug...
    Serial.println("[RESET] webReset");

    // Acicones...
}

bool webRaceStart(void)
{
    // Debug...
    Serial.println("[IDLE] webRaceStart");

    // Acciones...
    // Para probarlo vamos a realizar un test que se encargue de mandar un true
}

void webTempUpdate(uint32_t time)
{
    // Debug...
    Serial.println("[RUNNING] webUpdateTemp:");
    Serial.print(time / 1000.0);
    Serial.println(" s");

    // Acciones...
}