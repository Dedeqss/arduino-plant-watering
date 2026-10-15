# Stație meteo și umiditate sol – Arduino UNO + ESP32 + Blynk

Proiect școlar: Arduino UNO citește **DHT11** (temperatură și umiditate aer) și un **senzor rezistiv de umiditate a solului YL-69/YL-38** (AO). ESP32 transmite valorile la Blynk prin Wi-Fi. Un LED se aprinde automat la temperatura configurată sau manual din aplicație. **Nu folosește releu sau pompă.**

## Programe separate

- [Arduino UNO – Arduino_UNO.ino](Arduino_UNO/Arduino_UNO.ino)
- [ESP32 – ESP32.ino](ESP32/ESP32.ino)
- [Exemplu configurație ESP32](ESP32/secrets.example.h)

**IMPORTANT:** Nu pune cele două fișiere `.ino` în același sketch Arduino IDE. Fiecare placă are propriul program, încărcat prin USB.

## Conexiuni

| Dispozitiv | Arduino UNO |
|---|---|
| DHT11 S / DATA | D2 |
| DHT11 + / VCC | 5V |
| DHT11 − / GND | GND |
| Senzor sol AO | A0, ANALOG IN |
| Senzor sol DO | Neconectat |
| Senzor sol VCC | 5V |
| Senzor sol GND | GND |
| LED anod (+) | D8 prin rezistor de 220 Ω |
| LED catod (−) | GND |
| ESP32 TX2 / GPIO17 | D10 direct |
| ESP32 RX2 / GPIO16 | D11 **prin divizor 5V → 3.3V** |
| ESP32 GND | UNO GND |

Divizorul dintre Arduino D11 și ESP32 GPIO16 folosește trei rezistoare de **1 kΩ**:

```text
Arduino D11 (5V TX)
       |
      1k
       |
       +----------------- ESP32 GPIO16 (RX2)
       |
      1k
       |
      1k
       |
   GND comun
```

**Nu conecta direct D11 (5V) la GPIO16 (3.3V)**. GND Arduino și GND ESP32 trebuie să fie comune. Conectează/deconectează firele cu alimentarea oprită. Pentru primele teste alimentează fiecare placă separat prin USB; bateria dreptunghiulară de 9V nu este potrivită pentru alimentarea ambelor plăci în timpul conexiunilor Wi-Fi. Senzorul de sol: conectează sonda metalică la modulul cu AO/DO și păstrează placa electronică uscată.

## Instalare – Arduino IDE

### 1) Arduino UNO

1. Instalează din Library Manager **DHT sensor library** (Adafruit) și **Adafruit Unified Sensor**.
2. Deschide `Arduino_UNO/Arduino_UNO.ino`, selectează **Arduino Uno** și portul său.
3. Încarcă folosind Upload. Deschide Serial Monitor la **115200 baud**.

### 2) ESP32

1. Instalează pachetul de plăci **esp32** (Espressif Systems) și biblioteca **Blynk**.
2. Deschide `ESP32/ESP32.ino`; selectează **ESP32 Dev Module**.
3. În același folder, **copiază** `secrets.example.h` ca `secrets.h` și completează local `BLYNK_AUTH_TOKEN`, `WIFI_SSID`, `WIFI_PASS`. Nu modifica numele `BLYNK_TEMPLATE_ID` decât dacă ai creat alt template.
4. Încarcă folosind Upload și deschide Serial Monitor la **115200 baud**.
5. Hotspot-ul Samsung trebuie să funcționeze pe banda **2.4 GHz** și să ofere internet.

Fișierul `ESP32/secrets.h` este ignorat de Git (`.gitignore`) și nu trebuie publicat. Dacă tokenul sau parola au fost expuse anterior, regenerează tokenul Blynk și schimbă parola.

## Setările Blynk IoT

Template-ul folosit: `led si releu` (numele vechi poate rămâne, deși acum se controlează LED-ul).

| Pin virtual | Nume | Tip | Domeniu | Widget |
|---|---|---|---|---|
| V0 | Temperatura | Double | 0–50 °C | Labeled Value / Gauge |
| V1 | Umiditate aer | Double | 0–100 % | Labeled Value |
| V2 | Stare LED | Integer | 0–1 | Indicator LED |
| V3 | Mod automat | Integer | 0–1 | Switch (1 = AUTO) |
| V4 | Control LED | Integer | 0–1 | Switch (numai în MANUAL) |
| V5 | Prag temperatură | Integer | 15–40 °C | Slider |
| V6 | Umiditate sol | Integer | 0–100 % | Gauge / Value |

Configurează datastream-urile în **Developer Zone → My Templates → Datastreams**, apoi leagă widgeturile din dashboardul mobil. Pentru pornirea inițială, pune V3=1, V4=0, V5=24 în aplicație.

- În **AUTO**, LED-ul se aprinde când temperatura DHT11 este **mai mare sau egală** cu pragul V5.
- În **MANUAL**, V4 pornește/oprește LED-ul indiferent de temperatură.
- La eroare DHT11, în AUTO LED-ul se stinge. În MANUAL, comanda manuală rămâne activă.
- ESP32 retransmite configurația către UNO la fiecare 4 secunde și publică valorile în Blynk la 5 secunde.

## Calibrarea senzorului de sol

`Soil RAW` are intervalul 0–1023. Procentul este o **estimare**, nu o măsurătoare directă a conținutului de apă.

1. Ține sonda curată și uscată, citește `Soil RAW` în Serial Monitor UNO și notează valoarea.
2. Introdu doar sonda în **sol bine umezit**; așteaptă stabilizarea și notează `Soil RAW`.
3. În `Arduino_UNO.ino`, modifică `SOIL_DRY_RAW` și `SOIL_WET_RAW` cu valorile observate și încarcă din nou UNO.
4. Compară valorile afișate în Blynk (V6) cu cele din Serial Monitor.

Exemplul 850/350 din cod **nu este calibrare efectuată**. Dacă sonda ta produce valori în sens invers, calibrarea cu punctele reale rezolvă problema. Senzorul rezistiv poate coroda dacă este menținut alimentat și în sol umed timp îndelungat.

## Depanare

- **Blynk Offline:** verifică Wi-Fi-ul de 2.4 GHz, tokenul Blynk și alimentarea ESP32.
- **UNO vede temperatura, dar ESP32 nu:** verifică TX/RX încrucișate, divizorul și GND comun.
- **Umiditate sol 0% sau 100% permanent:** verifică AO → A0, alimentarea și calibrarea.
- **LED nu răspunde:** verifică D8 → rezistor 220Ω → anod LED → catod GND; V3=0 înainte de control manual V4.
- **Temperatura sau umiditatea se opresc:** verifică pinii DHT11 și mesajele `EROARE DHT11` pe UNO.

## Backup vechi

Proiectul anterior de udare cu pompă, releu și documentația veche a fost păstrat pe ramura `backup-original-watering-2026-10-15`, înainte de înlocuirea proiectului de pe `main`.
