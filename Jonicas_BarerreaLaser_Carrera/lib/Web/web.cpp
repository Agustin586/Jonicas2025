#include "web.h"
#include "config.h"

void webTask(void *pv)
{
    // Inicializaciones...

    for(;;)
    {
        // Debug...
        Serial.println("[WEB] webTask running");

        // Acciones...

        vTaskDelay(pdMS_TO_TICKS(1000));  // Le pongo un delay para que no sobrecargue las tareas, se cambiar.
    }
}

void webReset(void)
{
    // Debug...
    Serial.println("[RESET] webReset");

    // Acicones...
}

bool webRaceStart(void)
{
    bool val = false;
    // Debug...
    // Serial.println("[IDLE] webRaceStart");

    // Acciones...
    return val;
}

void webTempUpdate(uint32_t time)
{
    // Debug...
    Serial.printf("[RUNNING] webUpdateTemp: %.2f s\n", time / 1000.0);

    // Acciones...
}

void webRaceInit(void)
{
    // Debug...
    Serial.println("[RUNNING] webRaceInit");

    // Acciones...
}

bool webRaceReset(void)
{
    bool reset = false;
    // Debug...
    // Serial.println("[FINISH] webRaceReset");

    // Acciones...
    return reset;
}

void webRaceFinish(void)
{
    // Debug...
    Serial.println("[FINISH] webRaceFinish");
}