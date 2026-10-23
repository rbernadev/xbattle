#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#define U8G2_R0 0
#define U8X8_PIN_NONE 0
enum { u8g2_font_5x7_tr=1, u8g2_font_profont15_tr=2, u8g2_font_profont22_tn=3 };
extern unsigned long g_micros;
extern FILE* g_framelog;
class U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C {
  int font=1; int col=1; std::string cmds;
  int cw() const { return font==1?5:(font==2?8:11); }
  void add(const char* fmt,...) __attribute__((format(printf,2,3)));
public:
  U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C(int,int){}
  void begin(){} void setBusClock(int){}
  void clearBuffer(){ cmds.clear(); }
  void setFont(int f){ font=f; }
  void setDrawColor(int c){ col=c; add("{\"o\":\"col\",\"c\":%d}",c); }
  int getStrWidth(const char* s){ return (int)strlen(s)*cw(); }
  void drawStr(int x,int y,const char* s){ add("{\"o\":\"str\",\"x\":%d,\"y\":%d,\"f\":%d,\"s\":\"%s\"}",x,y,font,s); }
  void drawBox(int x,int y,int w,int h){ add("{\"o\":\"box\",\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d}",x,y,w,h); }
  void drawFrame(int x,int y,int w,int h){ add("{\"o\":\"frame\",\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d}",x,y,w,h); }
  void drawCircle(int x,int y,int r){ add("{\"o\":\"circle\",\"x\":%d,\"y\":%d,\"r\":%d}",x,y,r); }
  void drawDisc(int x,int y,int r){ add("{\"o\":\"disc\",\"x\":%d,\"y\":%d,\"r\":%d}",x,y,r); }
  void drawTriangle(int a,int b,int c,int d,int e,int f){ add("{\"o\":\"tri\",\"p\":[%d,%d,%d,%d,%d,%d]}",a,b,c,d,e,f); }
  void sendBuffer(){ if(g_framelog) fprintf(g_framelog,"{\"t\":%lu,\"c\":[%s]}\n", g_micros/1000, cmds.c_str()); g_micros += 13000; }
};
inline void U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C::add(const char* fmt,...){
  char b[256]; va_list ap; va_start(ap,fmt); vsnprintf(b,sizeof b,fmt,ap); va_end(ap);
  if(!cmds.empty()) cmds+=","; cmds+=b;
}
