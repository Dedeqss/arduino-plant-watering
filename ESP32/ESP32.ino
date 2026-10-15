// Configureaza direct aici conexiunea la hotspot si Blynk.
// NU publica parola sau tokenul real intr-un repository public.
#define BLYNK_TEMPLATE_ID "TMPL47bG6YtpW"
#define BLYNK_TEMPLATE_NAME "led si releu"
#define BLYNK_AUTH_TOKEN "INLOCUIESTE_CU_TOKENUL_BLYNK"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <stdio.h>
#include <string.h>

char ssid[] = "Samsung S25";
char pass[] = "INLOCUIESTE_CU_PAROLA_WIFI";

// ESP32 UART2: TX2(GPIO17) -> UNO D10 direct
// ESP32 RX2(GPIO16) <- UNO D11 PRIN divizor rezistiv 5V->3.3V

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

// Diagnostice + reconectare fara blocare permanenta in Blynk.begin().
unsigned long lastWifiStatus = 0;
unsigned long lastWifiRetry = 0;
unsigned long lastBlynkRetry = 0;
bool wifiWasConnected = false;

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

void maintainConnection() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (wifiWasConnected) {
      Serial.println("Wi-Fi deconectat!");
      wifiWasConnected = false;
    }

    if (now - lastWifiStatus >= 5000UL) {
      lastWifiStatus = now;
      Serial.print("Astept Wi-Fi ... status=");
      Serial.println((int)WiFi.status());
      Serial.println("Verifica parola, hotspot pornit, banda 2.4 GHz, WPA2.");
    }

    // Reincearca daca hotspot-ul nu este disponibil la pornire.
    if (now - lastWifiRetry >= 15000UL) {
      lastWifiRetry = now;
      Serial.println("Reincerc conectarea la hotspot...");
      WiFi.reconnect();
    }

    return;
  }

  if (!wifiWasConnected) {
    wifiWasConnected = true;
    Serial.print("Wi-Fi conectat! IP: ");
    Serial.println(WiFi.localIP());
    lastBlynkRetry = 0; // Incercam Blynk imediat.
  }

  if (!Blynk.connected() &&
      (lastBlynkRetry == 0 || now - lastBlynkRetry >= 7000UL)) {
    lastBlynkRetry = now;
    Serial.println("Wi-Fi OK. Incerc conectarea la Blynk...");

    // 2 secunde maxim per incercare, nu blocheaza restul programului.
    if (!Blynk.connect(2000)) {
      Serial.println("Blynk nu se conecteaza: verifica tokenul si internetul.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, ESP_RX, ESP_TX);

  // Configuram Blynk fara sa blocam pornirea cand Wi-Fi nu merge.
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  Blynk.config(BLYNK_AUTH_TOKEN);

  timer.setInterval(4000L, sendConfig);
  timer.setInterval(5000L, publishBlynk);

  Serial.print("ESP32: conectare hotspot ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  sendConfig();
}

void loop() {
  maintainConnection();

  if (Blynk.connected()) {
    Blynk.run();
  }

  timer.run();
  readArduino();
}
