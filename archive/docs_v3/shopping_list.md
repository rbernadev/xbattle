# Lista della Spesa — Battle Pass DIY v4 (BOM Rigorosa)

Questa è la distinta base (BOM) esatta. **Non ci sono alternative o opzioni**. Per garantire il corretto assemblaggio nel guscio stampato 3D (che ha tolleranze al decimo di millimetro) e il funzionamento corretto del firmware, **devi acquistare esattamente questi modelli specifici, STOP.**

### 1. Microcontrollore: Seeed Studio XIAO ESP32-C3
* **Specifiche tassative**: Devi prendere la versione **C3** (non base, non S3). È l'unica che offre il chip RISC-V con BLE 5.0 e circuito di ricarica LiPo integrato mantenendo le dimensioni miniaturizzate di 21x17.5mm.
* **Costo medio**: ~10.00€ - 12.00€ (su Amazon IT).
* **Link per acquisto (Amazon)**: [Clicca qui per il XIAO ESP32-C3 su Amazon](https://www.amazon.it/s?k=XIAO+ESP32-C3)

### 2. Sensore Ottico RPM: Modulo TCRT5000 a 4-PIN (con LM393)
* **Specifiche tassative**: Devi prendere la schedina **rettangolare (spesso blu) con 4 piedini** (VCC, GND, D0, A0). **❌ ATTENZIONE AL MODELLO SBAGLIATO**: NON comprare la versione "KY-033" (che è nera, lunga, ha un grosso blocco di plastica nera attorno al sensore e solo 3 pin). Il nostro modello 3D è calcolato al millimetro (32x14mm) solo per la versione a 4 pin.
* **Costo medio**: ~6.00€ (venduti di solito in pacchi da 3 o 5 pezzi su Amazon).
* **Link per acquisto (Amazon)**: [Clicca qui per i moduli TCRT5000 a 4 PIN su Amazon](https://www.amazon.it/s?k=TCRT5000+4+pin)

### 3. Batteria: LiPo 3.7V 802540 (550mAh)
* **Specifiche tassative**: Il codice batteria deve essere ESATTAMENTE **802540**. Questo codice detta le misure fisiche: 8mm di spessore, 25mm di larghezza, 40mm di lunghezza. È il limite fisico calcolato al millimetro per il vano della scocca. Qualsiasi batteria più grande non ti farà chiudere lo sportello.
* **Costo medio**: ~8.00 - 10.00€ (su Amazon).
* **Link per acquisto (Amazon)**: [Clicca qui per la LiPo 802540 su Amazon](https://www.amazon.it/s?k=LiPo+802540+3.7V)

### 4. Display: OLED 0.91" 128x32 I2C (SSD1306)
* **Specifiche tassative**: Schermo da **0.91 pollici**, formato rettangolare lungo, con interfaccia **I2C a 4 pin** (GND, VCC, SCL, SDA). Il PCB misura 38x12mm. *Avviso: NON comprare la versione SPI a 7 pin, lo scasso è disegnato per i 4 pin.*
* **Costo medio**: ~6.00€ - 8.00€ (su Amazon).
* **Link per acquisto (Amazon)**: [Clicca qui per l'OLED 0.91 I2C su Amazon](https://www.amazon.it/s?k=OLED+0.91+I2C+SSD1306)

### 5. Pulsante di Sync: Tactile Switch 6x6x5mm
* **Specifiche tassative**: Pulsante standard THT (Through-Hole) a 4 pin. Base quadrata 6x6mm. **Altezza totale: 5mm**. Se prendi altezze diverse (es. 4.3mm o 7mm) il bottone affonderà dentro la scocca o sporgerà bloccandosi contro il bordo.
* **Costo medio**: ~5.00€ (venduti in scatole/kit assortiti su Amazon).
* **Link per acquisto (Amazon)**: [Clicca qui per i pulsanti 6x6x5 su Amazon](https://www.amazon.it/s?k=tactile+switch+6x6x5)

### 6. Interruttore di Accensione: Slide Switch SS12D00G3
* **Specifiche tassative**: Micro interruttore a scorrimento (SPDT a 3 pin). Il codice standard è **SS12D00** (oppure SS12D07). Il corpo misura 8.5x4mm, ideale per incastrarsi perfettamente nell'apposito alloggiamento stampato nel tappo posteriore.
* **Costo medio**: ~5.00€ (venduti in pacchi da 10-20 pezzi su Amazon).
* **Link per acquisto (Amazon)**: [Clicca qui per lo Slide Switch SS12D00 su Amazon](https://www.amazon.it/s?k=Slide+Switch+SS12D00G3)

### 7. Ferramenta per la Clip a Scatto (Recupero)
* **Molla**: 1x Molla estratta da una normale penna a scatto (biro).
* **Perno**: 1x Spezzone di filamento per stampante 3D da 1.75mm (tagliato dritto, lungo circa 15mm).
