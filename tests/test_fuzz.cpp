#include "../firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino"
#include <iostream>
#include <stdexcept>
#include <string>
#include <random>
using namespace std;
static long long checks=0;
static void ck(bool c,const char*why){checks++;if(!c)throw runtime_error(string("FAIL: ")+why+" at simulated_ms="+to_string(__ms));}
int main(){
 try{
  srand(42);setup();
  const int seeds=5;
  long long sessionsTotal=0, frameTotal=0;
  for(int seed=0;seed<seeds;seed++){
    mt19937 r(1800+seed);auto pick=[&](int maxV){return (int)(r()%maxV);};
    save.currentShip=pick(SHIP_COUNT);
    save.unlockedShips=(1<<SHIP_COUNT)-1;
    save.unlockedPets=pick((1<<PET_COUNT));
    for(int i=0;i<UPG_COUNT;i++)save.upgradeLevels[i]=pick(UPGRADES[i].maxLevel+1);
    game.mode=MODE_ENDLESS;resetGame();game.state=PLAYING;
    lastFrame=__ms;lastDabblePoll=__ms;
    int games=0,bossesSeen=0;
    for(int frame=0;frame<42500;frame++){
      if(game.state==GAME_OVER){
         games++;game.mode=MODE_ENDLESS;resetGame();game.state=PLAYING;
      }
      // Damage and death are allowed; this exercise creates many fresh gameplay sessions.
      GamePad.cross=(pick(100)<75);GamePad.circle=(pick(100)<20);
      GamePad.square=(pick(700)==0);GamePad.triangle=(pick(280)==0);
      GamePad.up=(pick(100)<12);GamePad.down=(pick(100)<12);
      GamePad.left=(pick(100)<12);GamePad.right=(pick(100)<12);
      GamePad.ax=pick(15)-7;GamePad.ay=pick(15)-7;
      GamePad.start=false;GamePad.select=false;
      if(frame%600==0 && !boss.active && pick(10)<7){
         spawnBoss((BossType)pick(10));boss.y=AREA_Y+50;bossesSeen++;
      }
      // Controlled collision stress: repeatedly exercise legitimate enemy bullet -> player death/revive.
      if(frame%74==0) spawnEnemyBullet(game.playerX,game.playerY,0,0);
      __ms+=16;loop();frameTotal++;
      ck(game.level>=1 && game.level<=999,"level valid");
      ck(game.lives>=0 && game.lives<=9,"lives bounded");
      ck(game.bombsLeft>=0 && game.bombsLeft<=3,"bomb count bounded");
      ck(save.coins>=0 && save.coins<100000000,"wallet sane");
      ck(save.currentShip>=0 && save.currentShip<SHIP_COUNT,"ship index valid");
      ck(game.weapon>=WPN_DEFAULT && game.weapon<=WPN_MISSILE,"weapon index valid");
      if(game.state==PLAYING){
         ck(std::isfinite(game.playerX) && std::isfinite(game.playerY),"player finite");
         ck(game.playerX >= AREA_X + 12 && game.playerX<=AREA_X + AREA_W - 12,"player horizontal bounds");
         ck(game.playerY >= AREA_Y + 12 && game.playerY<=AREA_Y + AREA_H - 12,"player vertical bounds");
      }
    }
    sessionsTotal+=games;
    cout<<"FUZZ seed="<<seed<<" simulated_seconds="<<42500*16/1000
        <<" games_lost="<<games<<" random_boss_spawns="<<bossesSeen
        <<" game_state="<<(int)game.state<<"\n";
  }
  ck(sessionsTotal>=15,"fuzz tests should cover many GameOvers");
  cout<<"FUZZ PASS: "<<checks<<" assertions, "<<frameTotal
      <<" frames, "<<sessionsTotal<<" completed losing sessions, 56m40s cumulative simulated\n";
 }catch(const exception&e){cerr<<e.what()<<"\n";return 1;}
}
