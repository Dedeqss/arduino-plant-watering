#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <stdio.h>
#include <string.h>

// Copiaza secrets.example.h in secrets.h in acelasi folder.
// COMPLETEAZA valorile doar local; secrets.h este ignorat de Git.
// ESP32 UART2: TX2(GPIO17) -> UNO D10 direct
// ESP32 RX2(GPIO16) <- UNO D11 PRIN divizor rezistiv 5V->3.3V
#include "secrets.h"

#define ESP_RX 16
#define ESP_TX 17

bool automaticMode = true;
bool manualLED = false;
int thresholdC = 24;
bool ledState = false;
bool sensorValid = false;
bool haveData = false;

float temperature = 0.0f;
float airHumidity = 0.0f;
int soilPercent = 0;
int soilRaw = 0;

char rxBuffer[80];
size_t rxLength = 0;
BlynkTimer timer;

void sendConfig() {
  Serial2.print("CFG,");
  Serial2.print(automaticMode ? 1 : 0);
  Serial2.print(',');
  Serial2.print(manualLED ? 1 : 0);
  Serial2.print(',');
  Serial2.println(thresholdC);
}

BLYNK_WRITE(V3) {
  automaticMode = param.asInt() == 1;
  sendConfig();
}

BLYNK_WRITE(V4) {
  manualLED = param.asInt() == 1;
  sendConfig();
}

BLYNK_WRITE(V5) {
  thresholdC = constrain(param.asInt(), 15, 40);
  sendConfig();
}

BLYNK_CONNECTED() {
  Serial.println("Blynk conectat");
  Blynk.syncVirtual(V4, V5, V3);
}

void processLine(const char* line) {
  int reportedLED;
  int reportedMode;
  int percent;
  int raw;
  float temp, humidity;

  if (sscanf(line, "ERROR,%d", &reportedLED) == 1) {
    sensorValid = false;
    ledState = reportedLED == 1;
    Serial.println("Eroare citire DHT11");
    return;
  }

  if (sscanf(line, "DATA,%f,%f,%d,%d,%d,%d",
             &temp, &humidity, &reportedLED,
             &reportedMode, &percent, &raw) != 6) {
    Serial.print("Date ignorate: ");
    Serial.println(line);
    return;
  }

  if (reportedLED < 0 || reportedLED > 1 ||
      reportedMode < 0 || reportedMode > 1 ||
      raw < 0 || raw > 1023) {
    return;
  }

  temperature = temp;
  airHumidity = humidity;
  ledState = reportedLED == 1;
  soilPercent = constrain(percent, 0, 100);
  soilRaw = raw;
  sensorValid = true;
  haveData = true;

  Serial.print("T: ");
  Serial.print(temperature, 1);
  Serial.print(" C | Aer: ");
  Serial.print(airHumidity, 1);
  Serial.print("% | Sol: ");
  Serial.print(soilPercent);
  Serial.print("% | RAW: ");
  Serial.print(soilRaw);
  Serial.print(" | LED: ");
  Serial.println(ledState ? "ON" : "OFF");
}

void readArduino() {
  while (Serial2.available()) {
    char c = (char)Serial2.read();
    if (c == '\n') {
      rxBuffer[rxLength] = '\0';
      if (rxLength) processLine(rxBuffer);
      rxLength = 0;
    } else if (c != '\r') {
      if (rxLength < sizeof(rxBuffer) - 1) {
        rxBuffer[rxLength++] = c;
      } else {
        rxLength = 0;
      }
    }
  }
}

void publishBlynk() {
  if (!Blynk.connected()) return;

  if (haveData && sensorValid) {
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, airHumidity);
    Blynk.virtualWrite(V6, soilPercent);
  }
  Blynk.virtualWrite(V2, ledState ? 1 : 0);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, ESP_RX, ESP_TX);

  WiFi.mode(WIFI_STA);
  timer.setInterval(4000L, sendConfig);
  timer.setInterval(5000L, publishBlynk);

  Serial.println("ESP32: conexiune Wi-Fi / Blynk");
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);

  sendConfig();
}

void loop() {
  Blynk.run();
  timer.run();
  readArduino();
}
