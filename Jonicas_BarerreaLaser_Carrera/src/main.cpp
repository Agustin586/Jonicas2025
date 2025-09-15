#include <Arduino.h>

// Declaración de la tarea
void TaskBlinkLED(void *pvParameters);

// put function declarations here:
int myFunction(int, int);

void setup() {
  Serial.begin(115200); // Inicializa el puerto serie

  // Crear la tarea de parpadeo del LED
  xTaskCreate(
    TaskBlinkLED,      // Función de la tarea
    "BlinkLED",        // Nombre de la tarea
    2048,              // Tamaño del stack (más grande por seguridad)
    NULL,              // Parámetro para la tarea
    1,                 // Prioridad
    NULL               // Handle de la tarea
  );

  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  Serial.print("Resultado de myFunction: ");
  Serial.println(result);
}

void loop() {
  // No es necesario usar loop() con FreeRTOS, las tareas se encargan
}

// Definición de la tarea
void TaskBlinkLED(void *pvParameters) {
  (void) pvParameters;
  pinMode(LED_BUILTIN, OUTPUT);

  for (;;) {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("LED ON");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("LED OFF");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}