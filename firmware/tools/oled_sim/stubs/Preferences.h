#pragma once
#include <map>
#include <string>
class Preferences {
  static std::map<std::string,std::map<std::string,double>>& db(){ static std::map<std::string,std::map<std::string,double>> d; return d; }
  std::string ns;
public:
  bool begin(const char* n,bool=false){ ns=n; return true; }
  void end(){}
  void clear(){ db()[ns].clear(); }
  unsigned int getUInt(const char* k,unsigned int d=0){ auto&m=db()[ns]; return m.count(k)?(unsigned)m[k]:d; }
  float getFloat(const char* k,float d=0){ auto&m=db()[ns]; return m.count(k)?(float)m[k]:d; }
  void putUInt(const char* k,unsigned v){ db()[ns][k]=v; }
  void putFloat(const char* k,float v){ db()[ns][k]=v; }
};
