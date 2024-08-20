#include <WiFi.h>
#include <HTTPClient.h>

// Configuration
const int COIN_ACCEPTOR_PIN = 16;
const char* WIFI_SSID = "LABCFP-32 8991";
const char* WIFI_PASSWORD = "12345678";
const char* SPLITPAY_SERVER = "http://192.168.1.216:5001/deposit";  // Replace with actual server URL
const unsigned long TIEMPO_MAXIMO = 300;  // milliseconds

// Global variables
volatile int contador = 0;
unsigned long tiempo_inicio = 0;

void IRAM_ATTR handleCoinPulse() {
  contador++;
  if (tiempo_inicio == 0) {
    tiempo_inicio = millis();
  }
}

void setup() {
  Serial.begin(115200);
  
  // Setup Wi-Fi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Setup coin acceptor pin
  pinMode(COIN_ACCEPTOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(COIN_ACCEPTOR_PIN), handleCoinPulse, FALLING);

  Serial.println("SplitPay: Esperando a Insertar Monedas");
}

void loop() {
  unsigned long currentMillis = millis();

  if (tiempo_inicio != 0 && (currentMillis - tiempo_inicio >= TIEMPO_MAXIMO)) {
    String payload;
    
    if (contador == 1) {
      Serial.println("$0.25");
      payload = "{\"value\": \"0.25\"}";
    } else if (contador == 2) {
      Serial.println("$1.00");
      payload = "{\"value\": \"1.00\"}";
    } else if (contador == 3) {
      Serial.println("$0.05");
      payload = "{\"value\": \"0.05\"}";
    } else if (contador == 5) {
      Serial.println("$0.10");
      payload = "{\"value\": \"0.10\"}";
    } else {
      payload = "";
    }
    
    if (payload != "") {
      sendPostRequest(SPLITPAY_SERVER, payload);
    }
    
    // Reset counters
    contador = 0;
    tiempo_inicio = 0;
  }

  delay(10);  // Add a small delay to avoid a busy loop
}

void sendPostRequest(const char* serverUrl, const String& payload) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl); // Specify the URL

    http.addHeader("Content-Type", "application/json"); // Specify content-type header

    int httpResponseCode = http.POST(payload); // Send the request
    if (httpResponseCode > 0) {
      String response = http.getString(); // Get the response payload
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error in HTTP request: " + String(httpResponseCode));
    }

    http.end(); // Free resources
  } else {
    Serial.println("Error: WiFi not connected");
  }
}