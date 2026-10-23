/**
 * =============================================================================
 * XJUDGE — BATTLE PASS DIY FIRMWARE
 * Hardware: Seeed Studio XIAO ESP32-C3
 * Sensore:  TCRT5000 (riflessione IR) collegato al pin D1
 * Display:  AZ-Delivery 0.91" OLED 128x32 I2C SSD1306 (opzionale, auto-detect)
 * Batteria: LiPo 3.7V 802540 (8x25x40mm) 550mAh — ricaricabile via USB-C
 *
 * PROTOCOLLO BLE (ASCII):
 *   TX (Notify → App):
 *     "LIVE:<rpm>"       → RPM istantanei durante il lancio (streaming)
 *     "RT:<rpm>"         → Picco massimo RPM a fine lancio
 *     "SYNC_COUNT:<n>"   → Quanti lanci offline ci sono in memoria
 *     "S:<rpm>"          → Singolo lancio offline (inviato in sequenza)
 *     "SYNC_END"         → Fine trasmissione lanci offline
 *     "BTN_SYNC"         → Pulsante fisico premuto, l'app avvia il sync
 *   RX (Write ← App):
 *     "SYNC"             → L'app chiede i lanci offline salvati in NVS
 *     "CLEAR"            → L'app chiede di cancellare lo storage offline
 *
 * LOGICA PRINCIPALE (loop):
 *   1. Ogni impulso del sensore IR scatta un interrupt (IRAM_ATTR).
 *   2. Dall'intervallo tra due impulsi consecutivi si calcola l'RPM.
 *   3. Se non arrivano impulsi per 400ms → il lancio è finito.
 *   4. Il picco massimo ("RT:") viene inviato via BLE o salvato offline.
 * =============================================================================
 */

#include <Arduino.h>
#include <Wire.h>           // I2C per il display OLED
#include <U8g2lib.h>        // Libreria grafica per display SSD1306
#include <BLEDevice.h>      // Stack BLE ESP32
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>        // Descrittore standard per le notifiche BLE (CCCD)
#include <Preferences.h>    // NVS (Non-Volatile Storage): storage offline dei lanci


// =============================================================================
// PINOUT — XIAO ESP32-C3
// Nota: la XIAO ha i pin etichettati D0-D10. Usiamo i macro Arduino (D1, D2…)
// I2C è assegnato ai pin hardware nativi della scheda (GPIO6 e GPIO7).
// =============================================================================
#define OLED_SDA 6          // GPIO6  = D4 sulla XIAO ESP32-C3 (I2C SDA)
#define OLED_SCL 7          // GPIO7  = D5 sulla XIAO ESP32-C3 (I2C SCL)

#define SENSOR_PIN D1       // Segnale digitale dal TCRT5000 (INPUT_PULLUP)
#define LED_BLE    D2       // LED blu: stato connessione BLE (solo senza display)
#define LED_LAUNCH D3       // LED verde: in fase di lancio attivo (solo senza display)
#define BUTTON_PIN D6       // Pulsante frontale tactile 6x6mm (INPUT_PULLUP, attivo LOW)


// =============================================================================
// COSTANTI SENSORE E BLE
// =============================================================================

// Numero di lamelle/magneti che il sensore legge per ogni giro completo del beyblade.
// Il TCRT5000 conta i passaggi delle alette del disco: 6 impulsi = 1 giro.
#define PULSES_PER_REV 6

// Tempo minimo in microsecondi tra due impulsi validi (filtro anti-rimbalzo hardware).
// 250µs corrisponde a un tetto teorico di ~40.000 RPM, ben oltre i ~15.000 RPM reali.
// Valori sotto questa soglia sono rumore elettrico e vengono scartati.
#define DEBOUNCE_MICROS 250

