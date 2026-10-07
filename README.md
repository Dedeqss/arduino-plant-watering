# Udător automat pentru plante cu Arduino UNO — v2.1

Un proiect pentru școală care citește umiditatea solului și comandă o pompă de apă printr-un releu. **Tot codul, inclusiv calibrarea, se află într-un singur fișier:** [arduino-plant-watering.ino](arduino-plant-watering.ino).

Versiunea v2.1 filtrează citirile senzorului și udă în pulsuri de 2 secunde. Calibrarea se salvează în EEPROM, dar după pornire sau reset pompa rămâne oprită până trimiți o comandă. Nu trebuie să instalezi biblioteci suplimentare: EEPROM este inclusă în pachetul Arduino AVR.

## Schema montajului

![Schema trimisă pentru udătorul automat](images/schema-udator-plante.jpg)

Imaginea este schema trimisă pentru proiect, salvată în format JPG pentru a se încărca mai repede. **Pentru montaj, urmează etichetele pinilor și tabelul de mai jos**, nu poziția desenată a conectorilor. Firul VCC al releului trebuie legat explicit la 5V Arduino, chiar dacă traseul său nu este complet clar în imagine.

## Componente necesare

| Componentă | Rol |
|---|---|
| Arduino UNO și cablu USB | Citește senzorul și comandă releul |
| Senzor capacitiv de umiditate a solului, compatibil cu 5V | Oferă semnalul analogic AOUT |
| Modul releu 5V, un canal | Comută alimentarea pompei |
| Pompă DC de 5V | Transportă apa către plantă |
| Sursă separată stabilizată 5V | Alimentează pompa; curentul trebuie să acopere și pornirea pompei |
| Furtun potrivit, rezervor de apă și fire | Completează montajul |

Sursa din imagine este de minimum 1A, dar verifică necesarul pompei tale. Arduino poate fi alimentat prin USB. Folosește un **modul de releu**, care are circuitul de comandă inclus, nu un releu simplu conectat direct la D7.

## Cum legi firele

Fă legăturile cu alimentările deconectate.

| De la | La |
|---|---|
| Senzor VCC | 5V Arduino |
| Senzor GND | GND Arduino |
| Senzor AOUT / AO | A0 Arduino |
| Releu VCC | 5V Arduino |
| Releu GND | GND Arduino |
| Releu IN | D7 Arduino |
| Plusul sursei separate de 5V | COM releu |
| NO releu | Firul pozitiv al pompei |
| Firul negativ al pompei | Minusul sursei separate |
| NC releu | Rămâne neconectat |

1. Leagă senzorul la 5V, GND și A0. AOUT este ieșirea analogică.
2. Leagă partea de comandă a releului la 5V, GND și D7. Senzorul și releul folosesc masa Arduino.
3. Leagă plusul sursei pompei la COM, apoi NO la plusul pompei.
4. Leagă minusul pompei direct la minusul sursei sale.
5. Conectează furtunul la ieșirea indicată de producătorul pompei. Respectă tipul acesteia: o pompă submersibilă se folosește conform instrucțiunilor sale și nu se pornește fără apă.

**COM** este contactul comun. **NO** este deschis în repaus, astfel încât pompa rămâne oprită când releul nu este activat. **NC** este închis în repaus și nu se folosește aici.

Contactele releului separă circuitul pompei de circuitul de comandă; pentru acest montaj cu releu, minusul sursei pompei nu trebuie conectat la GND Arduino. Nu conecta plusul sursei pompei la 5V Arduino și nu alimenta pompa dintr-un pin al plăcii.

Pentru o pompă DC cu motor cu perii, o diodă de protecție dimensionată pentru motor se montează în paralel cu pompa, cu catodul (capătul cu bandă) la plus și anodul la minus. Dioda modulului de releu protejează bobina releului, nu motorul pompei.

## Cum încarci noua versiune

1. Descarcă proiectul cu **Code → Download ZIP**, dezarhivează-l și redenumește folderul sketch-ului în **arduino-plant-watering**, ca fișierul `.ino`.
2. Deconectează alimentarea separată a pompei pentru încărcare și verificări.
3. Deschide **arduino-plant-watering.ino**, alege **Arduino Uno** și portul corect, apoi **Verify → Upload**.
4. Deschide **Serial Monitor la 9600 baud**.
5. Trebuie să apară mesajul **BOOT v2.1 - pompa oprita**. Dacă apare alt mesaj vechi, noul program nu este încă pe placă.

