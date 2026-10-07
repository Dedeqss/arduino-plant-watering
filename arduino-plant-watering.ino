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
const unsigned long MAX_PUMP_MS = 10000UL; // limita absoluta de protectie
const unsigned long WATER_PULSE_MS = 2000UL; // releul porneste 2s, nu PWM
const unsigned long MANUAL_PULSE_MS = 2000UL;
const byte MAX_AUTO_PULSES = 3;
const unsigned long DRY_CONFIRM_MS = 3000UL;
const unsigned long RELAY_REST_MS = 500UL;
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
bool manualMode = false;
byte autoPulses = 0;
byte badSamples = 0;
byte goodSamples = 0;
bool dryTiming = false;
unsigned long drySince = 0;
int pendingDry = 0;
int sensorHistory[8];
byte historyCount = 0;
byte historyIndex = 0;
int filteredRaw = 0;
bool filterReady = false;

void setPump(bool on) {
  digitalWrite(RELAY_PIN, on
    ? (RELAY_ACTIVE_LOW ? LOW : HIGH)
    : (RELAY_ACTIVE_LOW ? HIGH : LOW));
  if (pumpRunning && !on) lastStopped = millis();
  pumpRunning = on;
  if (on) pumpStarted = millis();
}

int readSensor() {
  int values[9];
  analogRead(SENSOR_PIN); // prima conversie se arunca
  for (byte i = 0; i < 9; i++) {
    values[i] = analogRead(SENSOR_PIN);
    for (byte j = i; j > 0 && values[j] < values[j - 1]; j--) {
      int temp = values[j]; values[j] = values[j - 1]; values[j - 1] = temp;
    }
  }
  return values[4]; // mediana ignora cateva varfuri izolate
}

bool readingsValid(int dry, int wet) {
  return dry > 5 && dry < 1018 && wet > 5 && wet < 1018 &&
    abs(dry - wet) >= 50;
}

bool validCalibration() {
  return dryCaptured && wetCaptured &&
    readingsValid(calibration.dry, calibration.wet);
}

void refreshSensor() {
  rawValue = readSensor();
  sensorHistory[historyIndex] = rawValue;
  historyIndex = (historyIndex + 1) % 8;
  if (historyCount < 8) historyCount++;
  if (rawValue <= 5 || rawValue >= 1018) {
    goodSamples = 0;
    if (badSamples < 3) badSamples++;
    if (badSamples >= 3) sensorFault = true;
  } else {
    badSamples = 0;
    if (goodSamples < 3) goodSamples++;
    if (goodSamples >= 3) sensorFault = false;
    if (!filterReady) { filteredRaw = rawValue; filterReady = true; }
    else filteredRaw = (3L * filteredRaw + rawValue + 2) / 4;
  }
  if (validCalibration() && filterReady) {
    moisture = constrain(map(filteredRaw, calibration.dry,
      calibration.wet, 0, 100), 0L, 100L);
  }
}

bool captureStable(int &value) {
  if (historyCount < 8) return false;
  int low = 1023, high = 0;
  long total = 0;
  for (byte i = 0; i < 8; i++) {
    int v = sensorHistory[i];
    if (v < low) low = v;
    if (v > high) high = v;
    total += v;
  }
  value = (total + 4) / 8;
  return low > 5 && high < 1018 && high - low <= 30;
}

void printHelp() {
  Serial.println(F("=== UDATOR AUTOMAT ==="));
  Serial.println(F("h = ajutor; c = calibrare ghidata automata"));
  Serial.println(F("Inainte de c: senzorul in sol uscat, pompa deconectata."));
  Serial.println(F("Dupa 10s: muta senzorul in sol foarte umed in 20s."));
  Serial.println(F("Calibrarea se salveaza. Trimite a cand montajul este pregatit."));
  Serial.println(F("1 = test manual 2 secunde; 0 = oprire imediata"));
  Serial.println(F("0 = opreste pompa ACUM si opreste modul automat"));
  Serial.println(F("a = porneste modul automat cu calibrare valida"));
  Serial.println(F("o = opreste modul automat si pompa; v = afiseaza starea"));
  Serial.println(F("r = deblocheaza dupa limita de timp (pompa oprita)"));
  Serial.println(F("Calibreaza cu pompa deconectata; reconecteaza apoi."));
}