// UUID del servizio BLE custom e delle sue due caratteristiche.
// Devono corrispondere ESATTAMENTE a quelli definiti in diy_battle_pass_provider.dart.
#define SERVICE_UUID   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_TX_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"  // Notify (ESP32 → App)
#define CHAR_RX_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a9"  // Write  (App → ESP32)


// =============================================================================
// OGGETTI GLOBALI
// =============================================================================

// Driver display OLED: inizializzato ma usato solo se hasDisplay == true.
// U8G2_R0 = nessuna rotazione, U8X8_PIN_NONE = reset non collegato.
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

Preferences preferences;        // Storage NVS per i lanci offline

BLECharacteristic* pTxChar = NULL;  // Caratteristica TX (globale per accederla dal loop)
bool deviceConnected = false;       // true se l'app è connessa via BLE
bool hasDisplay     = false;        // true se l'OLED è stato rilevato su I2C 0x3C


// =============================================================================
// VARIABILI INTERRUPT (volatile = scritte dall'ISR, lette dal loop principale)
// portMUX garantisce l'accesso atomico su ESP32 (dual-core)
// =============================================================================
volatile unsigned long lastPulseTime  = 0;  // Timestamp dell'ultimo impulso valido (µs)
volatile unsigned long pulseInterval  = 0;  // Intervallo tra gli ultimi due impulsi (µs)
volatile bool          newPulse       = false;  // Flag: c'è un nuovo impulso da processare?
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;


// =============================================================================
// ISR — INTERRUPT SERVICE ROUTINE DEL SENSORE IR
// Eseguita automaticamente ad ogni fronte di DISCESA sul pin SENSOR_PIN.
// IRAM_ATTR = caricata in RAM interna per esecuzione veloce (non nella flash).
// =============================================================================
void IRAM_ATTR handleSensorInterrupt() {
  portENTER_CRITICAL_ISR(&mux);       // Sezione critica: disabilita interrupt locali

  unsigned long currentTime = micros();
  unsigned long interval = currentTime - lastPulseTime;

  // Scarta impulsi troppo ravvicinati (rumore/rimbalzo hardware)
  if (interval > DEBOUNCE_MICROS) {
    pulseInterval  = interval;        // Salva l'intervallo valido
    lastPulseTime  = currentTime;     // Aggiorna il timestamp
    newPulse       = true;            // Segnala al loop che c'è un dato nuovo
  }

  portEXIT_CRITICAL_ISR(&mux);
}


// =============================================================================
// DISPLAY OLED — updateDisplay()
// Layout 128x32 pixel diviso in 3 zone:
//   ┌────────────────────────────────┐  ← y=0
//   │ STATUS (es. "BLE: CONNESSO")   │  ← y=7  (font 5x7)
//   ├────────────────────────────────┤  ← y=9  (linea separatore)
//   │ 12345  RPM      BATTLE PASS PRO│  ← y=24 (font grande + font piccolo)
//   └────────────────────────────────┘  ← y=32
// Se rpm == 0, al posto del numero viene mostrata la scritta "PRONTO".
// =============================================================================
void updateDisplay(String status, float rpm) {
  if (!hasDisplay) return;   // Nessun display collegato: esci subito

  u8g2.clearBuffer();

  // --- Riga 1: Status bar (angolo alto sx) ---
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setCursor(0, 7);
  u8g2.print(status);

  // Linea orizzontale separatrice sotto la status bar
  u8g2.drawHLine(0, 9, 128);

  // --- Area centrale: RPM o "PRONTO" ---
  u8g2.setFont(u8g2_font_profont22_tn);   // Font monospace grande per i numeri
  if (rpm > 0) {
    String rpmStr  = String(rpm, 0);      // Converti in intero (es. "14320")
    int    rpmWidth = u8g2.getStrWidth(rpmStr.c_str());
    u8g2.setCursor(2, 28);
    u8g2.print(rpmStr);

    // Unità "RPM" in font piccolo, subito dopo il numero
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(2 + rpmWidth + 3, 28);
    u8g2.print("RPM");
  } else {
    u8g2.setFont(u8g2_font_profont15_tr);
    u8g2.setCursor(2, 26);
    u8g2.print("PRONTO");
  }

  // --- Angolo alto dx: label fissa del prodotto ---
  u8g2.setFont(u8g2_font_5x7_tr);
  const char* label  = "BATTLE PASS PRO";
  int         labelW = u8g2.getStrWidth(label);
  u8g2.setCursor(128 - labelW - 1, 7);   // Allineato a destra con 1px di margine
  u8g2.print(label);

  u8g2.sendBuffer();  // Invia il buffer al display fisico
}