**O actualizare în GitHub nu actualizează placa. Trebuie să faci Upload din nou.**

## Ce s-a schimbat și de ce

- La fiecare pornire/reset, pompa rămâne oprită, inclusiv când există calibrare salvată. Nu mai reia singură udarea după o resetare provocată de alimentare. Trimiți `a` când montajul este pregătit.
- Modurile manual și automat sunt separate. Comanda `a` oprește un test manual înainte să activeze automatul și începe o nouă pauză.
- Testul manual și fiecare udare automată durează **aproximativ 2 secunde**; oprirea se verifică în bucla programului. Sunt udări mai scurte; nu înseamnă că motorul merge mai încet.
- În automat, se așteaptă **60 de secunde** între pulsuri pentru absorbția apei.
- Citirile au filtru median și netezire; solul trebuie să rămână sub prag timp de **3 secunde** ca să pornească udarea.
- Trei citiri consecutive aproape de limitele senzorului opresc automatul. O singură citire izolată nu mai schimbă imediat starea.
- După 3 pulsuri, dacă solul este încă la 35% sau mai puțin după pauză, sistemul se blochează pentru verificare. Contorul se resetează când solul ajunge la 55%, sau când, după pauza de absorbție, este peste pragul de pornire.
- Timpii sunt recalculați după oprirea releului, pentru a evita o repornire imediată provocată de scăderea unui timp mai nou dintr-un timp memorat mai vechi.
- Afișarea ajutorului/stării este amânată când motorul merge. Astfel, un șir de comenzi de afișare nu prelungește udarea prin blocarea comunicației seriale.
- Calibrarea nouă are CRC16 și este publicată în EEPROM numai după scrierea datelor. O înregistrare nouă invalidă este refuzată. Valorile valide din versiunea veche sunt migrate automat; o întrerupere în timpul salvării poate cere recalibrare.
- Constantele pragurilor și duratelor sunt verificate la compilare, astfel încât combinațiile invalide să nu fie încărcate.
- Calibrarea folosește ultimele 8 citiri, aproximativ 2 secunde. Dacă acestea sunt instabile, o refuză. Calibrarea salvată anterior rămâne intactă dacă anulezi sau eșuează o recalibrare.

## Calibrarea: o singură comandă

Cu Arduino conectat și **alimentarea pompei deconectată**:

1. Pune partea sensibilă în pământ uscat. Scrie `c` și apasă Send/Enter.
2. Ține senzorul nemișcat 10 secunde. Programul capturează singur valoarea uscată.
3. La mesajul de mutare, pune senzorul în pământ foarte umed. Ai 20 de secunde; lasă-l stabil în ultimele secunde.
4. Dacă diferența dintre uscat și umed este de minimum 50 RAW, iar ultimele citiri variază cu cel mult 30 RAW, programul salvează calibrarea în EEPROM.
5. Pune senzorul în ghiveci, verifică apa, furtunul și firele, apoi reconectează sursa pompei.
6. Trimite `a` pentru udarea automată.

**Nou în v2.1: calibrarea nu pornește singură pompa.** Ea salvează valorile, iar `a` activează udarea. Valorile rămân după întreruperea alimentării; la repornire trimiți din nou `a`, fără recalibrare. Dacă nu există calibrare validă, automatul refuză pornirea.

Senzorul trebuie mutat fizic între cele două probe. Procentul afișat este o scară relativă între uscat și umed, nu o măsurare de laborator a cantității de apă.

## Cum udă automat

După `a`, așteaptă cel puțin **60 de secunde**. La **35% sau mai puțin**, dacă valoarea rămâne sub prag 3 secunde, releul alimentează pompa **2 secunde**. Dacă senzorul ajunge la **55%**, pompa se poate opri mai devreme.

După oprire, așteaptă încă **60 de secunde**. Dacă solul este încă prea uscat, poate face un alt puls. Nu lasă pompa pornită continuu până când apa ajunge la senzor.

Exemplu: 30% → udare 2 secunde → pauză 60 secunde → citește din nou. Dacă ajunge peste 35% după absorbție, nu mai pornește; dacă este încă la 30%, poate încerca din nou. Dacă rămâne prea uscat după 3 pulsuri, apare **BLOCAT** și cere verificarea apei/senzorului.

