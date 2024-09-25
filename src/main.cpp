#include <WiFi.h>
#include <HTTPClient.h>

// Pines del sensor HC-SR04
constexpr int8_t TRIGGER_PIN = 18;
constexpr int8_t ECHO_PIN = 19;
constexpr int8_t LED_PIN = 5;

// Datos de la red WiFi
const char *ssid = "NOMBRE_DE_TU_WIFI";
const char *password = "CONTRASEÑA_DE_TU_WIFI";

// Variables para el manejo del tiempo
uint64_t previousMillis = 0; // Para el intervalo de medición
constexpr uint64_t DISTANCE_UPDATE_DELAY = 2000;       // Intervalo de tiempo (2 segundos)

constexpr float SOUND_SPEED = 0.034; /* Constante en cm/us */

// Función para medir la distancia usando el sensor HC-SR04
long medirDistancia() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  long duracion = pulseIn(ECHO_PIN, HIGH);
  long distancia = duracion * SOUND_SPEED / 2;
  return distancia;
}

// Función para enviar notificación al servidor web
void enviarNotificacion(long distancia) {
  if (WiFi.status() == WL_CONNECTED) { // Verificar si el ESP32 está conectado a WiFi
    HTTPClient http;
    String url = "http://192.168.1.52/notificaciones.php"; // Reemplaza con la URL de tu servidor
    url += "?distancia=";
    url += distancia;

    http.begin(url); // Iniciar la solicitud HTTP
    int httpResponseCode = http.GET(); // Realizar la solicitud GET

    if (httpResponseCode > 0) {
      Serial.print("Notificación enviada, código de respuesta: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error en la solicitud: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }

    http.end(); // Finalizar la solicitud
  } else {
    Serial.println("Error: No conectado a WiFi");
  }
}

// Tarea que mide la distancia y maneja la alarma
void medirDistanciaTask(void *pvParameters) {
  for (;;) {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= DISTANCE_UPDATE_DELAY) {
      previousMillis = currentMillis;

      long distancia = medirDistancia();
      Serial.print("Distancia: ");
      Serial.print(distancia);
      Serial.println(" cm");

      // Activar la alarma si la distancia es menor a 10 cm
      if (distancia < 10) {
        digitalWrite(LED_PIN, HIGH); // Encender el LED
        enviarNotificacion(distancia); // Enviar la notificación por WiFi
      } else {
        digitalWrite(LED_PIN, LOW); // Apagar el LED
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(115200);

  // Configurar los pines
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // Conectar a la red WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");

  // Crear la tarea para medir la distancia y manejar el tiempo
  xTaskCreate(medirDistanciaTask, "Medir Distancia", 4096, NULL, 1, NULL);
}

void loop() { }