void processCalibration(unsigned long now) {
  if (calibrationStep == WAIT_DRY && now - stepStarted >= DRY_CAPTURE_MS) {
    if (!captureStable(pendingDry)) {
      calibrationStep = IDLE;
      Serial.println(F("USCAT instabil/la limita. Pompa oprita. Reincearca c."));
      return;
    }
    calibrationStep = WAIT_WET;
    stepStarted = now;
    historyCount = 0;
    Serial.print(F("USCAT RAW = ")); Serial.println(pendingDry);
    Serial.println(F("Muta senzorul in sol foarte umed; ai 20 secunde."));
  } else if (calibrationStep == WAIT_WET &&
             now - stepStarted >= WET_CAPTURE_MS) {
    int pendingWet = 0;
    calibrationStep = IDLE;
    if (!captureStable(pendingWet) || !readingsValid(pendingDry, pendingWet)) {
      Serial.println(F("Calibrare esuata/instabila. Cea salvata anterior ramane intacta."));
      return;
    }
    calibration.magic = MAGIC;
    calibration.dry = pendingDry;
    calibration.wet = pendingWet;
    dryCaptured = wetCaptured = true;
    EEPROM.put(0, calibration);
    filterReady = false;
    refreshSensor();
    Serial.print(F("UMED RAW = ")); Serial.println(pendingWet);
    Serial.println(F("Calibrare salvata. Pune senzorul in ghiveci, apoi trimite a."));
  }
}

void printStatus() {
  Serial.print(F("RAW: ")); Serial.print(rawValue);
  Serial.print(F(" | Filtrat: ")); Serial.print(filteredRaw);
  Serial.print(F(" | Umiditate: "));
  if (validCalibration() && filterReady) Serial.print(moisture);
  else Serial.print(F("?"));
  Serial.print(F("% | Pompa: "));
  Serial.print(pumpRunning ? F("PORNITA") : F("OPRITA"));
  Serial.print(F(" | Mod: "));
  Serial.print(calibrationStep != IDLE ? F("CALIBRARE") :
    (automatic ? F("AUTOMAT") : (manualMode ? F("MANUAL") : F("OPRIT"))));
  Serial.print(F(" | Pulsuri: ")); Serial.print(autoPulses);
  if (timeoutLocked) Serial.print(F(" | BLOCAT"));
  if (sensorFault) Serial.print(F(" | EROARE SENZOR"));
  Serial.println();
}

