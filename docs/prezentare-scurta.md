# Prezentare scurtă — Udător automat pentru plante

Proiectul meu este un sistem automat de udare a plantelor cu Arduino UNO. Senzorul capacitiv de umiditate oferă o valoare analogică pe A0. După calibrarea în sol uscat și foarte umed, programul transformă citirea într-un procent relativ. Calibrarea se face din Serial Monitor, în același fișier cu programul, și se salvează în EEPROM.

Arduino comandă releul prin D7. Releul conectează sursa separată de 5V la pompă prin contactele COM și NO. Pompa nu este alimentată din Arduino.

Când umiditatea ajunge la 35% sau mai puțin, pompa poate porni. Se oprește la 55%. Diferența între praguri împiedică comutările repetate. După oprire urmează o pauză de 60 de secunde pentru distribuirea apei în sol. Dacă funcționează continuu 10 secunde, pompa se blochează până verific montajul și trimit comanda de deblocare.

Astfel demonstrez citirea unui senzor analogic, comanda releului, calibrarea și salvarea datelor în memoria EEPROM. Rezervorul trebuie verificat separat, deoarece sistemul nu are senzor de nivel.
