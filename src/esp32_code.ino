#include <WiFi.h>

// ----------- WIFI CREDENTIALS -----------
const char* ssid = "realme 10 Pro 5G";
const char* password = "erenyeager";

// ----------- PIN DEFINITIONS -------------
const int voltagePin = 34;     // ADC input
const int ledPin = 2;          // On-board LED
const int stepPin = 27;        // FOOTSTEP SENSOR PIN

// ----------- PARAMETERS ------------------
float maxSafeVoltage = 4.9;
float voltageDividerRatio = 2.0;

// ----------- STEP COUNTER ----------------
volatile int steps = 0;
unsigned long lastStepTime = 0;
const unsigned long debounceDelay = 300; // ms

WiFiServer server(80);

// ----------- INTERRUPT -------------------
void IRAM_ATTR stepDetected() {
  unsigned long now = millis();
  if (now - lastStepTime > debounceDelay) {
    steps++;
    lastStepTime = now;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  pinMode(stepPin, INPUT_PULLDOWN);

  attachInterrupt(digitalPinToInterrupt(stepPin),
                  stepDetected, RISING);

  Serial.println("ESP32 Energy Harvesting Tile System");

  // -------- WiFi Connect --------
  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi FAILED");
  }

  server.begin();
}

void loop() {
  // -------- READ VOLTAGE --------
  int rawADC = analogRead(voltagePin);
  float voltage = (rawADC / 4095.0) * 3.3 * voltageDividerRatio;

  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.print(" V | Steps: ");
  Serial.println(steps);

  // -------- LED WARNING --------
  digitalWrite(ledPin, voltage >= maxSafeVoltage);

  // -------- WEB SERVER --------
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("/data") != -1) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Access-Control-Allow-Origin: *");
      client.println();
      client.print("{\"voltage\":");
      client.print(voltage, 2);
      client.print(",\"steps\":");
      client.print(steps);
      client.print("}");
    }
    else {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println();
      client.println("<h1>ESP32 Energy Monitor</h1>");
      client.print("<p>Voltage: ");
      client.print(voltage);
      client.println(" V</p>");
      client.print("<p>Steps: ");
      client.print(steps);
      client.println("</p>");
    }

    client.stop();
  }

  delay(200);
}