Limita absolută de **10 secunde** rămâne ca protecție de rezervă. În funcționarea normală, pulsul de 2 secunde oprește pompa înainte de această limită.

## Toate comenzile

În Serial Monitor la **9600 baud**, scrie câte un caracter și apasă **Send/Enter**. Literele mari și mici sunt acceptate; Enter/CR/LF sunt ignorate. Săgețile nu sunt comenzi pentru acest program.

| Comandă | Ce face exact |
|---|---|
| `1` | Pornește un test manual de **2 secunde**, fără calibrare și fără să depindă de umiditate. Dezactivează automatul. Dacă pompa merge deja, nu schimbă modul și nu prelungește timpul. Refuză testul în timpul calibrării, când există blocare sau în primele 0,5 s după o oprire. |
| `0` | Oprește imediat pompa și automatul; anulează calibrarea în curs. Nu șterge calibrarea salvată și nu elimină o blocare. |
| `a` | Oprește mai întâi pompa și testul manual. Dacă există calibrare validă, cel puțin 3 citiri plauzibile și nu există blocare, activează automatul și începe pauza de 60 s. Nu pornește direct pompa. Dacă cerințele nu sunt îndeplinite, rămâne oprit. |
| `c` | Oprește pompa și începe calibrarea ghidată: uscat după 10 s, umed după încă 20 s. La final salvează valorile, dar rămâne oprit până la `a`. |
| `o` | Oprește pompa și automatul, ca `0`; anulează calibrarea. |
| `r` | Oprește tot și elimină blocarea/contorul de pulsuri. Nu pornește pompa. După verificare trimiți `a` pentru automat sau, după minimum 0,5 s, `1` pentru test. În timpul calibrării, întâi o anulezi cu `0`. |
| `v` | Cere afișarea RAW, valorii filtrate, umidității, stării pompei, modului, numărului de pulsuri și erorilor. Dacă pompa merge, afișarea este amânată până se oprește. |
| `h` | Cere afișarea ajutorului și comenzilor. Dacă pompa merge, afișarea este amânată până se oprește. |

`u`, `w` și `s` din versiunile vechi sunt ignorate. Testarea manuală trebuie supravegheată, cu apă în pompă, conform instrucțiunilor acesteia. După un test, modul automat rămâne oprit până trimiți `a`.

## Pompa este prea puternică: ce poate face codul

**Releul este un întrerupător: motorul primește tensiunea sursei sau este oprit.** Nu controla viteza cu PWM pe releu și nu îl comuta rapid pentru a reduce puterea.

Această versiune reduce **durata și cantitatea de apă per udare**, nu viteza motorului. Constantele `WATER_PULSE_MS` și `MANUAL_PULSE_MS` sunt inițial 2000 ms. Ajustează-le după debitul măsurat al pompei; nu crește timpul peste limita absolută `MAX_PUMP_MS`.

Dacă motorul merge prea tare, verifică întâi că sursa are tensiunea cerută de pompă: pentru pompa de 5V din proiect, sursa trebuie să fie **5V DC**. Nu folosi o sursă de 9V/12V. Curentul nominal mai mare al unei surse corecte de 5V nu forțează automat motorul să consume mai mult. Pentru controlul turației este necesar un circuit de comandă potrivit pompei, de exemplu un driver/MOSFET dimensionat și protecție pentru motor; schema cu releu nu oferă asta.

## Arduino se deconectează din IDE

O dispariție a portului USB poate avea cauze electrice, de cablu sau de port. Nu putem confirma cauza doar din cod. Interferențele unui motor și o sursă insuficientă pot provoca funcționare instabilă; codul nu poate repara sursa sau cablul.

Testează în această ordine:

1. **Deconectează doar alimentarea pompei**, păstrând Arduino prin USB. Trimite `1`, apoi `0` de câteva ori. Releul trebuie să comute, iar portul USB să rămână disponibil.
2. Dacă se deconectează și fără pompă alimentată, verifică firele releului, scurtcircuitele, cablul USB de date și alt port USB direct al PC-ului.
3. Dacă este stabil fără pompă și se deconectează numai când motorul este alimentat, verifică sursa separată, curentul de pornire al pompei și protecția la interferențe. Acest rezultat indică o problemă legată de circuitul motorului, fără să identifice singur piesa defectă.
4. Pompa nu se alimentează din Arduino 5V. Pentru acest circuit cu contacte de releu, sursa pompei rămâne separată; nu uni plusurile surselor.
5. Pentru un motor DC cu perii, verifică dioda de protecție în paralel cu pompa, dimensionată pentru motor: banda către plus, celălalt capăt către minus. Nu monta dioda invers. Dioda de pe modul protejează bobina releului, nu motorul.
6. Dacă portul rămâne conectat, dar mesajul **BOOT v2.1** reapare fără să deschizi monitorul sau să faci Upload, Arduino s-a resetat. Deschiderea Serial Monitor poate provoca un reset normal pe UNO.

