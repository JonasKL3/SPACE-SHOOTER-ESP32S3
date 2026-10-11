#include "../firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino"
#include <iostream>
#include <stdexcept>
#include <string>
#include <cmath>
using namespace std;
static int checks = 0;
static void expect(bool value,const char*msg){++checks;if(!value)throw runtime_error(string(msg)+" t="+to_string(__ms));}
static void fresh(){ GamePad=GamePadT(); input=InputState{};game.mode=MODE_CAMPAIGN; resetGame();game.state=PLAYING;}
int main(){
 try {
 srand(5656);setup();
 expect(std::string(GAME_VERSION)=="v6.6.0","integration version");
 expect(LEVEL_DURATION_MS==12000,"level 12s");
 expect(BOSS_INTRO_FRAMES==120,"intro 120 logic frames");
 expect(MAX_PETS==5,"five pets preserved");
 fresh();
 game.levelStartTime=__ms;
 __ms+=11984;updateTimers();expect(game.level==1,"not level up before 12s");
 __ms+=32;updateTimers();expect(game.level==2,"advance to level 2 at 12s");
 for(int expected=3;expected<=4;expected++){
   __ms+=12016;updateTimers();expect(game.level==expected,"automatic consecutive level progression");
 }
 // Type-3 stationary shooter: cannot freeze the transition forever.
 for (auto &e:enemies) e.active=false;
 enemies[0]=Enemy{};enemies[0].active=true;enemies[0].type=3;enemies[0].hp=3;
 enemies[0].w=24;enemies[0].h=20;enemies[0].y=80;enemies[0].x=140;
 game.lastEnemySpawn=__ms;
 __ms+=12016;updateTimers();
 expect(game.level==4 && game.bossPending,"boss preps after level4");
 expect(!boss.active && game.state==PLAYING,"boss not spawned before cleanup");
 __ms+=4000;updateTimers();
 expect(game.bossPending && !boss.active,"stationary enemy holds prelude briefly");
 __ms+=2001;updateTimers();
 expect(game.level==5 && !game.bossPending && boss.active,"bounded cleanup starts boss");
 expect(game.state==BOSS_INTRO && boss.y<AREA_Y,"intro begins above visible area");
 expect(!enemies[0].active,"stationary enemies cleared after grace");
 expect(boss.type==BOSS_SENTINEL,"correct first boss");
 int hp=boss.hp;
 for(int i=0;i<60;i++){__ms+=16; updateBossIntro();expect(game.state==BOSS_INTRO,"intro still active at first half");}
 expect(boss.y>AREA_Y-boss.h-30 && boss.y<AREA_Y+50,"boss eased midway");
 for(int i=60;i<120;i++){__ms+=16;updateBossIntro();}
 expect(game.state==PLAYING && boss.y==AREA_Y+50,"intro completes after 120 frames");
 expect(boss.hp==hp,"no boss damage during intro");
 onBossKilled();
 expect(game.level==5 && game.nextBossLevel==10,"post-boss stage state");
 __ms+=12016;updateTimers();expect(game.level==6,"postboss level6 reachable");
 fresh();
 save.upgradeLevels[UPG_START_BOMB]=2;game.bombsLeft=2;
 useBomb();expect(game.bombsLeft==1 && game.bombCooldown==BOMB_COOLDOWN_FRAMES,"bomb starts cooldown");
 useBomb();expect(game.bombsLeft==1,"can't use bomb during cooldown");
 for(int i=0;i<BOMB_COOLDOWN_FRAMES;i++){updateTimers();}
 expect(game.bombCooldown==0,"bomb cooldown expires");
 useBomb();expect(game.bombsLeft==0,"second bomb works after cooldown");
 // Splitter (type 7) creates two children only AFTER projectile loops finish.
 fresh();
 for(auto &e:enemies)e.active=false;
 enemies[0]=Enemy{};enemies[0].active=true;enemies[0].type=7;
 enemies[0].hp=1;enemies[0].maxHp=1;enemies[0].x=160;enemies[0].y=80;
 enemies[0].w=30;enemies[0].h=28;
 spawnBullet(160,80,0,-8,1,3,C_BULLET,C_UI);
 checkBulletHits();
 int children=0;for(auto &e:enemies)if(e.active){children++;expect(e.type==0,"child type correct");}
 expect(children==2 && pendingEnemySpawnCount==0,"deferred splitter child spawn exactly twice");
 expect(!bossKillPending,"boss cleanup not left pending");
 fresh();
 game.state=PAUSED;
 konamiActive=false;konamiProgress=0;save.coins=32;
 GamePad.cross=true;GamePad.select=true;
 __ms+=64;loop();
 expect(game.state==PAUSED,"debug select does not exit pause");
 expect(konamiActive && save.coins==999999,"X+SELECT activates debug");
 expect(popup.active && popup.isDebug && popup.idx==-1,"correct safe debug popup");
 __ms+=64;loop();expect(game.state==PAUSED,"hold combo cannot leave pause");
 GamePad.select=false;GamePad.cross=false; __ms+=64;loop();
 GamePad.select=true;__ms+=64;loop();
 expect(game.state==MENU,"select alone can still exit pause");
 fresh();
 const int savedCoins=save.coins; const int savedPets=save.unlockedPets;
 resetGame();expect(save.coins==savedCoins && save.unlockedPets==savedPets,"restarting game preserves NVS-backed state");
 cout<<"V6.5.6 FEATURE PASS: "<<checks<<" assertions, boss cleanup/intro, timer, bomb, debug and save regression\n";
 }catch(const exception&e){cerr<<"FAIL v656: "<<e.what()<<"\n";return 1;}
}
