#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <HTTPClient.h>

#define BUTTON_PIN 4
#define COIN_PIN 16  // Pin for the coin detector

// Timing parameters
volatile int pulseCount = 0;
unsigned long lastPulseTime = 0;
const unsigned long pulseTimeout = 1000;  // Timeout between pulses in milliseconds

// Access Point (AP) configuration
const char* AP_SSID = "SplitPay_Device";
const char* AP_PASSWORD = "SplitPay24";

bool serverOnline = false;
bool wifi_connected = false;

// Web server instance on port 80
WebServer server(80);

// Variables for Wi-Fi credentials and server endpoint
String wifiSSID, wifiPassword, serverEndpoint, splitpayPassword;

/**
 * Interrupt Service Routine for coin pulse detection
 * Increments pulse count and records the time of the last pulse.
 */
void IRAM_ATTR onCoinPulse() {
  pulseCount++;
  lastPulseTime = millis();
}

/**
 * Setup for the coin detector pin and interrupt.
 */
void initializeCoinDetector() {
  pinMode(COIN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(COIN_PIN), onCoinPulse, FALLING);
}

/**
 * Check if the BOOT button is pressed.
 */
bool isButtonPressed() {
  return digitalRead(BUTTON_PIN) == LOW;  // Assuming LOW when pressed
}

/**
 * Setup function initializes the ESP32, SPIFFS, Wi-Fi Access Point, and web server.
 */
void setup() {
  Serial.begin(115200);

  // Set the BUTTON pin as input
  pinMode(BUTTON_PIN, INPUT_PULLUP);


  // Initialize SPIFFS for storing Wi-Fi credentials and server endpoint
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS.");
    return;
  }



  // Load any saved credentials from SPIFFS
  loadCredentials();
  
  if (isButtonPressed()) {
    serverOnline = true;
    // Start Access Point

    if (wifi_connected) {
      WiFi.softAP(AP_SSID, AP_PASSWORD);
      Serial.println("Access Point initialized. Connect to 'SplitPay'.");
    }

    // Setup web server routes
    server.on("/", handleRoot);      // Route for configuration form
    server.on("/save", handleSave);  // Route to save the configuration
    server.begin();
    Serial.println("Web server started.");
  }

  // Initialize the coin detector
  initializeCoinDetector();
}

/**
 * Main loop continuously handles web server requests and sends coin data when required.
 */
void loop() {
  if (serverOnline) {
    server.handleClient();
  }

  // If coin pulses were detected, send the data after the timeout
  if (pulseCount > 0 && millis() - lastPulseTime > pulseTimeout) {
    sendCoinData();
    pulseCount = 0;  // Reset the pulse count after sending
  }
}

/**
 * Serve the root page, which provides a configuration form for Wi-Fi and endpoint settings.
 */
void handleRoot() {
  String html = "<form action='/save' method='POST'>"
                "SSID: <input type='text' name='ssid' value='"
                + wifiSSID + "'><br>"
                             "Password: <input type='password' name='password' value='"
                + wifiPassword + "'><br>"
                                 "Endpoint: <input type='text' name='endpoint' value='"
                + serverEndpoint + "'><br>"
                                   "SplitPay Password: <input type='password' name='splitpay_password' value='"
                + splitpayPassword + "' ><br>"
                                     "<input type='submit' value='Save'></form>";

  // Check if the error parameter is present in the URL
  if (server.hasArg("error")) {
    String errorMessage = server.arg("error");
    html += "<p style='color:red;'>Error: " + errorMessage + "</p>";
  }

  // Send the HTML content with the error message (if any)
  server.send(200, "text/html", html);
}
/**
 * Save Wi-Fi credentials and server endpoint to SPIFFS.
 * After saving, the ESP32 restarts to apply the new configuration.
 */