// =============================================================================
// NVS — saveOfflineLaunch()
// Salva un lancio in memoria non volatile (NVS) quando il BLE non è connesso.
// I lanci vengono indicizzati con chiavi "launch_0", "launch_1", ...
// La chiave "count" tiene il conteggio totale.
// Limite massimo: 500 lanci (FW-03: prevenzione overflow del contatore uint).
// =============================================================================
void saveOfflineLaunch(float rpm) {
  preferences.begin("bbx_offline", false);  // false = read/write mode

  unsigned int count = preferences.getUInt("count", 0);

  // Protezione overflow: se lo storage è pieno, scarta il lancio
  if (count >= 500) {
    preferences.end();
    if (hasDisplay) updateDisplay("MEM PIENA!", rpm);
    return;
  }

  // Salva il lancio con chiave progressiva
  String key = "launch_" + String(count);
  preferences.putFloat(key.c_str(), rpm);
  preferences.putUInt("count", count + 1);
  preferences.end();

  // Feedback visivo all'utente
  if (hasDisplay) {
    updateDisplay("SALVATO OFF", rpm);    // Display: mostra RPM + conferma
  } else {
    // LED: 2 lampeggi veloci del LED_LAUNCH come conferma
    for (int i = 0; i < 2; i++) {
      digitalWrite(LED_LAUNCH, LOW);  delay(100);
      digitalWrite(LED_LAUNCH, HIGH); delay(100);
    }
  }
}


// =============================================================================
// CALLBACK BLE — Gestione connessione/disconnessione
// Chiamate automaticamente dallo stack BLE quando l'app si connette o scollega.
// =============================================================================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    if (!hasDisplay) digitalWrite(LED_BLE, HIGH);  // LED fisso = connesso
    updateDisplay("BLE: CONNESSO", 0);
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    if (!hasDisplay) digitalWrite(LED_BLE, LOW);   // LED spento = disconnesso
    updateDisplay("BLE: ATTESA", 0);
    BLEDevice::startAdvertising();  // Riprende l'advertising per nuove connessioni
  }
};


// =============================================================================
// CALLBACK BLE — Gestione comandi in arrivo dall'app (caratteristica RX)
// L'app scrive sulla caratteristica CHAR_RX_UUID per comandare l'ESP32.
// =============================================================================
class MyCommCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    std::string rxVal = pChar->getValue();

    // --- Comando SYNC: l'app vuole scaricare i lanci offline ---
    if (rxVal == "SYNC") {
      updateDisplay("SYNC IN CORSO", 0);

      preferences.begin("bbx_offline", true);   // true = read-only mode
      unsigned int count = preferences.getUInt("count", 0);

      // 1. Informa l'app di quanti lanci stanno per arrivare
      pTxChar->setValue(("SYNC_COUNT:" + String(count)).c_str());
      pTxChar->notify();
      delay(100);  // Piccola pausa per garantire la ricezione sul lato app

      // 2. Invia ogni lancio salvato, uno alla volta (intervallo 60ms per BLE)
      for (unsigned int i = 0; i < count; i++) {
        String key    = "launch_" + String(i);
        float  rpmVal = preferences.getFloat(key.c_str(), 0.0);
        pTxChar->setValue(("S:" + String(rpmVal, 0)).c_str());
        pTxChar->notify();
        delay(60);   // Throttle BLE: evita di saturare il buffer del ricevitore
      }
      preferences.end();

      // 3. Segnala la fine della trasmissione
      pTxChar->setValue("SYNC_END");
      pTxChar->notify();
      updateDisplay("BLE: CONNESSO", 0);
    }

    // --- Comando CLEAR: l'app ha scaricato i dati e vuole svuotare lo storage ---
    else if (rxVal == "CLEAR") {
      preferences.begin("bbx_offline", false);
      preferences.clear();                    // Cancella tutti i lanci
      preferences.putUInt("count", 0);        // Azzera il contatore
      preferences.end();
      // Nessun feedback visivo: l'azione è silente per design
    }
  }
};


