/*
  Udator automat - Arduino UNO
  Senzor: VCC -> 5V, GND -> GND, AOUT -> A0.
  Releu: VCC -> 5V, GND -> GND, IN -> D7.
  Pompa: +5V sursa separata -> COM; NO -> +pompa;
         -pompa -> minusul sursei separate. NC ramane liber.
  Serial Monitor 9600 baud: h = ajutor.
  Calibrarea si udarea sunt in acest singur fisier.
*/
#include <EEPROM.h>

const byte SENSOR_PIN = A0;
const byte RELAY_PIN = 7;
const bool RELAY_ACTIVE_LOW = true; // false pentru releu activ HIGH
const int START_WATERING = 35;
const int STOP_WATERING = 55;
const unsigned long MAX_PUMP_MS = 10000UL;
const unsigned long SOAK_MS = 60000UL; // apa are timp sa se distribuie
const unsigned long SAMPLE_MS = 250UL;
const unsigned long LOG_MS = 1000UL;
const uint16_t MAGIC = 0x5732;

struct Calibration {
  uint16_t magic;
  int16_t dry;
  int16_t wet;
};
Calibration calibration = {0, 750, 350};
bool dryCaptured = false;
bool wetCaptured = false;
bool automatic = false;
bool pumpRunning = false;
bool timeoutLocked = false;
bool sensorFault = false;
int rawValue = 0;
int moisture = 0;
unsigned long pumpStarted = 0;
unsigned long lastStopped = 0;
unsigned long lastSample = 0;
unsigned long lastLog = 0;

void setPump(bool on) {
  digitalWrite(RELAY_PIN, on
    ? (RELAY_ACTIVE_LOW ? LOW : HIGH)
    : (RELAY_ACTIVE_LOW ? HIGH : LOW));
  if (pumpRunning && !on) lastStopped = millis();
  pumpRunning = on;
  if (on) pumpStarted = millis();
}

int readSensor() {
  long total = 0;
  for (byte i = 0; i < 16; i++) total += analogRead(SENSOR_PIN);
  return (total + 8) / 16;
}

bool validCalibration() {
  return dryCaptured && wetCaptured &&
    calibration.dry > 5 && calibration.dry < 1018 &&
    calibration.wet > 5 && calibration.wet < 1018 &&
    abs(calibration.dry - calibration.wet) >= 50;
}

void refreshSensor() {
  rawValue = readSensor();
  // Detecteaza doar valori aproape de limite; nu toate defectele de senzor.
  sensorFault = rawValue <= 5 || rawValue >= 1018;
  if (validCalibration()) {
    moisture = constrain(map(rawValue, calibration.dry,
      calibration.wet, 0, 100), 0L, 100L);
  }
}

void printHelp() {
  Serial.println(F("=== UDATOR AUTOMAT ==="));
  Serial.println(F("h = ajutor; c = mod calibrare (pompa oprita)"));
  Serial.println(F("u = memoreaza RAW in pamant uscat"));
  Serial.println(F("w = memoreaza RAW in pamant foarte umed"));
  Serial.println(F("s = salveaza calibrarea in EEPROM"));
  Serial.println(F("a = porneste modul automat cu calibrare valida"));
  Serial.println(F("o = opreste modul automat si pompa"));
  Serial.println(F("r = deblocheaza dupa limita de timp (pompa oprita)"));
  Serial.println(F("Calibreaza cu pompa deconectata; reconecteaza apoi."));
}

