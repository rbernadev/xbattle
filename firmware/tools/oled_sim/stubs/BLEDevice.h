#pragma once
#include <string>
#include <cstdio>
extern unsigned long g_micros;
struct BLEAdvertisingS { void addServiceUUID(const char*){} };
class BLECharacteristic;
class BLECharacteristicCallbacks { public: virtual void onWrite(BLECharacteristic*){} virtual ~BLECharacteristicCallbacks(){} };
struct BLE2902 {};
extern FILE* g_blelog;
class BLECharacteristic {
public:
  static const int PROPERTY_READ=1, PROPERTY_NOTIFY=2, PROPERTY_WRITE=4;
  std::string val; BLECharacteristicCallbacks* cb=nullptr;
  void setValue(const char* v){ val=v; }
  void setValue(const std::string& v){ val=v; }
  std::string getValue(){ return val; }
  void notify(){ if(g_blelog) fprintf(g_blelog,"%lu %s\n", g_micros/1000, val.c_str()); }
  void addDescriptor(BLE2902*){}
  void setCallbacks(BLECharacteristicCallbacks* c){ cb=c; }
};
struct BLEService { BLECharacteristic* createCharacteristic(const char*,int){ return new BLECharacteristic(); } void start(){} };
class BLEServer;
class BLEServerCallbacks { public: virtual void onConnect(BLEServer*){} virtual void onDisconnect(BLEServer*){} virtual ~BLEServerCallbacks(){} };
struct BLEServer { void setCallbacks(BLEServerCallbacks*){} BLEService* createService(const char*){ return new BLEService(); } };
struct BLEDevice {
  static void init(const char*){}
  static BLEServer* createServer(){ return new BLEServer(); }
  static BLEAdvertisingS* getAdvertising(){ static BLEAdvertisingS a; return &a; }
  static void startAdvertising(){}
};
