# XBattle — Beyblade X Battle Pass DIY

Dispositivo DIY che si aggancia ai lanciatori Beyblade X (String e Winder), misura la potenza di lancio con un
sensore ottico e la invia via BLE a un'app Flutter.

```
xbattle/
├── app/                     App Flutter (lib/main.dart, pubspec.yaml)
├── firmware/xbattle_firmware/  Sketch Arduino per XIAO ESP32-C3
├── hardware/case/
│   ├── legacy_v4/           Case v4 (SCAD+STL) — da rifare, tenuto come storico
│   └── tools/               Calibro di tolleranza per la stampante
└── docs/                    Analisi, hardware, misure, wiring, documento di progetto originale
```

Documenti utili: `docs/ANALISI.md` · `docs/HARDWARE.md` · `docs/MISURE_LANCIATORI.md` · `docs/wiring_diagram.md`

## App (prima volta)
```bash
cd app
flutter create . --project-name bbx_battle_pass_pro --org it.xbattle   # genera android/ ios/
git checkout -- lib/main.dart pubspec.yaml                              # ripristina i file del repo
flutter pub get
flutter run
```
Aggiungi i permessi BLE in `android/app/src/main/AndroidManifest.xml` (BLUETOOTH_SCAN, BLUETOOTH_CONNECT).

## Firmware
Arduino IDE 2 → scheda **XIAO_ESP32C3**, "USB CDC On Boot: Enabled"; librerie: U8g2. Apri `firmware/xbattle_firmware/`.
Usa il core arduino-esp32 2.0.x finché non si verifica la compatibilità BLE con la 3.x.

## Case 3D
Vedi `docs/MISURE_LANCIATORI.md`: il nuovo case parte dalle misure reali dei lanciatori.
