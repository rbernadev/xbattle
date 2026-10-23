# Hardware necessario

## 1. Elettronica (dalla BOM in `shopping_list.md`)
| Q.tà | Componente | Specifica da rispettare | Costo indicativo* |
|------|-----------|--------------------------|-------------------|
| 1 | Seeed Studio **XIAO ESP32-C3** | proprio la C3 (BLE, carica LiPo a bordo, USB-C) | 10-12 € |
| 1 | Modulo **TCRT5000** a 4 pin (VCC, GND, D0, A0) con LM393 e trimmer | NON KY-033 (3 pin) | ~6 € (pacco da 3-5) |
| 1 | LiPo 3.7 V **802540** (8x25x40 mm, ~550 mAh) | verifica che abbia il circuito di protezione (PCM) e la polarità del connettore/cavi | 8-10 € |
| 1 | OLED **0.91" 128x32 I2C SSD1306** (4 pin, indirizzo 0x3C) | NON versione SPI a 7 pin; opzionale | 6-8 € |
| 1 | Tactile switch **6x6x5 mm** THT | altezza 5 mm | ~5 € (kit assortito) |
| 1 | Slide switch **SS12D00G3** (SPDT) | taglia il + della batteria | ~5 € (pacco 10-20) |

*Prezzi dalla lista originale, su Amazon.it; le confezioni contengono più pezzi del necessario. Totale componenti ≈ 40-46 €.

## 2. Consumabili e minuteria (NON nella lista originale)
- Cavetto siliconico flessibile **AWG 30** (o wire-wrap) in più colori
- Guaina termorestringente piccola, **nastro Kapton**
- Stagno + flussante
- Colla a caldo o cianoacrilato gel
- **Cavo USB-C dati** (non solo ricarica) per programmare la XIAO
- (per il nuovo case) viti **M2** corte oppure inserti a caldo M2, magneti 3x1 mm: da decidere col design

## 3. Attrezzi
- **Calibro digitale** (essenziale per il fit)
- Saldatore a punta fine, pinzette, multimetro
- Stampante: FDM (PETG/PLA+) o resina. Con resina standard evita ganci a scatto sottili (fragili); serve resina tough
  o un sistema di blocco diverso.

## 4. Opzioni per risparmiare spazio (da valutare nel nuovo case)
- Sostituire il modulo TCRT5000 (32x14 mm) con il **componente nudo** + 2 resistenze: risparmio di decine di mm, ma
  serve rifare la parte analogica e ritarare soglia/firmware.
- OLED: tenerlo (feedback immediato) o toglierlo e usare LED + app; il firmware gestisce entrambi i casi.
- Impilare in verticale (batteria sotto, XIAO sopra) anziché in fila.

## 5. Software / strumenti PC
- Android Studio (o VS Code) con plugin Flutter + Dart, Flutter SDK
- Arduino IDE 2 oppure PlatformIO per il firmware
- OpenSCAD per il case (+ slicer: PrusaSlicer/Bambu Studio/Chitubox a seconda della stampante)
