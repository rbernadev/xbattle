#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include <map>
#define IRAM_ATTR
#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 2
#define OUTPUT 1
#define FALLING 2
#define D1 3
#define D2 4
#define D3 5
#define D6 21
extern unsigned long g_micros;
inline unsigned long micros(){ return g_micros; }
inline unsigned long millis(){ return g_micros/1000; }
inline void delay(unsigned long ms){ g_micros += ms*1000; }
inline void pinMode(int,int){}
inline void digitalWrite(int,int){}
extern int g_button;
inline int digitalRead(int){ return g_button; }
inline int digitalPinToInterrupt(int p){ return p; }
inline void attachInterrupt(int,void(*)(),int){}
struct SerialStub { void begin(int){} } ;
extern SerialStub Serial;
class String {
public:
  std::string s;
  String(){}
  String(const char* c):s(c){}
  String(int v):s(std::to_string(v)){}
  String(unsigned int v):s(std::to_string(v)){}
  String(float v,int d){ char b[32]; snprintf(b,sizeof b,"%.*f",d,v); s=b; }
  String operator+(const String&o) const { String r; r.s=s+o.s; return r; }
  const char* c_str() const { return s.c_str(); }
};
inline String operator+(const char* a,const String&b){ String r(a); return r+b; }
