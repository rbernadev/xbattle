# Analisi del progetto (aggiornata al 23/10/2026)

## Cos'è
Clone DIY del **Beyblade X BX-09 BeyBattle Pass** (Takara Tomy; scheda in `ORIGINALE_BX09.md`): si aggancia al lanciatore
(String o Winder), misura la potenza di lancio e la manda via Bluetooth a un'app.
- **Firmware** `firmware/xbattle_firmware` — XIAO ESP32-C3: TCRT5000 via interrupt, RPM, BLE, lanci offline in NVS (max 500), OLED animato.
- **App** `app/` — Flutter: connessione BLE, gauge, sessioni, top 5, storico. UUID e protocollo coincidono col firmware.
- **Case** `hardware/case` — da rifare; la v4 è in `archive/case_v4`.

## Stato dei problemi (✅ risolto · 🟡 parziale · ⬜ aperto)
### Firmware
| # | Problema | Stato |
|---|---|---|
| 1 | Campionamento perso (loop 2 ms contro impulsi ogni ~0,67 ms, una sola variabile) | ✅ v2: buffer circolare nell'ISR, media su 4 intervalli. Simulato: picco 14189 su 14200 reali |
| 2 | `PULSES_PER_REV = 6` fisso, ma String e Winder hanno denti/rapporti diversi | ⬜ serve una costante per lanciatore + calibrazione |
| 3 | `LIVE:` a ogni impulso | ✅ v2: max ~25 messaggi/s |
| 4 | I2C/display usati anche dai callback BLE (task diverso) | ✅ v2: i callback impostano solo flag, il display lo tocca solo `loop()` |
| 5 | `delay(60)` dentro `onWrite` (SYNC) | ✅ v2: macchina a stati nel `loop()`, un lancio ogni 60 ms |
| 6 | Nessun risparmio energetico (BLE e CPU sempre attivi, LiPo 550 mAh). Il risveglio da deep sleep sul C3 funziona solo su GPIO0-5 (D0-D3): D6 non può svegliare | ⬜ |
| 7 | D6 = GPIO21 = TX UART0: con "USB CDC On Boot" disattivato `Serial.print` confligge col pulsante | ⬜ (basta attivare l'opzione, vedi README) |
| 8 | Core arduino-esp32 3.x ha cambiato l'API BLE | ⬜ usare 2.0.x o migrare a NimBLE |
| 9 | Nessuna misura dello stato batteria | ⬜ partitore 2x100 kΩ su un pin ADC |
| 10 | La v2 non è mai stata provata su hardware | ⬜ **da fare appena arriva la XIAO** |

### App
- ⬜ Non gestisce `BTN_SYNC` (il pulsante fisico non avvia il sync lato app).
- ⬜ 694 righe in un file da spezzare; manca lo scaffolding Android/iOS (`flutter create .`, vedi README) e i permessi BLE.
- ⬜ `convertRpmToLaunchPower()` assume LP = RPM. Dal manuale ufficiale l'app originale mostra "SP" dell'ordine di 300-7500:
  la relazione con gli RPM è **sconosciuta** e va calibrata contro un Battle Pass originale, se se ne trova uno.
- ⬜ Syncfusion Gauges: controllare le condizioni della licenza Community.

### Case
- La v4 non calzava perché si basava su quote inventate **e su un meccanismo sbagliato** (binari a T + levetta con molla di penna).
  Il manuale mostra un attacco a **4 sporgenze/incavi + 2 ganci laterali a scatto** (`ORIGINALE_BX09.md`).
- Era anche enorme (130 x 32 x 30 mm) rispetto al contenuto.
- La geometria dei ganci e della piastra è disponibile in `hardware/case/reference/` (CC BY-SA); mancano ancora fondo con le
  4 sporgenze e finestra del sensore → `MISURE_LANCIATORI.md`.
- **Incognita principale:** cosa legge il sensore IR dell'originale e attraverso quale finestra. Finché non è chiaro, la posizione del TCRT5000 nel case è un'ipotesi.

## Incongruenze trovate nei file originali (ora in `archive/`)
| Dove | Problema | Verità adottata |
|---|---|---|
| docx §3 | batteria "802540" ma quote 5x20x30 mm (=502030) | 802540 = 8x25x40 mm (da verificare con calibro, vedi `COMPONENTI_MISURATI.md`) |
| PNG saldature e pianta | batteria "502030" | 802540 |
| PNG saldature | pin errati rispetto al vero XIAO C3 (D0-D6 a sinistra; 5V, GND, 3V3, D10-D7 a destra); collegamenti non coincidono con firmware | solo la tabella di `wiring_diagram.md` (D1 sensore, D4/D5 I2C, D6 pulsante) |
| PNG pianta | testo corrotto, quote che non corrispondono allo .scad | da rifare col nuovo case |
| docx | "revisione 3" ma il file si chiama v4; codice firmware obsoleto (`OLED_SDA 5/6`, `3V4`) | il `.ino` è la fonte di verità |
| Note sull'attacco | "guida a T rovesciata + dente a molla" e "base a forchetta con braccetti a molla" | manuale e STL mostrano 4 sporgenze/incavi, 2 ganci a scatto separati |