bool verifySplitpayToken(const String& token) {
  HTTPClient http;
  String verifyEndpoint = serverEndpoint + "/verify_splitpay";

  // Configure POST request
  http.begin(verifyEndpoint);  // URL of the verification endpoint
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + token);

  // Send POST request
  int httpResponseCode = http.GET();

  // Check if the response is HTTP 200 OK
  if (httpResponseCode == 200) {
    Serial.println("Token Activado !");
    http.end();  // Close connection
    return true;
  } else {
    Serial.print("Token verification failed. HTTP Response Code: ");

    http.end();  // Close connection
    return false;
  }
}

void handleSave() {
  // Get the arguments from the web form
  wifiSSID = server.arg("ssid");
  wifiPassword = server.arg("password");
  serverEndpoint = server.arg("endpoint");
  splitpayPassword = server.arg("splitpay_password");

  wifiSSID.trim();
  wifiPassword.trim();
  serverEndpoint.trim();
  splitpayPassword.trim();

  // Save credentials to SPIFFS
  saveCredentials();

  connectToWiFi();

  // Verify the Splitpay token
  if (verifySplitpayToken(splitpayPassword)) {
    // Send success response and restart the ESP32
    server.send(200, "text/html", "Settings saved! Rebooting...");
    delay(1000);
    ESP.restart();  // Restart ESP32 to apply changes
  } else {
    // If verification fails, redirect to home with an error message
    splitpayPassword = "";
    saveCredentials();
    server.sendHeader("Location", "/?error=Verification failed", true);
    server.send(302, "text/plain", "");
  }
}

/**
 * Save Wi-Fi and server endpoint credentials to SPIFFS.
 */
void saveCredentials() {
  File configFile = SPIFFS.open("/config.txt", FILE_WRITE);
  if (!configFile) {
    Serial.println("Error opening file for writing.");
    return;
  }

  // Save credentials line by line
  configFile.println(wifiSSID);
  configFile.println(wifiPassword);
  configFile.println(serverEndpoint);
  configFile.println(splitpayPassword);
  configFile.close();
  Serial.println("Credentials saved.");
}

/**
 * Load Wi-Fi credentials and server endpoint from SPIFFS.
 * Connect to Wi-Fi if valid credentials are found.
 */
void loadCredentials() {
  File configFile = SPIFFS.open("/config.txt", FILE_READ);
  if (!configFile) {
    Serial.println("Failed to open config file.");
    return;
  }

  // Read the credentials line by line
  wifiSSID = configFile.readStringUntil('\n');
  wifiPassword = configFile.readStringUntil('\n');
  serverEndpoint = configFile.readStringUntil('\n');
  splitpayPassword = configFile.readStringUntil('\n');
  wifiSSID.trim();
  wifiPassword.trim();
  serverEndpoint.trim();
  splitpayPassword.trim();
  configFile.close();

  // Attempt to connect to Wi-Fi if credentials are available
  if (!wifiSSID.isEmpty() && !wifiPassword.isEmpty()) {
    connectToWiFi();
  }
}

/**
 * Connect to Wi-Fi using the saved credentials.
 */
void connectToWiFi() {

  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  Serial.print("Connecting to WiFi");
  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startTime > 25000) {  // Timeout after 15 seconds
      Serial.println("\nFailed to connect to Wi-Fi.");
      return;
    }
  }

  wifi_connected = true;
  Serial.println("\nConnected to WiFi.");
}

/**
 * Send coin pulse data to the configured server endpoint.
 */
void sendCoinData() {
  int coinValue = getCoinValue(pulseCount);

  // Prepare JSON data for the POST request
  String postData = "{\"value\": " + String(coinValue) + "}";
  http.addHeader("Authorization", "Bearer " + token);

  HTTPClient http;
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(serverEndpoint);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(postData);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server response: " + response);
    } else {
      Serial.println("POST request failed: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("Not connected to Wi-Fi.");
  }
}

/**
 * Convert the number of pulses to the corresponding coin value.
 */
int getCoinValue(int pulses) {
  switch (pulses) {
    case 1: return 25;   // Quarter
    case 2: return 100;  // 1 Dollar
    case 3: return 10;   // Dime
    case 4: return 5;    // Nickel
    case 5: return 1;    // Penny
    default: return 0;   // Unknown value
  }
}
