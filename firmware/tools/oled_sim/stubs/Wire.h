#pragma once
struct WireStub { void begin(int,int){} void beginTransmission(int){} int endTransmission(){return 0;} void setClock(int){} };
extern WireStub Wire;
