# Hardware necessario

Legenda: ✅ già in tuo possesso (foto 23/10/2026) · 🛒 da comprare · ❓ da confermare

## 1. Elettronica
| Stato | Q.tà | Componente | Specifica da rispettare | Costo indicativo* | Cerca su Amazon.it |
|---|---|---|---|---|---|
| ❓ | 1 | Seeed Studio **XIAO ESP32-C3** | proprio la C3 (BLE, carica LiPo a bordo, USB-C) | 10-12 € | [link](https://www.amazon.it/s?k=XIAO+ESP32-C3) |
| ✅ | 1 | Modulo **TCRT5000** a 4 pin (VCC, GND, D0, A0) con LM393 e trimmer | NON KY-033 (3 pin). Il sensore sta sul lato inferiore, a un'estremità corta | ~6 € (pacco da 3-5) | [link](https://www.amazon.it/s?k=TCRT5000+4+pin) |
| ✅ | 1 | LiPo 3.7 V **802540** (EP, 550 mAh) | verifica circuito di protezione e polarità; misura la larghezza reale | 8-10 € | [link](https://www.amazon.it/s?k=LiPo+802540+3.7V) |
| ✅ | 1 | OLED **0.91" 128x32 I2C SSD1306** (4 pin, indirizzo 0x3C) | NON versione SPI a 7 pin | 6-8 € | [link](https://www.amazon.it/s?k=OLED+0.91+I2C+SSD1306) |
| ✅ | 1 | Tactile switch **6x6** THT | misura l'altezza totale (la lista chiedeva 5 mm) | ~5 € (kit assortito) | [link](https://www.amazon.it/s?k=tactile+switch+6x6x5) |
| 🛒 | 1 | Slide switch **SS12D00G3** (SPDT) | taglia il + della batteria | ~5 € (pacco 10-20) | [link](https://www.amazon.it/s?k=Slide+Switch+SS12D00G3) |

*Prezzi dalla lista originale; le confezioni contengono più pezzi del necessario.

## 2. Consumabili e minuteria
- Cavetto siliconico flessibile **AWG 30** (o wire-wrap) in più colori
- Guaina termorestringente piccola, **nastro Kapton**
- Stagno + flussante
- Colla a caldo o cianoacrilato gel
- **Cavo USB-C dati** (non solo ricarica) per programmare la XIAO
- Per il nuovo case: viti **M2** corte o inserti a caldo M2, magneti 3x1 mm — da decidere col design

## 3. Attrezzi
- **Calibro digitale** (essenziale per il fit)
- Saldatore a punta fine, pinzette, multimetro
- Stampante: FDM (PETG/PLA+) o resina. La resina standard rompe i ganci a scatto sottili: usa resina tough o stampa i ganci in PETG.

## 4. Per risparmiare spazio nel nuovo case
- Il **modulo TCRT5000 è sprecato**: ~32 mm di scheda per un sensore da ~10 mm. Si può tagliare/accorciare il PCB oppure usare il componente nudo + 2 resistenze (serve riaggiustare soglia e firmware).
- **OLED**: tenuto (mostra l'ultimo RPM con animazioni). Dissaldare l'header e collegare i fili direttamente toglie altezza.
- Impilare in verticale (batteria sotto, XIAO sopra) invece che in fila.
- Sensore **schermato dalla luce**: l'originale è sensibile a sole e lampade a incandescenza (`ORIGINALE_BX09.md`).

## 5. Software / strumenti PC
- Android Studio (o VS Code) con plugin Flutter + Dart, Flutter SDK
- Arduino IDE 2 oppure PlatformIO per il firmware
- OpenSCAD per il case (+ slicer: PrusaSlicer/Bambu Studio/Chitubox a seconda della stampante)
