# Analisi del progetto (stato al 23/10/2026)

## Cos'è
Clone DIY del **Beyblade X BX-09 Beybattle Pass** (Takara Tomy): un dispositivo che si aggancia al lanciatore, misura la
potenza di lancio e la manda via Bluetooth a un'app.
- **Firmware** `firmware/xbattle_firmware` — XIAO ESP32-C3: legge il TCRT5000 via interrupt, calcola gli RPM, invia via BLE
  (`LIVE:`, `RT:`, `S:`, `SYNC_COUNT:`, `SYNC_END`, `BTN_SYNC`), salva i lanci offline in NVS (max 500), OLED opzionale.
- **App** `app/` — Flutter: connessione BLE, gauge, sessioni, top 5, storico (`shared_preferences`).
  UUID servizio/caratteristiche verificati: coincidono col firmware.
- **Case** `hardware/case` — v4 (da rifare, vedi sotto).

## Incongruenze trovate nei file
| Dove | Problema | Verità da adottare |
|------|----------|--------------------|
| docx §3 | batteria "802540" ma quote 5x20x30 mm (= 502030) | 802540 = 8x25x40 mm |
| PNG saldature e pianta | batteria "502030" | 802540 |
| PNG saldature | pin errati: 5V/GND/3V3 disegnati sul lato sinistro e D7-D10 a destra; sul vero XIAO C3 sono invertiti (D0-D6 a sinistra; 5V, GND, 3V3, D10-D7 a destra). Collegamenti sensore su D9/D7, display su D10... non coincidono con firmware e tabella | usare **solo** la tabella di `wiring_diagram.md` (D1 sensore, D4/D5 I2C, D6 pulsante); rifare il PNG |
| PNG pianta | testo corrotto ("FRONTE EOLSCK"), quote che non corrispondono allo .scad | da rifare col nuovo case |
| docx | è "revisione 3" ma il file si chiama v4; codice firmware nel docx obsoleto (`OLED_SDA 5/6`, `3V4`); il `.ino` è giusto (GPIO6/7 = D4/D5) | il `.ino` è la fonte di verità |
| App | non gestisce il messaggio `BTN_SYNC` che il firmware invia alla pressione del pulsante: il pulsante quindi non fa nulla lato app | da implementare |

## Firmware: problemi tecnici, in ordine di priorità
1. **Campionamento perso.** A 15.000 RPM con 6 impulsi/giro arrivano ~1500 impulsi/s (uno ogni ~0,67 ms), ma il `loop()` gira
   ogni ~2 ms e legge una sola variabile `pulseInterval` sovrascritta ad ogni interrupt: se ne leggono solo 1 su 3-4, e il
   picco è sensibile al rumore. Soluzione: buffer circolare riempito dall'ISR, media su N impulsi, picco su finestra mobile.
2. **PULSES_PER_REV fisso a 6.** Il sensore legge una parte del *lanciatore*, non il bey: String e Winder hanno denti e
   rapporti diversi. Serve una costante per lanciatore (selezionabile da pulsante/app) e una calibrazione contro valori noti.
3. **`LIVE:` a ogni impulso.** Il BLE non regge centinaia di notify/s: limitare a ~20-30 Hz.
4. **Accessi concorrenti all'I2C.** `updateDisplay()` viene chiamata sia dal `loop()` sia dai callback BLE (task diverso):
   rischio di corruzione/blocchi. Usare flag e aggiornare il display solo nel `loop()`.
5. **`delay(60)` dentro `onWrite` (SYNC).** Blocca lo stack BLE per decine di lanci. Inviare dal `loop()` con una coda.
6. **Nessun risparmio energetico.** CPU e advertising BLE sempre attivi con una LiPo da 550 mAh: autonomia presumibilmente
   modesta (da misurare). Aggiungere sleep dopo inattività. Attenzione: sul C3 il risveglio da deep sleep funziona solo sui
   GPIO0-5 (D0-D3); **D6 (GPIO21) non può svegliare** → se vuoi il risveglio da pulsante, spostalo su D2 o D3
   (liberi se rinunci ai LED di fallback).
7. **D6 = GPIO21 = TX UART0.** Con "USB CDC On Boot" disattivato, `Serial.print` esce su quel pin e confligge col pulsante.
   Attivare "USB CDC On Boot" nella configurazione della scheda.
8. **Versione del core Arduino-ESP32.** La 3.x ha modificato l'API BLE (firme dei callback, tipo restituito da `getValue`):
   fissare una versione testata (es. 2.0.x) oppure migrare a NimBLE-Arduino. Va verificato compilando.
9. **Nessuna misura della batteria** (la XIAO C3 non ha sense integrato): partitore 2x100 kΩ su un pin ADC, se serve.

## App
- 694 righe in un unico file: da spezzare (BLE, repository lanci, UI).
- Manca lo scaffolding Android/iOS: da `app/` eseguire `flutter create .` (non sovrascrive `lib/` e `pubspec.yaml`
  se li rimetti a posto, vedi README) e aggiungere i permessi BLE nel manifest.
- `convertRpmToLaunchPower()` assume LP = RPM: è un'ipotesi, non un dato verificato.
- Syncfusion Gauges ha licenza Community con condizioni d'uso: controllale; in alternativa un gauge custom.

## Case v4: perché non calza
- Le quote del lanciatore erano stime (il docx lo dichiara).
- 130 x 32 x 30 mm: enorme rispetto a ciò che contiene. Il volume minimo dei componenti (batteria 40x25x8, XIAO ~21x18,
  OLED ~38x12, modulo TCRT5000 32x14) permette un guscio molto più corto e basso — stima da validare col nuovo layout.
- L'aggancio (binari a T + levetta con molla di penna + perno da filamento) è complesso e non validato su un lanciatore reale.
