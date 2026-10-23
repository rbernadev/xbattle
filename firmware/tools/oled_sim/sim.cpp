#include "Arduino.h"
#include "Wire.h"
#include <cstdarg>
unsigned long g_micros = 0; int g_button = 1; SerialStub Serial; WireStub Wire;
FILE* g_blelog = nullptr; FILE* g_framelog = nullptr;
#include "../../xbattle_firmware/xbattle_firmware.ino"
#include <vector>
struct Seg { double t0,t1,rpm0,rpm1; };
// profilo di un lancio: sale a 'peak' in 0.5 s poi scende a ~40% in 1.5 s
static std::vector<Seg> launch(double start,double peak){ return { {start,start+0.5,3000,peak},{start+0.5,start+2.0,peak,peak*0.4} }; }
int main(){
  g_blelog = fopen("ble.log","w"); g_framelog = fopen("frames.jsonl","w");
  setup();
  { Preferences p; p.begin("bbx_offline"); p.putUInt("count",24); for(int i=0;i<24;i++){ String k="launch_"+String(i); p.putFloat(k.c_str(), 9000+300*i);} }
  std::vector<Seg> segs; for(auto&s:launch(4.0,14200)) segs.push_back(s); for(auto&s:launch(9.5,16800)) segs.push_back(s);
  double nextPulseUs = -1; 
  bool connected=false, syncDone=false;
  while (g_micros < 17500000UL){
    double tsec = g_micros/1e6;
    if(!connected && tsec>2.5){ connected=true; deviceConnected=true; }
    if(connected && !syncDone && tsec>14.0){ syncDone=true; syncRequested=true; }
    // impulsi del sensore
    for(;;){
      double rpm=0; bool in=false;
      for(auto&s:segs) if(tsec>=s.t0 && tsec<s.t1){ double f=(tsec-s.t0)/(s.t1-s.t0); rpm=s.rpm0+(s.rpm1-s.rpm0)*f; in=true; }
      if(!in){ nextPulseUs=-1; break; }
      double iv = 60e6/(rpm*PULSES_PER_REV);
      if(nextPulseUs<0) nextPulseUs = g_micros;
      if(nextPulseUs > g_micros) break;
      unsigned long save=g_micros; g_micros=(unsigned long)nextPulseUs; handleSensorInterrupt(); g_micros=save;
      nextPulseUs += iv;
    }
    loop();
  }
  fclose(g_blelog); fclose(g_framelog); printf("best=%.0f last=%.0f\n",bestRPM,lastLaunchRPM); return 0;
}
