#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <string.h>
#include <stdio.h>

// Arduino UNO: DHT11 + senzor de sol YL-69 + LED
// Legaturi: DHT11 S=D2; YL-69 AO=A0; LED D8 prin 220 ohmi;
// ESP32 TX2(GPIO17) -> UNO D10;
// UNO D11 -> divizor 1k/2k -> ESP32 RX2(GPIO16); GND comun.

#define DHT_PIN 2
#define DHT_TYPE DHT11
#define SOIL_PIN A0
#define LED_PIN 8

// Inlocuieste cu masuratorile reale din Serial Monitor.
// DRY = sonda uscata, WET = sonda in sol bine umezit.
const int SOIL_DRY_RAW = 850;
const int SOIL_WET_RAW = 350;

const unsigned long READ_INTERVAL_MS = 2000UL;
SoftwareSerial esp32(10, 11);  // RX, TX
DHT dht(DHT_PIN, DHT_TYPE);

bool automaticMode = true;
bool manualLED = false;
bool ledState = false;
bool sensorValid = false;
float temperature = 0.0f;
float airHumidity = 0.0f;
int soilRaw = 0;
int soilPercent = 0;
int thresholdC = 24;

unsigned long lastRead = 0;
char rxBuffer[48];
size_t rxLength = 0;

void applyLED() {
  ledState = automaticMode
    ? (sensorValid && temperature >= thresholdC)
    : manualLED;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
}

int measureSoilRaw() {
  long sum = 0;
  for (byte i = 0; i < 10; i++) {
    sum += analogRead(SOIL_PIN);
    delay(3);
  }
  return (int)(sum / 10);
}

int toSoilPercent(int raw) {
  if (SOIL_DRY_RAW == SOIL_WET_RAW) return 0;
  long result = map(raw, SOIL_DRY_RAW, SOIL_WET_RAW, 0, 100);
  return (int)constrain(result, 0L, 100L);
}

void handleCommand(const char* line) {
  int mode, manual, limitC;
  if (sscanf(line, "CFG,%d,%d,%d", &mode, &manual, &limitC) != 3) return;
  if ((mode != 0 && mode != 1) ||
      (manual != 0 && manual != 1) ||
      limitC < 15 || limitC > 40) {
    Serial.println(F("Configurare respinsa"));
    return;
  }

  automaticMode = (mode == 1);
  manualLED = (manual == 1);
  thresholdC = limitC;
  applyLED();

  Serial.print(F("Config: "));
  Serial.print(automaticMode ? F("AUTO") : F("MANUAL"));
  Serial.print(F(", prag="));
  Serial.print(thresholdC);
  Serial.print(F(" C, LED="));
  Serial.println(ledState ? F("ON") : F("OFF"));
}

void readSerialCommands() {
  while (esp32.available()) {
    char c = (char)esp32.read();
    if (c == '\n') {
      rxBuffer[rxLength] = '\0';
      if (rxLength) handleCommand(rxBuffer);
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

void setup() {
  Serial.begin(115200);
  esp32.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  dht.begin();
  lastRead = millis();
  Serial.println(F("Arduino UNO: DHT11 + soil + LED + ESP32"));
  Serial.println(F("Prag initial: 24 C, mod AUTO"));
}

void loop() {
  readSerialCommands();

  if (millis() - lastRead < READ_INTERVAL_MS) return;
  lastRead = millis();

  soilRaw = measureSoilRaw();
  soilPercent = toSoilPercent(soilRaw);

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    sensorValid = false;
    applyLED();  // In AUTO LED se stinge la eroare DHT11.
    Serial.println(F("EROARE DHT11"));
    esp32.print(F("ERROR,"));
    esp32.println(ledState ? 1 : 0);
    return;
  }

  sensorValid = true;
  temperature = t;
  airHumidity = h;
  applyLED();

  // Format: DATA,temp,umiditate_aer,led,mod_auto,sol_procent,sol_raw
  esp32.print(F("DATA,"));
  esp32.print(temperature, 1);
  esp32.print(',');
  esp32.print(airHumidity, 1);
  esp32.print(',');
  esp32.print(ledState ? 1 : 0);
  esp32.print(',');
  esp32.print(automaticMode ? 1 : 0);
  esp32.print(',');
  esp32.print(soilPercent);
  esp32.print(',');
  esp32.println(soilRaw);

  Serial.print(F("Temperatura: "));
  Serial.print(temperature, 1);
  Serial.print(F(" C | Umiditate aer: "));
  Serial.print(airHumidity, 1);
  Serial.print(F("% | Umiditate sol: "));
  Serial.print(soilPercent);
  Serial.print(F("% | Soil RAW: "));
  Serial.print(soilRaw);
  Serial.print(F(" | Prag: "));
  Serial.print(thresholdC);
  Serial.print(F(" C | Mod: "));
  Serial.print(automaticMode ? F("AUTO") : F("MANUAL"));
  Serial.print(F(" | LED: "));
  Serial.println(ledState ? F("ON") : F("OFF"));
}
