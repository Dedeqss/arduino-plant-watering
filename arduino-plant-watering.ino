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
const unsigned long DRY_CAPTURE_MS = 10000UL;
const unsigned long WET_CAPTURE_MS = 20000UL;
const uint16_t MAGIC = 0x5732;

enum CalibrationStep { IDLE, WAIT_DRY, WAIT_WET };
CalibrationStep calibrationStep = IDLE;
unsigned long stepStarted = 0;

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
  Serial.println(F("h = ajutor; c = calibrare ghidata automata"));
  Serial.println(F("Inainte de c: senzorul in sol uscat, pompa deconectata."));
  Serial.println(F("Dupa 10s: muta senzorul in sol foarte umed in 20s."));
  Serial.println(F("Programul salveaza si activeaza automat dupa calibrare."));
  Serial.println(F("1 = porneste pompa ACUM manual (maxim 10 secunde)"));
  Serial.println(F("0 = opreste pompa ACUM si opreste modul automat"));
  Serial.println(F("a = porneste modul automat cu calibrare valida"));
  Serial.println(F("o = opreste modul automat si pompa"));
  Serial.println(F("r = deblocheaza dupa limita de timp (pompa oprita)"));
  Serial.println(F("Calibreaza cu pompa deconectata; reconecteaza apoi."));
}

void processCalibration(unsigned long now) {
  if (calibrationStep == WAIT_DRY && now - stepStarted >= DRY_CAPTURE_MS) {
    calibration.dry = readSensor();
    dryCaptured = true;
    calibrationStep = WAIT_WET;
    stepStarted = now;
    Serial.print(F("USCAT RAW = "));
    Serial.println(calibration.dry);
    Serial.println(F("Acum muta senzorul in sol foarte umed."));
    Serial.println(F("Ai 20 secunde; tine electronica senzorului uscata."));
  } else if (calibrationStep == WAIT_WET &&
             now - stepStarted >= WET_CAPTURE_MS) {
    calibration.wet = readSensor();
    wetCaptured = true;
    calibrationStep = IDLE;
    Serial.print(F("UMED RAW = "));
    Serial.println(calibration.wet);
    if (!validCalibration()) {
      dryCaptured = wetCaptured = false;
      Serial.println(F("Calibrare esuata: valori la limita sau diferenta sub 50. Reincearca c."));
      return;
    }
    calibration.magic = MAGIC;
    EEPROM.put(0, calibration);
    lastStopped = now;
    automatic = !timeoutLocked;
    if (timeoutLocked) {
      Serial.println(F("Calibrare salvata, dar pompa e blocata. Verifica, apoi r si a."));
    } else {
      Serial.println(F("Calibrare salvata. Mod automat activ; asteapta 60 secunde."));
    }
  }
}

void handleCommand(char command) {
  switch (command) {
    case 'h': printHelp(); break;
    case '1':
      if (timeoutLocked) {
        Serial.println(F("Pompa blocata dupa 10s. Verifica apa/furtunul, apoi r."));
        break;
      }
      automatic = false;
      calibrationStep = IDLE;
      // Daca merge deja, nu reseta cronometrul limitei de 10 secunde.
      if (!pumpRunning) setPump(true);
      Serial.println(F("Pompa PORNITA manual, maxim 10 secunde. Trimite 0 pentru oprire."));
      break;
    case '0':
      automatic = false;
      calibrationStep = IDLE;
      setPump(false);
      Serial.println(F("Pompa OPRITA manual. Trimite a pentru mod automat."));
      break;
    case 'c':
      automatic = false;
      setPump(false);
      dryCaptured = false;
      wetCaptured = false;
      calibrationStep = WAIT_DRY;
      stepStarted = millis();
      Serial.println(F("Calibrare: tine senzorul in sol USCAT 10 secunde."));
      break;
    case 'a':
      refreshSensor();
      if (calibrationStep != IDLE || !validCalibration() || sensorFault || timeoutLocked) {
        Serial.println(F("Pornire refuzata: verifica calibrarea, senzorul sau blocarea."));
      } else {
        automatic = true;
        Serial.println(F("Mod automat activ."));
      }
      break;
    case 'o':
      automatic = false;
      setPump(false);
      calibrationStep = IDLE;
      Serial.println(F("Mod automat oprit."));
      break;
    case 'r':
      setPump(false);
      automatic = false;
      timeoutLocked = false;
      lastStopped = millis();
      Serial.println(F("Deblocat. Trimite a pentru automat sau 1 pentru test manual."));
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
    Serial.println(F("LIMITA 10s: pompa blocata. Verifica montajul; r apoi a sau 1."));
  }
  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;
    refreshSensor();
    if (sensorFault && automatic) {
      setPump(false);
      automatic = false;
    }
  }
  while (Serial.available() > 0) handleCommand(Serial.read());
  now = millis();
  processCalibration(now);
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
    Serial.print(calibrationStep != IDLE ? F("CALIBRARE") :
      (automatic ? F("AUTOMAT") : F("OPRIT")));
    if (timeoutLocked) Serial.print(F(" | BLOCAT 10s"));
    if (sensorFault) Serial.print(F(" | EROARE SENZOR"));
    Serial.println();
  }
}
