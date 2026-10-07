# Prezentare scurtă — Udător automat pentru plante, v2

Proiectul folosește Arduino UNO pentru a citi un senzor capacitiv de umiditate pe A0. Citirile sunt filtrate pentru a reduce influența fluctuațiilor. Calibrarea se pornește cu o singură comandă și folosește probe de sol uscat și foarte umed. Valorile sunt salvate în EEPROM.

Arduino comandă un releu prin D7. Contactele COM și NO conectează sursa separată de 5V la pompă. Motorul nu se alimentează din Arduino. Releul poate porni și opri motorul, dar nu îi reglează viteza.

După pornire sau reset, pompa rămâne oprită. Comanda a activează udarea automată. Dacă solul este la 35% sau mai puțin și valoarea rămâne sub prag, programul udă cel mult 2 secunde, apoi așteaptă 60 de secunde pentru absorbția apei. La 55% poate opri pompa mai devreme. Dacă solul rămâne uscat după 3 pulsuri și pauzele dintre ele, sistemul se blochează pentru verificare.

Comanda 1 pornește un test manual de 2 secunde, iar 0 oprește pompa. Aceste teste nu necesită calibrare. Rezervorul trebuie verificat separat deoarece sistemul nu are senzor de nivel. Cablarea, sursa și protecția motorului sunt importante pentru a preveni interferențele și resetările.
