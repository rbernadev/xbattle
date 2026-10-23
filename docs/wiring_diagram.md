# Schema di Collegamento Elettrico (Wiring Diagram)
**Versione Hardware:** XIAO ESP32-C3 + OLED + Batteria 802540

> ⚠️ **Gli schemi PNG originali (saldatura e posizionamento) sono obsoleti** e stanno in `archive/docs_v3/`: il pinout della XIAO
> è disegnato al contrario e la batteria riportata è la 502030. **Vale solo la tabella qui sotto** (coincide col firmware).
> I nuovi schemi andranno rifatti insieme al nuovo case.

Questo documento illustra come collegare tutti i componenti per il Battle Pass DIY. Tutti i pin citati si riferiscono al microcontrollore **Seeed Studio XIAO ESP32-C3**.

## 1. Schema a blocchi dei Pin (Pinout)

Tutta la logica opera a **3.3V**. Utilizza il pad `3V3` della XIAO per alimentare i sensori e lo schermo.

| Componente | Pin Componente | Pin XIAO ESP32-C3 | Note |
| :--- | :--- | :--- | :--- |
| **Sensore IR (TCRT5000)** | `VCC` | `3V3` | Alimentazione |
| | `GND` | `GND` | Massa |
| | `D0` (Digital Out) | **`D1`** | Segnale impulsi |
| | `A0` (Analog Out) | *Non collegato* | Non usato |
| **Display OLED (Opzionale)** | `VCC` | `3V3` | Alimentazione |
| | `GND` | `GND` | Massa |
| | `SDA` | **`D4`** (SDA) | Dati I2C |
| | `SCL` | **`D5`** (SCL) | Clock I2C |
| **Pulsante Frontale** | `Pin 1` | **`D6`** | Segnale (usa Pullup interno) |
| | `Pin 2` | `GND` | Massa |
| **Batteria LiPo 802540** | Cavo Rosso `+` | Pin `Centrale` Slide Switch | Il Positivo della batteria va al centro dell'interruttore |
| | Cavo Nero `-` | Pad `BAT-` | Massa diretta (Sotto la scheda XIAO) |
| **Interruttore (Slide Switch)** | Pin `Laterale` | Pad `BAT+` | Ritorno del positivo verso la scheda XIAO |

*(Nota per fallback senza display): Se non monti il display OLED, puoi collegare il positivo di un LED Blu al pin `D2` (con resistenza in serie verso GND) e il positivo di un LED Verde al pin `D3` (con resistenza in serie verso GND).*

---

## 2. Schema di Saldatura (Visuale)

La batteria va saldata nei due pad posteriori (BAT+/BAT-) della scheda XIAO, passando per lo slide switch sul positivo. Si consiglia di usare fili **AWG 30** flessibili.

*(schema PNG obsoleto, vedi avviso in alto — da rifare)*

### Consigli per la Saldatura
1. **Interruttore a Scorrimento (Slide Switch)**: Taglia a metà il cavo rosso (Positivo) della batteria. Salda la metà che proviene dalla batteria al pin **Centrale** dell'interruttore. Salda uno spezzone di cavo dal pin **Laterale** dell'interruttore al pad `BAT+` della XIAO. In questo modo interrompi fisicamente l'energia.
2. **Batteria**: Salda la batteria per ultima, per evitare cortocircuiti accidentali mentre muovi la scheda. Usa un pezzetto di nastro isolante (Kapton) per isolare i pad sottostanti se entrano in contatto con altri cavi.
3. **Derivazioni 3V3 e GND**: La XIAO ha un solo pad `3V3` e un solo pad `GND` sul bordo. Dovrai sdoppiare il cavo per alimentare sia lo schermo che il sensore. Un buon metodo è saldare due fili sottili nello stesso foro, oppure fare la biforcazione "in volo" e coprirla con guaina termorestringente.
4. **Sensore TCRT5000**: Spesso i pin del modulo sono dritti. Potrebbe essere necessario dissaldarli o piegarli a 90° affinché il sensore entri nel vano del modello 3D (`sensor_slot`).

---

## 3. Schema di Posizionamento nel Modello 3D

Una volta saldati i componenti, andranno alloggiati all'interno dell'holster (il guscio stampato in 3D). **Questa sezione si riferisce al case v4 (archiviato)**: il nuovo case cambierà layout e fori.

*(schema PNG obsoleto — da rifare col nuovo case)*

1. **Batteria 802540**: Va infilata sul fondo della sezione posteriore. Grazie all'aumento a 8x25x40mm calzerà a pennello senza "ballare".
2. **Scheda XIAO**: Va adagiata sopra la batteria. Assicurati che la porta USB-C sia rivolta verso la finestra `usb_slot_w` e a filo col retro del guscio.
3. **Interruttore (Slide Switch)**: Va incastrato nell'apposita fessura creata all'interno del tappo posteriore (`electronics_lid`), di fianco al foro USB. Fissa il corpo metallico con una goccia di colla a caldo in modo che la levetta scorra liberamente verso l'esterno.
4. **OLED**: Fai passare i 4 fili nella canalina e spingi lo schermo contro la mascherina superiore. Un goccio di colla a caldo ai bordi previene vibrazioni. Se non lo usi, usa il tappo cieco in ABS/PETG.
5. **Pulsante**: Va spinto nella sede anteriore rotonda. Usa i rebbi laterali per fermarlo.
6. **Sensore**: Infilalo dal basso nella fessura `sensor_sight_dia`. I led IR (blu e nero) devono affacciarsi direttamente sui binari dove scorre la cremagliera del lanciatore.