void handleCommand(char command) {
  switch (command) {
    case 'h': printHelp(); break;
    case 'c':
      automatic = false;
      setPump(false);
      dryCaptured = false;
      wetCaptured = false;
      Serial.println(F("Calibrare noua. Pune senzorul in sol uscat, trimite u."));
      break;
    case 'u':
    case 'w':
      automatic = false;
      setPump(false);
      refreshSensor();
      if (sensorFault) {
        Serial.println(F("RAW la limita. Verifica firele; captura refuzata."));
        break;
      }
      if (command == 'u') {
        calibration.dry = rawValue;
        dryCaptured = true;
        Serial.print(F("USCAT = "));
      } else {
        calibration.wet = rawValue;
        wetCaptured = true;
        Serial.print(F("UMED = "));
      }
      Serial.println(rawValue);
      break;
    case 's':
      if (validCalibration()) {
        calibration.magic = MAGIC;
        EEPROM.put(0, calibration);
        Serial.println(F("Calibrare salvata. Trimite a pentru udare."));
      } else Serial.println(F("Captureaza u si w; diferenta minima 50 RAW."));
      break;
    case 'a':
      refreshSensor();
      if (!validCalibration() || sensorFault || timeoutLocked) {
        Serial.println(F("Pornire refuzata: verifica calibrarea, senzorul sau blocarea."));
      } else {
        automatic = true;
        Serial.println(F("Mod automat activ."));
      }
      break;
    case 'o':
      automatic = false;
      setPump(false);
      Serial.println(F("Mod automat oprit."));
      break;
    case 'r':
      setPump(false);
      automatic = false;
      timeoutLocked = false;
      lastStopped = millis();
      Serial.println(F("Deblocat. Verifica apa si furtunul, apoi trimite a."));
      break;
    default: break; // ignora Enter, CR/LF si alte caractere
  }
}

void setup() {
  // Pregateste nivelul OFF inainte de configurarea pinului ca iesire.
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  pinMode(RELAY_PIN, OUTPUT);
  Serial.begin(9600);
  EEPROM.get(0, calibration);
  dryCaptured = wetCaptured = true;
  if (calibration.magic != MAGIC || !validCalibration()) {
    calibration.magic = 0;
    calibration.dry = 750;
    calibration.wet = 350;
    dryCaptured = wetCaptured = false;
    Serial.println(F("Fara calibrare salvata. Pompa ramane oprita."));
  } else {
    automatic = true;
    Serial.println(F("Calibrare incarcata; modul automat porneste dupa 60 secunde."));
  }
  lastStopped = millis();
  refreshSensor();
  printHelp();
}

void loop() {
  unsigned long now = millis();
  // Limita este verificata la fiecare trecere, fara delay de o secunda.
  if (pumpRunning && now - pumpStarted >= MAX_PUMP_MS) {
    setPump(false);
    automatic = false;
    timeoutLocked = true;
    Serial.println(F("LIMITA 10s: udare blocata. Verifica montajul; r apoi a."));
  }
  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;
    refreshSensor();
    if (sensorFault) {
      setPump(false);
      automatic = false;
    }
  }
  while (Serial.available() > 0) handleCommand(Serial.read());
  now = millis();
  if (automatic && validCalibration() && !sensorFault && !timeoutLocked) {
    if (pumpRunning && moisture >= STOP_WATERING) {
      setPump(false);
      Serial.println(F("Sol suficient de umed. Pauza 60 secunde."));
    } else if (!pumpRunning && moisture <= START_WATERING &&
               now - lastStopped >= SOAK_MS) {
      setPump(true);
      Serial.println(F("Sol uscat. Pompa pornita."));
    }
  }
  if (now - lastLog >= LOG_MS) {
    lastLog = now;
    Serial.print(F("RAW: ")); Serial.print(rawValue);
    Serial.print(F(" | Umiditate relativa: "));
    if (validCalibration()) Serial.print(moisture);
    else Serial.print(F("?"));
    Serial.print(F("% | Pompa: "));
    Serial.print(pumpRunning ? F("PORNITA") : F("OPRITA"));
    Serial.print(F(" | Mod: "));
    Serial.print(automatic ? F("AUTOMAT") : F("OPRIT/CALIBRARE"));
    if (timeoutLocked) Serial.print(F(" | BLOCAT 10s"));
    if (sensorFault) Serial.print(F(" | EROARE SENZOR"));
    Serial.println();
  }
}

