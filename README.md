# Udător automat pentru plante cu Arduino UNO

Un proiect pentru școală care citește umiditatea solului și comandă o pompă de apă printr-un releu. **Tot codul, inclusiv calibrarea, se află într-un singur fișier:** [arduino-plant-watering.ino](arduino-plant-watering.ino).

Calibrarea se face din Serial Monitor și se salvează în memoria EEPROM, astfel încât valorile să rămână după oprirea alimentării. Nu trebuie să instalezi biblioteci suplimentare: EEPROM este inclusă în pachetul Arduino AVR.

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

## Cum încarci codul în Arduino IDE

1. Pe GitHub apasă **Code → Download ZIP** și dezarhivează proiectul.
2. Folderul sketch-ului trebuie să se numească **arduino-plant-watering**, exact ca fișierul .ino. Dacă arhiva creează folderul arduino-plant-watering-main, redenumește-l.
3. Deschide fișierul **arduino-plant-watering.ino** în Arduino IDE.
4. Conectează Arduino prin USB.
5. Selectează **Arduino Uno** și portul plăcii din meniul **Tools** sau selectorul de placă.
6. Apasă **Verify**, apoi **Upload**.
7. Deschide **Serial Monitor** la **9600 baud**.

După Upload, programul rulează pe placă. Nu trebuie să păstrezi IDE-ul deschis pentru udare. Deschiderea Serial Monitor poate reseta UNO; programul reîncarcă valorile salvate și așteaptă 60 de secunde înainte de udare.

## Calibrare cu o singură comandă

La prima pornire, fără calibrare salvată, pompa rămâne oprită. **Deconectează alimentarea pompei înainte de calibrare.** Ține electronica și conectorul senzorului departe de apă.

1. Pune senzorul în pământ uscat. Deschide Serial Monitor la **9600 baud**, trimite doar litera **c** și apasă Enter.
2. Lasă senzorul nemișcat **10 secunde**. Programul memorează singur valoarea pentru sol uscat.
3. Când vezi mesajul „Acum muta senzorul in sol foarte umed”, mută senzorul în pământ foarte umed. Ai **20 de secunde** să îl muți și să îl lași să se stabilizeze.
4. Programul citește valoarea umedă, verifică diferența dintre măsurători, salvează calibrarea în EEPROM și activează modul automat. Dacă verificarea eșuează, repetă de la pasul 1.
5. Reconectează alimentarea pompei după ce verifici montajul. Udarea poate începe după **60 de secunde** de la salvare, dacă solul e sub pragul de pornire.

**Nu trebuie să trimiți u, w, s sau a pentru calibrarea normală.** Folosești doar `c` și muți fizic senzorul între solul uscat și cel umed. Nu poate deduce ambele valori dintr-o singură probă de sol.

Valorile rămân salvate în EEPROM. La următoarea pornire, Arduino activează singur modul automat și așteaptă 60 de secunde înainte de prima udare.

| Comandă | Acțiune |
|---|---|
| `c` | Pornește calibrarea ghidată și oprește pompa |
| `h` | Afișează ajutorul |
| `a` | Reactivează udarea după o oprire manuală sau după deblocare |
| `o` | Oprește pompa și modul automat pentru sesiunea curentă; anulează calibrarea în curs |
| `r` | Elimină blocarea după limita de 10 secunde; după verificarea montajului, trimite și `a` |

Comenzile sunt litere mici. Enter și opțiunile de terminare a liniei sunt ignorate. Dacă limita de 10 secunde s-a activat înainte de recalibrare, ea rămâne blocată până verifici montajul și trimiți `r`, apoi `a`.

**Procentul este o scară relativă între cele două valori calibrate**, nu o măsurare de laborator a cantității de apă din sol.

## Cum funcționează

- Senzorul este citit la fiecare 250 ms; fiecare citire este media a 16 măsurători.
- La cel mult **35%**, pompa pornește, dacă a trecut pauza de 60 de secunde.
- La cel puțin **55%**, pompa se oprește.
- După oprire, programul așteaptă **60 de secunde** pentru distribuirea apei în sol.
- Dacă pompa funcționează **10 secunde continuu**, se oprește și udarea se blochează. Nu repornește automat în bucla următoare.
- Pentru deblocare, verifică apa, furtunul și senzorul, apoi trimite **r** și **a**.
- O valoare RAW aproape de 0 sau 1023 oprește automat udarea. Această verificare nu detectează toate defecțiunile, un senzor deconectat poate da și valori intermediare.

Cele două praguri formează un histerezis: pompa nu comută repetat când valoarea oscilează în jurul unui singur prag. Pragurile, pauza și timpul maxim pot fi schimbate în constantele de la începutul fișierului. Limita de 10 secunde se adaptează debitului pompei și ghiveciului după teste supravegheate.

## Releu activ LOW sau HIGH

Codul folosește inițial:

```cpp
const bool RELAY_ACTIVE_LOW = true;
```

Asta înseamnă LOW pentru activare și HIGH pentru oprire. Verifică specificațiile modulului tău. Dacă este activ HIGH, schimbă valoarea în **false** și reîncarcă programul. Testează cu alimentarea pompei deconectată.

Programul pregătește nivelul de oprire înainte de configurarea D7 ca ieșire. Comportamentul releului în timpul resetării, înainte să ruleze codul, depinde de modul și de circuitul de intrare al acestuia.

## Testare și probleme frecvente

| Situație | Ce verifici |
|---|---|
| RAW nu variază | AOUT la A0, VCC și GND, senzor introdus în sol |
| Releul lucrează invers | Valoarea RELAY_ACTIVE_LOW |
| Releul clicăie, pompa nu merge | COM și NO, sursa separată, polaritatea și necesarul pompei |
| Pompa se blochează după 10 secunde | Rezervorul, furtunul, debitul și poziția senzorului; r apoi a după verificare |
| Nu începe imediat udarea | Calibrarea, modul automat, pragul de 35% și pauza de 60 secunde |
| Arduino se resetează când pornește pompa | Sursa pompei, firele, interferențele motorului și dioda de protecție |

Ține apa departe de Arduino, releu și conexiunile electrice. Folosește doar partea de joasă tensiune a unei surse gata construite; nu conecta tensiunea de la priză la montaj. Prima testare se face supravegheat. Fără senzor de nivel, programul nu poate detecta un rezervor gol.

## Prezentare pentru școală

Vezi [prezentarea scurtă](docs/prezentare-scurta.md), cu rolul componentelor și explicația funcționării.

## Structura proiectului

| Fișier | Conținut |
|---|---|
| arduino-plant-watering.ino | Singurul sketch: calibrare, EEPROM și udare |
| README.md | Montaj, utilizare și depanare |
| docs/prezentare-scurta.md | Text de prezentare |
| images/schema-udator-plante.jpg | Imaginea originală a schemei |

## Referințe Arduino

- [Încărcarea unui sketch](https://support.arduino.cc/hc/en-us/articles/4733418441116-Upload-a-sketch-in-Arduino-IDE)
- [Memoria EEPROM](https://docs.arduino.cc/learn/built-in-libraries/eeprom/)
- [analogRead](https://docs.arduino.cc/language-reference/en/functions/analog-io/analogRead/)
