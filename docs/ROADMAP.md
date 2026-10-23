# Roadmap (aggiornata al 23/10/2026)

## Stato attuale
- Firmware v2 con OLED animato: simulato su PC, **mai provato su hardware**.
- App Flutter: da compilare la prima volta, manca `BTN_SYNC`.
- Case: nuovo, da progettare partendo da attacco reale (manuale + riferimento Thingiverse).
- Hardware in mano: OLED, LiPo 802540, pulsante, TCRT5000. Mancano XIAO ESP32-C3 (da confermare) e slide switch.

## Fasi
1. **Controllo componenti** — misure col calibro in `COMPONENTI_MISURATI.md`; polarità e tensione della LiPo.
2. **Acquisti** — XIAO ESP32-C3 (se manca), slide switch, filo AWG30, guaina, Kapton, stagno, cavo USB-C dati, calibro.
3. **Banco di prova senza case** — Arduino IDE 2 (XIAO_ESP32C3, USB CDC On Boot: Enabled, core 2.0.x, U8g2); OLED su D4/D5, pulsante D6, sensore D1; verifica OLED con boot animato, servizio BLE con nRF Connect.
4. **App** — `flutter create .`, permessi BLE, collegamento e ricezione `RT:`/`LIVE:`.
5. **Coupon zero dell'attacco** — stampa piastra + ganci dal riferimento Thingiverse e prova su String e Winder; calibro di tolleranza.
6. **Foto e quote delle port** (`MISURE_LANCIATORI.md`) — in particolare cosa c'è sotto la finestra del sensore.
7. **Prova del sensore sul lanciatore** — modulo fissato provvisoriamente, trimmer regolato; conta i denti, ricava `PULSES_PER_REV` per String e Winder.
8. **Firmware** — costante per lanciatore, sleep (pulsante su pin che sveglia), `BTN_SYNC` nell'app, LED di fallback come l'originale.
9. **Case v5** — layout compatto, coupon della port + finestra, prototipo, versione estetica.
10. **Assemblaggio e collaudo** — saldatura (batteria per ultima), ricarica USB-C, lanci reali, autonomia; calibrazione vs. originale se disponibile.