void handleCommand(char command) {
  switch (command) {
    case 'h': printHelp(); break;
    case 'v': printStatus(); break;
    case '1':
      if (calibrationStep != IDLE) {
        Serial.println(F("Test refuzat in calibrare. Trimite 0 pentru anulare."));
        break;
      }
      if (pumpRunning) break; // nu prelungeste si nu schimba modul in timpul udarii
      if (millis() - lastStopped < RELAY_REST_MS) {
        Serial.println(F("Asteapta 0.5s inainte de alt test."));
        break;
      }
      if (timeoutLocked) {
        Serial.println(F("Pompa blocata. Verifica apa/furtunul, apoi r."));
        break;
      }
      automatic = false;
      manualMode = true;
      calibrationStep = IDLE;
      // Daca merge deja, nu reseta cronometrul limitei de 10 secunde.
      if (!pumpRunning) setPump(true);
      Serial.println(F("Test manual PORNIT 2 secunde. 0 opreste imediat."));
      break;
    case '0':
      automatic = false;
      manualMode = false;
      dryTiming = false;
      calibrationStep = IDLE;
      setPump(false);
      Serial.println(F("Pompa OPRITA manual. Trimite a pentru mod automat."));
      break;
    case 'c':
      automatic = false;
      setPump(false);
      manualMode = false;
      historyCount = 0;
      calibrationStep = WAIT_DRY;
      stepStarted = millis();
      Serial.println(F("Calibrare: tine senzorul in sol USCAT 10 secunde."));
      break;
    case 'a':
      refreshSensor();
      if (calibrationStep != IDLE || !validCalibration() || sensorFault || goodSamples < 3 || timeoutLocked) {
        Serial.println(F("Pornire refuzata: verifica calibrarea, senzorul sau blocarea."));
      } else {
        setPump(false);
        manualMode = false;
        autoPulses = 0;
        dryTiming = false;
        lastStopped = millis();
        automatic = true;
        Serial.println(F("Mod automat activ. Prima udare dupa minimum 60s."));
      }
      break;
    case 'o':
      automatic = false;
      manualMode = false;
      dryTiming = false;
      setPump(false);
      calibrationStep = IDLE;
      Serial.println(F("Mod automat oprit."));
      break;
    case 'r':
      if (calibrationStep != IDLE) { Serial.println(F("Anuleaza calibrarea cu 0.")); break; }
      setPump(false);
      manualMode = false;
      autoPulses = 0;
      dryTiming = false;
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
  Serial.println(F("BOOT v2 - pompa oprita. Daca mesajul reapare singur: verifica alimentarea."));
  EEPROM.get(0, calibration);
  dryCaptured = wetCaptured = true;
  if (calibration.magic != MAGIC || !validCalibration()) {
    calibration.magic = 0;
    calibration.dry = 750;
    calibration.wet = 350;
    dryCaptured = wetCaptured = false;
    Serial.println(F("Fara calibrare salvata. Pompa ramane oprita."));
  } else {
    automatic = false;
    Serial.println(F("Calibrare incarcata. Pompa ramane OPRITA; trimite a sau 1."));
  }
  lastStopped = millis();
  refreshSensor();
  printHelp();
}

void loop() {
  // Citeste un numar limitat de caractere ca sa nu amane controlul pompei.
  for (byte i = 0; i < 8 && Serial.available() > 0; i++) {
    char command = Serial.read();
    if (command >= 'A' && command <= 'Z') command += 'a' - 'A';
    handleCommand(command);
  }
  unsigned long now = millis();
  if (pumpRunning && now - pumpStarted >= MAX_PUMP_MS) {
    setPump(false);
    automatic = manualMode = false;
    timeoutLocked = true;
    Serial.println(F("LIMITA ABSOLUTA 10s: BLOCAT. Verifica, apoi r."));
  } else if (pumpRunning && now - pumpStarted >=
             (manualMode ? MANUAL_PULSE_MS : WATER_PULSE_MS)) {
    setPump(false);
    if (manualMode) {
      manualMode = false;
      Serial.println(F("Test manual terminat (2s). Pentru alt test: 1."));
    } else Serial.println(F("Puls terminat (2s). Pauza 60s pentru absorbtie."));
  }

  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;
    refreshSensor();
    if (sensorFault && automatic) {
      setPump(false);
      automatic = false;
      dryTiming = false;
      Serial.println(F("Senzor la limita 3 citiri. Automat oprit; verifica firele."));
    }
  }
  processCalibration(now);
  if (automatic && validCalibration() && !sensorFault && !timeoutLocked) {
    if (moisture >= STOP_WATERING) {
      if (pumpRunning) setPump(false);
      autoPulses = 0;
      dryTiming = false;
    } else if (moisture <= START_WATERING) {
      if (!dryTiming) { dryTiming = true; drySince = now; }
      if (!pumpRunning && now - lastStopped >= SOAK_MS &&
          now - drySince >= DRY_CONFIRM_MS) {
        if (autoPulses >= MAX_AUTO_PULSES) {
          automatic = false;
          timeoutLocked = true;
          Serial.println(F("BLOCAT: sol inca uscat dupa 3 pulsuri. Verifica apa/senzorul; r."));
        } else {
          autoPulses++;
          setPump(true);
          Serial.println(F("Sol uscat stabil: puls udare 2s."));
        }
      }
    } else {
      dryTiming = false;
      if (!pumpRunning && now - lastStopped >= SOAK_MS) autoPulses = 0;
    }
  }
  if (now - lastLog >= LOG_MS) {
    lastLog = now;
    printStatus();
  }
}
