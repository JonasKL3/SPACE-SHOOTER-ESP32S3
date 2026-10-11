#include "stubs/Arduino.h"
// Arduino preprocessor places this generated prototype before user-defined enums.
void feedKonami(int k);
#include "../firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino"
#include <cassert>
int main() {
  konamiActive=false;
  konamiProgress=0;
  save.coins=0;
  const int start[]={K_UP,K_UP,K_DOWN,K_DOWN,K_LEFT,K_RIGHT,K_LEFT,K_RIGHT,K_CROSS,K_CIRCLE};
  for(int k:start) feedKonami(k);
  assert(konamiActive && save.coins == 999999);
  return 0;
}
