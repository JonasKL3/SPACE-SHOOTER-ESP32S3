#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <climits>
#include <cassert>
using std::abs;
#define TWO_PI 6.28318530717958647692f
#define PI 3.14159265358979323846f
#define TFT_WHITE 0xFFFF
#define TFT_BLACK 0x0000
#define TFT_YELLOW 0xFFE0
template<typename T,typename U,typename V> T constrain(T x,U a,V b){return x<(T)a?(T)a:(x>(T)b?(T)b:x);}
unsigned long __ms=0;
unsigned long millis(){return __ms;} unsigned long micros(){return __ms*1000;} void delay(int){}
long random(long a,long b){return a + (b>a? rand()%(b-a):0);}
void randomSeed(unsigned long){}
unsigned long esp_random(){return 1;}
int getCpuFrequencyMhz(){return 240;}
struct SerialT{
 void begin(int){}
 void println(){}
 template<class T> void println(T){}
 template<class...A> void printf(const char*,A...){}
} Serial;
struct ESPT{int getFreePsram(){return 3000000;} int getFreeHeap(){return 250000;}} ESP;
struct Preferences{
 void begin(const char*,bool){}
 int getInt(const char*,int d){return d;}
 bool getBool(const char*,bool d){return d;}
 void end(){}
 void putInt(const char*,int){}
 void putBool(const char*,bool){}
};
struct GamePadT{
 bool up=false,down=false,left=false,right=false,cross=false,circle=false,triangle=false,square=false,start=false,select=false;
 int ax=0,ay=0;
 bool isUpPressed(){return up;} bool isDownPressed(){return down;}
 bool isLeftPressed(){return left;} bool isRightPressed(){return right;}
 bool isCrossPressed(){return cross;} bool isCirclePressed(){return circle;}
 bool isTrianglePressed(){return triangle;} bool isSquarePressed(){return square;}
 bool isStartPressed(){return start;} bool isSelectPressed(){return select;}
 int getXaxisData(){return ax;} int getYaxisData(){return ay;}
} GamePad;
struct DabbleT{void processInput(){} void begin(const char*){}} Dabble;
struct TFT_eSPI{TFT_eSPI(int=0,int=0){} void init(){} void setRotation(int){} void fillScreen(int){}};
struct TFT_eSprite{
 TFT_eSprite(TFT_eSPI*){}
 void setColorDepth(int){} bool createSprite(int,int){return true;} void fillSprite(int){}
 uint16_t color565(int,int,int){return 0;}
 void drawPixel(int,int,int){} void drawCircle(int,int,int,int){} void fillTriangle(int,int,int,int,int,int,int){}
 void fillCircle(int,int,int,int){} void fillRect(int,int,int,int,int){} void drawRect(int,int,int,int,int){}
 void fillRoundRect(int,int,int,int,int,int){} void drawLine(int,int,int,int,int){}
 void setTextColor(int,int=0){} void setTextSize(int){} void setCursor(int,int){}
 void print(const char*){} void print(int){} template<class...A> void printf(const char*,A...){}
 void pushSprite(int,int){}
};