Până rezolvi cauza deconectării, testează supravegheat. După orice reset, v2.1 rămâne oprit și nu reia singur udarea.

## Releu activ LOW sau HIGH

```cpp
const bool RELAY_ACTIVE_LOW = true;
```

LOW activează și HIGH oprește un releu activ LOW. Dacă modulul tău este activ HIGH, schimbă în `false` și fă Upload. Verifică asta întâi cu pompa deconectată.

Programul pregătește nivelul OFF înainte de configurarea D7 ca ieșire. În timpul resetării, înainte să ruleze programul, D7 nu este încă o ieșire comandată; comportamentul releului depinde de circuitul de intrare al modulului. Dacă acesta se activează la reset chiar și fără motor alimentat, circuitul de intrare trebuie verificat, nu doar codul.

## Verificarea codului și limite

Versiunea v2.1 a fost compilată pentru **Arduino UNO / ATmega328P**, cu **Arduino CLI 1.3.1**, pachetul **Arduino AVR Boards 1.8.6** și compilatorul **avr-gcc 7.3.0**.

- Flash: **7578 / 32256 bytes (23%)**.
- Variabile globale în RAM: **262 / 2048 bytes (12%)**; rămân 1786 bytes pentru stivă și variabile locale.
- Compilarea sketch-ului a reușit. Prima compilare a afișat avertismente de parametri nefolosiți din biblioteca de bază Arduino, nu din sketch.
- **114 verificări simulate au trecut**, cu întârzieri pentru releu și UART, timp de 32 de biți, comenzi repetate, schimbarea modului, pauze/pulsuri/blocare, senzor defect, filtrare, calibrare și anulare.
- Au fost testate modificările individuale ale fiecăruia dintre cei 64 de biți ai înregistrării EEPROM, întreruperea salvării în diferite puncte, migrarea calibrării vechi și senzori cu sens invers al valorilor.
- Testele pe calculator au folosit verificarea accesului la memorie și a comportamentelor nedefinite. Acestea nu simulează curentul motorului, contactele fizice sau comportamentul portului USB.
- În codul vechi a fost reprodusă o repornire fără pauza de 60 s când ceasul avansa în timpul opririi releului. Același scenariu trece cu v2.1.

**Compilarea și testele nu confirmă că montajul tău fizic este în regulă.** Nu avem măsurători ale sursei, curentului motorului sau zgomotului electric. Dacă USB se deconectează, urmează testele de izolare de mai sus. Protecția de timp depinde de executarea programului: dacă microcontrolerul se blochează sau contactele releului se lipesc, codul nu garantează oprirea motorului.

Filtrul nu detectează toate defectele: un senzor scos poate da și valori intermediare. Nu există senzor de nivel, deci programul nu știe dacă rezervorul este gol. Verifică alimentarea și releul cu motorul deconectat înainte de proba cu apă.

## Structura proiectului

| Fișier | Conținut |
|---|---|
| arduino-plant-watering.ino | Singurul sketch: calibrare, filtrare, EEPROM și control |
| README.md | Montaj, comenzi și depanare |
| docs/prezentare-scurta.md | Explicația pentru școală |
| images/schema-udator-plante.jpg | Schema trimisă, convertită în JPG |

## Referințe

- [Alimentarea separată a motoarelor și interferențele — Adafruit](https://learn.adafruit.com/adafruit-motor-shield-v2.1-for-arduino/powering-motors)
- [Motor DC și diodă de protecție — Adafruit](https://learn.adafruit.com/adafruit-arduino-lesson-13-dc-motors/transistors)
- [Placă nerecunoscută / cablu USB — Arduino](https://support.arduino.cc/hc/en-us/articles/4412955149586-If-your-board-is-not-detected-by-Arduino-IDE)
- [Upload în Arduino IDE](https://support.arduino.cc/hc/en-us/articles/4733418441116-Upload-a-sketch-in-Arduino-IDE)