// =============================================================================
// SETUP — Inizializzazione al boot
// =============================================================================
void setup() {
  Serial.begin(115200);   // Serial monitor per debug (opzionale in produzione)

  // --- Auto-detect display OLED su I2C (indirizzo standard SSD1306 = 0x3C) ---
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0) {
    // ACK ricevuto: display presente
    hasDisplay = true;
    u8g2.begin();
    updateDisplay("AVVIO...", 0);
  } else {
    // Nessun ACK: display assente, usa i LED come feedback
    hasDisplay = false;
    pinMode(LED_BLE, OUTPUT);
    pinMode(LED_LAUNCH, OUTPUT);
    digitalWrite(LED_LAUNCH, HIGH);  // LED_LAUNCH acceso = in standby (logica invertita)
  }

  // --- Sensore IR: interrupt su fronte FALLING (passaggio lamella davanti al sensore) ---
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), handleSensorInterrupt, FALLING);

  // --- Pulsante frontale: tactile switch montato nel corpo 3D (active LOW) ---
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // --- Stack BLE: crea il server, il servizio e le due caratteristiche ---
  BLEDevice::init("BBX_BattlePass_PRO");   // Nome device visibile durante la scansione
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  // TX: caratteristica che l'ESP32 usa per inviare notifiche all'app
  pTxChar = pService->createCharacteristic(
    CHAR_TX_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxChar->addDescriptor(new BLE2902());  // CCCD: necessario per attivare le notifiche

  // RX: caratteristica su cui l'app scrive i comandi (SYNC, CLEAR)
  BLECharacteristic* pRxChar = pService->createCharacteristic(
    CHAR_RX_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxChar->setCallbacks(new MyCommCallbacks());

  pService->start();
  BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();   // Inizia ad annunciarsi come periferica BLE

  updateDisplay("BLE: ATTESA", 0);
}


// =============================================================================
// VARIABILI DI STATO DEL LANCIO (dichiarate fuori dal loop per persistere)
// =============================================================================
float maxRPM     = 0.0;    // Picco RPM del lancio corrente
bool  isLaunching = false;  // true dal primo impulso fino a 400ms di silenzio


// =============================================================================
// PULSANTE FRONTALE — Debounce software
// Algoritmo standard a due stati: registra il cambio, poi aspetta BUTTON_DEBOUNCE_MS
// prima di considerarlo stabile. Un secondo cooldown di 1s evita pressioni multiple.
// =============================================================================
bool          lastButtonState       = HIGH;
unsigned long lastButtonChangeMillis = 0;
const unsigned long BUTTON_DEBOUNCE_MS = 50;  // ms di stabilità richiesti

void checkButton() {
  bool currentState = digitalRead(BUTTON_PIN);

  // Rileva cambio di stato (fronte) e salva il timestamp
  if (currentState != lastButtonState) {
    lastButtonChangeMillis = millis();
    lastButtonState = currentState;
  }

  // Il pulsante è premuto (LOW) da abbastanza tempo → azione valida
  if (currentState == LOW && (millis() - lastButtonChangeMillis) > BUTTON_DEBOUNCE_MS) {
    static unsigned long lastActionMillis = 0;

    // Cooldown 1 secondo tra un'azione e la successiva (evita trigger multipli)
    if (millis() - lastActionMillis > 1000) {
      lastActionMillis = millis();

      if (deviceConnected && pTxChar != NULL) {
        // Invia "BTN_SYNC" all'app → l'app avvierà automaticamente la sincronizzazione
        pTxChar->setValue("BTN_SYNC");
        pTxChar->notify();
        updateDisplay("SYNC (pulsante)", 0);
      }
      // Se non connesso: il pulsante non fa nulla (il sync richiede BLE attivo)
    }
  }
}


// =============================================================================
// LOOP PRINCIPALE — eseguito ~500 volte al secondo (delay(2) finale)
// =============================================================================
void loop() {
  unsigned long currentMicros = micros();

  // 1. Controlla il pulsante frontale ad ogni ciclo
  checkButton();

  // 2. Blink LED BLE (solo senza display): lampeggio ogni 500ms con timer dedicato.
  //    Usare millis() % 1000 causerebbe jitter al primo ciclo dopo il rollover.
  if (!deviceConnected && !hasDisplay) {
    static unsigned long _lastBlinkMs = 0;
    static bool          _ledState    = false;
    if (millis() - _lastBlinkMs >= 500) {
      _lastBlinkMs = millis();
      _ledState    = !_ledState;
      digitalWrite(LED_BLE, _ledState ? HIGH : LOW);
    }
  }

  // 3. Rilevamento FINE LANCIO: nessun impulso per 400ms continui
  //    400ms è la finestra scelta perché i beyblade rallentano bruscamente a fine lancio.
  if (currentMicros - lastPulseTime > 400000) {
    if (isLaunching) {
      // Soglia minima 1200 RPM: filtra i lanci abortiti o il rumore residuo
      if (maxRPM > 1200) {
        if (deviceConnected) {
          // Invia il picco massimo all'app via BLE
          pTxChar->setValue(("RT:" + String(maxRPM, 0)).c_str());
          pTxChar->notify();
          updateDisplay("LANCIO!", maxRPM);
        } else {
          // BLE non disponibile: salva il lancio offline in NVS
          saveOfflineLaunch(maxRPM);
        }
      }
      // Reset stato lancio
      isLaunching = false;
      maxRPM      = 0.0;
      if (!hasDisplay) digitalWrite(LED_LAUNCH, HIGH);  // LED_LAUNCH torna in standby
    }
  }

  // 4. Elaborazione NUOVO IMPULSO dal sensore (settato dall'ISR)
  if (newPulse) {
    // Leggi i dati volatili in sezione critica per evitare race condition con l'ISR
    portENTER_CRITICAL(&mux);
    unsigned long interval = pulseInterval;
    newPulse = false;
    portEXIT_CRITICAL(&mux);

    // Primo impulso → il lancio è iniziato
    isLaunching = true;
    if (!hasDisplay) digitalWrite(LED_LAUNCH, LOW);  // LED_LAUNCH acceso durante lancio

    // Formula RPM:
    //   interval = microsecondi tra due impulsi consecutivi
    //   60.000.000 µs = 1 minuto in microsecondi
    //   / PULSES_PER_REV = converti da impulsi/min a giri/min
    float currentRPM = (60000000.0 / interval) / PULSES_PER_REV;

    // Scarta letture fisicamente impossibili (> 40.000 RPM = rumore elettrico)
    if (currentRPM < 40000.0) {
      if (currentRPM > maxRPM) maxRPM = currentRPM;  // Aggiorna il picco

      // Streaming live all'app: invia ogni RPM valido sopra la soglia minima
      if (deviceConnected && currentRPM > 1200) {
        pTxChar->setValue(("LIVE:" + String(currentRPM, 0)).c_str());
        pTxChar->notify();
      }
    }
  }

  delay(2);  // ~2ms di pausa: bilancia reattività e consumo CPU
}
