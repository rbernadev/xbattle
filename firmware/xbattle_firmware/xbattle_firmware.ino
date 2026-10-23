/**
 * =============================================================================
 * XBATTLE — BATTLE PASS DIY FIRMWARE  (v2: interfaccia OLED animata)
 * Hardware: Seeed Studio XIAO ESP32-C3
 * Sensore:  TCRT5000 (riflessione IR) collegato al pin D1
 * Display:  OLED 0.91" 128x32 I2C SSD1306 (opzionale, auto-detect)
 * Batteria: LiPo 3.7V 802540 (8x25x40mm) 550mAh — ricaricabile via USB-C
 *
 * PROTOCOLLO BLE (ASCII) — invariato rispetto alla v1:
 *   TX (Notify → App):
 *     "LIVE:<rpm>"       → RPM durante il lancio (ora max ~25 al secondo)
 *     "RT:<rpm>"         → Picco massimo RPM a fine lancio
 *     "SYNC_COUNT:<n>"   → Quanti lanci offline ci sono in memoria
 *     "S:<rpm>"          → Singolo lancio offline (inviato in sequenza)
 *     "SYNC_END"         → Fine trasmissione lanci offline
 *     "BTN_SYNC"         → Pulsante fisico premuto, l'app avvia il sync
 *   RX (Write ← App):
 *     "SYNC"             → L'app chiede i lanci offline salvati in NVS
 *     "CLEAR"            → L'app chiede di cancellare lo storage offline
 *
 * NOVITA' DELLA v2:
 *   - Display animato: boot, attesa con bey che gira, barra RPM live,
 *     conteggio animato del risultato, lampo "NEW BEST", banner BLE, sync.
 *   - Ultimo lancio e record restano salvati in NVS: dopo un riavvio il
 *     display mostra ancora l'ultimo valore.
 *   - Il display e l'I2C sono toccati SOLO dal loop() (prima anche dai callback
 *     BLE, rischio di conflitti). I callback impostano solo dei flag.
 *   - L'invio SYNC non blocca più lo stack BLE: parte dal loop() un lancio
 *     ogni 60 ms.
 *   - Gli impulsi del sensore finiscono in un buffer circolare e il loop li
 *     legge tutti: niente impulsi persi mentre il display si ridisegna.
 *     RPM = media mobile su 4 intervalli (riduce il rumore).
 *   - LIVE limitato a ~25 messaggi al secondo.
 *
 * Compatibilità: arduino-esp32 2.0.x (l'API BLE della 3.x è diversa).
 * Scheda: XIAO_ESP32C3, "USB CDC On Boot: Enabled". Libreria: U8g2.
 * =============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <math.h>


// =============================================================================
// PINOUT — XIAO ESP32-C3
// =============================================================================
#define OLED_SDA 6          // GPIO6  = D4 (I2C SDA)
#define OLED_SCL 7          // GPIO7  = D5 (I2C SCL)

#define SENSOR_PIN D1       // Segnale digitale dal TCRT5000 (INPUT_PULLUP)
#define LED_BLE    D2       // LED blu: stato BLE (solo senza display)
#define LED_LAUNCH D3       // LED verde: lancio attivo (solo senza display)
#define BUTTON_PIN D6       // Pulsante frontale (INPUT_PULLUP, attivo LOW)


// =============================================================================
// COSTANTI SENSORE E BLE
// =============================================================================
#define PULSES_PER_REV 6        // DA CALIBRARE per ogni lanciatore (String / Winder)
#define DEBOUNCE_MICROS 250     // ~40.000 RPM di tetto teorico
#define AVG_WINDOW 4            // intervalli mediati per calcolare l'RPM
#define RING_SIZE 64            // capienza del buffer degli impulsi
#define LIVE_INTERVAL_MS 40     // max ~25 notifiche LIVE al secondo
#define MIN_VALID_RPM 1200      // sotto questa soglia il lancio è scartato
#define MAX_VALID_RPM 40000.0f  // sopra è rumore elettrico
#define GAUGE_FULL_RPM 20000.0f // fondoscala della barra sul display

#define SERVICE_UUID   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_TX_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"  // Notify (ESP32 → App)
#define CHAR_RX_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a9"  // Write  (App → ESP32)


// =============================================================================
// OGGETTI GLOBALI
// =============================================================================
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

Preferences preferences;    // lanci offline (namespace "bbx_offline")
Preferences uiPrefs;        // ultimo lancio e record (namespace "bbx_ui")

BLECharacteristic* pTxChar = NULL;
volatile bool deviceConnected = false;
bool hasDisplay = false;

// Flag impostati dai callback BLE e letti SOLO dal loop()
volatile bool syncRequested = false;


// =============================================================================
// IMPULSI DEL SENSORE — buffer circolare riempito dall'ISR
// (la C3 ha un solo core: per un produttore (ISR) e un consumatore (loop)
//  con indici a 8 bit non serve un mutex)
// =============================================================================
volatile unsigned long lastPulseTime = 0;
volatile unsigned long ringBuf[RING_SIZE];
volatile uint8_t ringHead = 0;
uint8_t ringTail = 0;

void IRAM_ATTR handleSensorInterrupt() {
  unsigned long now = micros();
  unsigned long interval = now - lastPulseTime;
  if (interval > DEBOUNCE_MICROS) {
    ringBuf[ringHead] = interval;
    ringHead = (uint8_t)((ringHead + 1) % RING_SIZE);
    lastPulseTime = now;
  }
}


// =============================================================================
// STATO DEL LANCIO
// =============================================================================
float maxRPM = 0.0;
float liveRPM = 0.0;
bool  isLaunching = false;
unsigned long winBuf[AVG_WINDOW];
uint8_t winIdx = 0, winCount = 0;
unsigned long lastLiveMs = 0;


// =============================================================================
// STATO SYNC OFFLINE (macchina a stati eseguita dal loop)
// =============================================================================
bool syncActive = false;
unsigned int syncCount = 0, syncIndex = 0;
unsigned long lastSyncMs = 0;


// =============================================================================
// INTERFACCIA OLED
//   BOOT   → logo che scorre + bey che accelera
//   IDLE   → bey che gira piano + ULTIMO lancio (resta finché non ne fai un altro)
//   LIVE   → bey veloce + barra RPM + valore in tempo reale
//   RESULT → conteggio animato fino al picco, lampo, "NEW BEST!" se record
//   SYNC   → barra di avanzamento del download offline
// =============================================================================
enum UiState { UI_BOOT, UI_IDLE, UI_LIVE, UI_RESULT, UI_SYNC };

UiState uiState = UI_BOOT;
unsigned long uiStateSince = 0;
unsigned long uiLastFrame = 0;
float uiAngle = 0.0f;

float lastLaunchRPM = 0.0f;     // 0 = nessun lancio ancora
float bestRPM = 0.0f;
bool  lastWasOffline = false;
bool  lastWasBest = false;
bool  memFull = false;
unsigned int sessionLaunches = 0;

const char* bannerText = NULL;  // messaggio temporaneo nella riga alta
unsigned long bannerUntil = 0;

void uiSetState(UiState s) {
  uiState = s;
  uiStateSince = millis();
}

void uiBanner(const char* text, unsigned long ms) {
  bannerText = text;
  bannerUntil = millis() + ms;
}

float clamp01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
float easeOutCubic(float t) { float u = 1.0f - clamp01(t); return 1.0f - u * u * u; }

// Bey stilizzato: cerchio esterno, perno centrale, 3 pale che ruotano
void drawBey(int cx, int cy, int r, float ang) {
  u8g2.drawCircle(cx, cy, r);
  u8g2.drawDisc(cx, cy, 2);
  for (int k = 0; k < 3; k++) {
    float a = ang + k * 2.0944f;                       // 120° tra le pale
    int tx = cx + (int)lroundf(cosf(a + 0.45f) * (r - 1));
    int ty = cy + (int)lroundf(sinf(a + 0.45f) * (r - 1));
    int bx = cx + (int)lroundf(cosf(a + 1.45f) * (r * 0.45f));
    int by = cy + (int)lroundf(sinf(a + 1.45f) * (r * 0.45f));
    int cx2 = cx + (int)lroundf(cosf(a - 0.35f) * (r * 0.45f));
    int cy2 = cy + (int)lroundf(sinf(a - 0.35f) * (r * 0.45f));
    u8g2.drawTriangle(tx, ty, bx, by, cx2, cy2);
  }
}

// Numero grande + "RPM" piccolo
void drawRpmBig(float v, int x, int base) {
  char buf[10];
  snprintf(buf, sizeof(buf), "%d", (int)(v + 0.5f));
  u8g2.setFont(u8g2_font_profont22_tn);
  u8g2.drawStr(x, base, buf);
  int w = u8g2.getStrWidth(buf);
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(x + w + 3, base, "RPM");
}

// Indicatore BLE in alto a destra: "BT" in negativo = connesso, puntino lampeggiante = in attesa
void drawBleIcon(unsigned long now) {
  u8g2.setFont(u8g2_font_5x7_tr);
  if (deviceConnected) {
    u8g2.drawBox(114, 0, 14, 9);
    u8g2.setDrawColor(0);
    u8g2.drawStr(117, 7, "BT");
    u8g2.setDrawColor(1);
  } else if ((now / 500) % 2 == 0) {
    u8g2.drawDisc(124, 4, 2);
  }
}

// Riga alta: banner temporaneo oppure l'etichetta indicata
void drawTopLabel(const char* label, unsigned long now) {
  u8g2.setFont(u8g2_font_5x7_tr);
  if (bannerText != NULL && now < bannerUntil) u8g2.drawStr(32, 7, bannerText);
  else u8g2.drawStr(32, 7, label);
}

void uiOnLaunchStart() {
  liveRPM = 0.0f;
  uiSetState(UI_LIVE);
}

void uiOnLaunchEnd(float peak, bool offline, bool full) {
  lastLaunchRPM = peak;
  lastWasOffline = offline;
  memFull = full;
  sessionLaunches++;
  lastWasBest = peak > bestRPM;
  if (lastWasBest) bestRPM = peak;
  uiPrefs.begin("bbx_ui", false);
  uiPrefs.putFloat("last", lastLaunchRPM);
  uiPrefs.putFloat("best", bestRPM);
  uiPrefs.end();
  uiSetState(UI_RESULT);
}

void uiOnLaunchAbort() {
  if (uiState == UI_LIVE) uiSetState(UI_IDLE);
}

void uiUpdate() {
  if (!hasDisplay) return;
  unsigned long now = millis();
  unsigned long frameMs = (uiState == UI_LIVE) ? 33 : 50;   // ~30 fps live, ~20 fps altrove
  if (now - uiLastFrame < frameMs) return;
  float dt = (now - uiLastFrame) / 1000.0f;
  if (dt > 0.1f) dt = 0.1f;
  uiLastFrame = now;

  // Banner a ogni cambio di connessione
  static bool prevConn = false;
  if (deviceConnected != prevConn) {
    prevConn = deviceConnected;
    uiBanner(deviceConnected ? "BLE CONNESSO" : "BLE SCOLLEGATO", 1500);
  }

  // Transizioni automatiche
  if (uiState == UI_BOOT && now - uiStateSince > 1700) uiSetState(UI_IDLE);
  if (uiState == UI_RESULT && now - uiStateSince > 4500) uiSetState(UI_IDLE);
  if (syncActive && uiState != UI_SYNC) uiSetState(UI_SYNC);
  if (!syncActive && uiState == UI_SYNC) uiSetState(UI_IDLE);

  float t = (now - uiStateSince) / 1000.0f;

  // Velocità di rotazione dell'icona (rad/s): solo effetto visivo, non è l'RPM reale
  float spin = 1.5f;
  if (uiState == UI_BOOT)        spin = 2.0f + 16.0f * t;
  else if (uiState == UI_LIVE)   spin = 3.0f + clamp01(liveRPM / GAUGE_FULL_RPM) * 28.0f;
  else if (uiState == UI_RESULT) spin = 1.5f + 14.0f * expf(-2.0f * t);
  else if (uiState == UI_SYNC)   spin = 5.0f;
  uiAngle += spin * dt;
  if (uiAngle > 6.2832f) uiAngle -= 6.2832f;

  u8g2.clearBuffer();
  u8g2.setDrawColor(1);
  drawBey(14, 16, 12, uiAngle);

  switch (uiState) {

    case UI_BOOT: {
      float x = 128.0f - (128.0f - 34.0f) * easeOutCubic(t / 0.6f);
      u8g2.setFont(u8g2_font_profont15_tr);
      u8g2.drawStr((int)x, 18, "XBATTLE");
      if (t > 0.7f) {
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(34, 30, "BATTLE PASS PRO");
      }
      break;
    }

    case UI_IDLE: {
      drawTopLabel(lastLaunchRPM > 0 ? "ULTIMO LANCIO" : "PRONTO AL LANCIO", now);
      drawBleIcon(now);
      if (lastLaunchRPM > 0) {
        drawRpmBig(lastLaunchRPM, 32, 30);
      } else {
        u8g2.setFont(u8g2_font_profont15_tr);
        u8g2.drawStr(32, 27, "LANCIA!");
      }
      break;
    }

    case UI_LIVE: {
      // barra RPM in alto
      u8g2.drawFrame(32, 0, 82, 7);
      int fill = (int)(clamp01(liveRPM / GAUGE_FULL_RPM) * 78.0f);
      if (fill > 0) u8g2.drawBox(34, 2, fill, 3);
      drawBleIcon(now);
      drawRpmBig(liveRPM, 32, 30);
      break;
    }

    case UI_RESULT: {
      float prog = clamp01(t / 0.7f);
      float shown = lastLaunchRPM * easeOutCubic(prog);
      const char* label = "LANCIO";
      char lbl[16];
      if (memFull) label = "MEMORIA PIENA";
      else if (lastWasOffline) label = "SALVATO OFFLINE";
      else { snprintf(lbl, sizeof(lbl), "LANCIO #%u", sessionLaunches); label = lbl; }
      if (lastWasBest && t > 0.7f && t < 3.2f) {
        // "NEW BEST!" lampeggiante
        u8g2.setFont(u8g2_font_5x7_tr);
        if ((now / 250) % 2 == 0) u8g2.drawStr(32, 7, "** NEW BEST! **");
      } else {
        drawTopLabel(label, now);
      }
      drawBleIcon(now);
      drawRpmBig(shown, 32, 30);
      // lampo in negativo nel momento in cui il conteggio arriva al picco
      if (t >= 0.7f && t < 0.82f) {
        u8g2.setDrawColor(2);
        u8g2.drawBox(0, 0, 128, 32);
        u8g2.setDrawColor(1);
      }
      break;
    }

    case UI_SYNC: {
      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.drawStr(32, 7, "SYNC OFFLINE");
      u8g2.drawFrame(32, 12, 94, 8);
      int fill = (syncCount == 0) ? 90 : (int)(90.0f * syncIndex / syncCount);
      if (fill > 0) u8g2.drawBox(34, 14, fill, 4);
      char buf[20];
      snprintf(buf, sizeof(buf), "%u / %u", syncIndex, syncCount);
      u8g2.setFont(u8g2_font_profont15_tr);
      u8g2.drawStr(32, 31, buf);
      break;
    }
  }

  u8g2.sendBuffer();
}


// =============================================================================
// NVS — saveOfflineLaunch()
// Ritorna true se il lancio è stato salvato, false se la memoria è piena.
// Limite massimo: 500 lanci.
// =============================================================================
bool saveOfflineLaunch(float rpm) {
  preferences.begin("bbx_offline", false);
  unsigned int count = preferences.getUInt("count", 0);

  if (count >= 500) {
    preferences.end();
    return false;
  }

  String key = "launch_" + String(count);
  preferences.putFloat(key.c_str(), rpm);
  preferences.putUInt("count", count + 1);
  preferences.end();

  if (!hasDisplay) {
    for (int i = 0; i < 2; i++) {
      digitalWrite(LED_LAUNCH, LOW);  delay(100);
      digitalWrite(LED_LAUNCH, HIGH); delay(100);
    }
  }
  return true;
}


// =============================================================================
// CALLBACK BLE — NIENTE display e NIENTE delay qui dentro: solo flag
// =============================================================================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    if (!hasDisplay) digitalWrite(LED_BLE, HIGH);
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    if (!hasDisplay) digitalWrite(LED_BLE, LOW);
    BLEDevice::startAdvertising();
  }
};

class MyCommCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    std::string rxVal = pChar->getValue();

    if (rxVal == "SYNC") {
      syncRequested = true;                    // lo esegue il loop()
    }
    else if (rxVal == "CLEAR") {
      preferences.begin("bbx_offline", false);
      preferences.clear();
      preferences.putUInt("count", 0);
      preferences.end();
    }
  }
};


// =============================================================================
// SYNC OFFLINE — macchina a stati, un messaggio ogni 60 ms, chiamata dal loop()
// =============================================================================
void syncStep() {
  // Un SYNC già in corso viene interrotto se l'app si scollega
  if (syncActive && !deviceConnected) {
    preferences.end();
    syncActive = false;
    return;
  }

  if (syncRequested && !syncActive && deviceConnected) {
    syncRequested = false;
    preferences.begin("bbx_offline", true);
    syncCount = preferences.getUInt("count", 0);
    syncIndex = 0;
    pTxChar->setValue(("SYNC_COUNT:" + String(syncCount)).c_str());
    pTxChar->notify();
    syncActive = true;
    lastSyncMs = millis() + 40;                // totale ~100 ms prima del primo lancio
    return;
  }
  syncRequested = false;

  if (!syncActive) return;
  if (millis() - lastSyncMs < 60) return;
  lastSyncMs = millis();

  if (syncIndex < syncCount) {
    String key = "launch_" + String(syncIndex);
    float rpmVal = preferences.getFloat(key.c_str(), 0.0);
    pTxChar->setValue(("S:" + String(rpmVal, 0)).c_str());
    pTxChar->notify();
    syncIndex++;
  } else {
    pTxChar->setValue("SYNC_END");
    pTxChar->notify();
    preferences.end();
    syncActive = false;
  }
}


// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);

  // Auto-detect display OLED (indirizzo SSD1306 = 0x3C)
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0) {
    hasDisplay = true;
    u8g2.begin();
    u8g2.setBusClock(400000);                  // I2C veloce: ridisegno più rapido
    uiSetState(UI_BOOT);
  } else {
    hasDisplay = false;
    pinMode(LED_BLE, OUTPUT);
    pinMode(LED_LAUNCH, OUTPUT);
    digitalWrite(LED_LAUNCH, HIGH);
  }

  // Ultimo lancio e record salvati
  uiPrefs.begin("bbx_ui", true);
  lastLaunchRPM = uiPrefs.getFloat("last", 0.0f);
  bestRPM = uiPrefs.getFloat("best", 0.0f);
  uiPrefs.end();

  pinMode(SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), handleSensorInterrupt, FALLING);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Stack BLE
  BLEDevice::init("BBX_BattlePass_PRO");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pTxChar = pService->createCharacteristic(
    CHAR_TX_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxChar->addDescriptor(new BLE2902());

  BLECharacteristic* pRxChar = pService->createCharacteristic(
    CHAR_RX_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxChar->setCallbacks(new MyCommCallbacks());

  pService->start();
  BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();
}


// =============================================================================
// PULSANTE FRONTALE — debounce software
// =============================================================================
bool          lastButtonState        = HIGH;
unsigned long lastButtonChangeMillis = 0;
const unsigned long BUTTON_DEBOUNCE_MS = 50;

void checkButton() {
  bool currentState = digitalRead(BUTTON_PIN);

  if (currentState != lastButtonState) {
    lastButtonChangeMillis = millis();
    lastButtonState = currentState;
  }

  if (currentState == LOW && (millis() - lastButtonChangeMillis) > BUTTON_DEBOUNCE_MS) {
    static unsigned long lastActionMillis = 0;

    if (millis() - lastActionMillis > 1000) {
      lastActionMillis = millis();

      if (deviceConnected && pTxChar != NULL) {
        pTxChar->setValue("BTN_SYNC");
        pTxChar->notify();
        uiBanner("SYNC (PULSANTE)", 1500);
      } else {
        uiBanner("NON CONNESSO", 1500);
      }
    }
  }
}


// =============================================================================
// ELABORAZIONE IMPULSI — svuota il buffer circolare riempito dall'ISR
// =============================================================================
void processPulses() {
  bool gotNew = false;

  while (ringTail != ringHead) {
    unsigned long interval = ringBuf[ringTail];
    ringTail = (uint8_t)((ringTail + 1) % RING_SIZE);

    if (!isLaunching) {
      // Primo impulso: inizia il lancio. Il suo intervallo copre il tempo di
      // inattività, quindi non è una misura valida e viene scartato.
      isLaunching = true;
      winIdx = 0;
      winCount = 0;
      maxRPM = 0.0f;
      if (!hasDisplay) digitalWrite(LED_LAUNCH, LOW);
      uiOnLaunchStart();
      continue;
    }

    winBuf[winIdx] = interval;
    winIdx = (uint8_t)((winIdx + 1) % AVG_WINDOW);
    if (winCount < AVG_WINDOW) winCount++;
    if (winCount < AVG_WINDOW) continue;       // finestra non ancora piena

    unsigned long sum = 0;
    for (uint8_t i = 0; i < AVG_WINDOW; i++) sum += winBuf[i];
    float avgInterval = (float)sum / AVG_WINDOW;

    // RPM = (60.000.000 µs / intervallo medio) / impulsi per giro
    float rpm = (60000000.0f / avgInterval) / PULSES_PER_REV;

    if (rpm < MAX_VALID_RPM) {
      if (rpm > maxRPM) maxRPM = rpm;
      liveRPM = rpm;
      gotNew = true;
    }
  }

  // Streaming live all'app, limitato a ~25 Hz
  if (gotNew && deviceConnected && liveRPM > MIN_VALID_RPM &&
      millis() - lastLiveMs >= LIVE_INTERVAL_MS) {
    lastLiveMs = millis();
    pTxChar->setValue(("LIVE:" + String(liveRPM, 0)).c_str());
    pTxChar->notify();
  }
}


// =============================================================================
// LOOP PRINCIPALE
// =============================================================================
void loop() {
  checkButton();

  // LED BLE lampeggiante (solo senza display)
  if (!deviceConnected && !hasDisplay) {
    static unsigned long _lastBlinkMs = 0;
    static bool          _ledState    = false;
    if (millis() - _lastBlinkMs >= 500) {
      _lastBlinkMs = millis();
      _ledState    = !_ledState;
      digitalWrite(LED_BLE, _ledState ? HIGH : LOW);
    }
  }

  syncStep();
  processPulses();

  // FINE LANCIO: nessun impulso per 400 ms
  if (micros() - lastPulseTime > 400000 && isLaunching) {
    if (maxRPM > MIN_VALID_RPM) {
      bool offline = !deviceConnected;
      bool full = false;
      if (!offline) {
        pTxChar->setValue(("RT:" + String(maxRPM, 0)).c_str());
        pTxChar->notify();
      } else {
        full = !saveOfflineLaunch(maxRPM);
      }
      uiOnLaunchEnd(maxRPM, offline, full);
    } else {
      uiOnLaunchAbort();
    }
    isLaunching = false;
    maxRPM = 0.0f;
    if (!hasDisplay) digitalWrite(LED_LAUNCH, HIGH);
  }

  uiUpdate();
  delay(2);
}
