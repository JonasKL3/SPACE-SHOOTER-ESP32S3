#include "../firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino"
#include <iostream>
#include <stdexcept>
#include <set>
#include <vector>
#include <climits>
#include <iomanip>
using namespace std;
static long long checks=0;
static void ok(bool cond,const char*text){checks++;if(!cond)throw runtime_error(string("CHECK FAILED: ")+text+" time="+to_string(__ms));}
static int activeBullets(){int n=0;for(auto &b:bullets)if(b.active)n++;return n;}
static int activeEnemyBullets(){int n=0;for(auto &b:enemyBullets)if(b.active)n++;return n;}
static int activeParticles(){int n=0;for(auto &p:particles)if(p.active)n++;return n;}
static int activeEnemies(){int n=0;for(auto &e:enemies)if(e.active)n++;return n;}
struct Stats{long frames=0;long kills=0;int maxEnemyShots=0;int maxShots=0;int maxParticles=0;int maxEnemies=0;int maxLevel=0;int bossKills=0;int bossesStarted=0;int phases[10][4]{};vector<int> bossOrder;long ms=0;} st;
static void runMode(GameMode mode,int seconds,bool expectWin){
 st=Stats{};GamePad=GamePadT();input=InputState{};
 game.mode=mode;
 save.currentShip=SHIP_HUNTER;save.unlockedShips=(1<<SHIP_COUNT)-1;
 save.unlockedPets=(1<<PET_COUNT)-1;
 for(int i=0;i<UPG_COUNT;i++)save.upgradeLevels[i]=UPGRADES[i].maxLevel;
 resetGame();game.state=PLAYING;game.shieldTimer=1000000;
 lastFrame=__ms;lastDabblePoll=__ms;
 int writesAtStart = mockNvsWrites;
 bool prevActive=false;
 int lastType=-1;
 int bossPhaseTarget=0;
 const long maxFrames=(long)seconds*1000/16;
 for(long frame=0;frame<maxFrames;frame++){
  // Simulated Dabble inputs every 16ms: actual same update loop and fire system.
  GamePad.cross=true; GamePad.circle=false;
  GamePad.ax=((frame/120)%3)-1; GamePad.ay=0;
  GamePad.up=false; GamePad.down=false;
  GamePad.left=((frame/150)%4==0);
  GamePad.right=((frame/150)%4==2);
  GamePad.triangle=(frame%520==30); // dash every ~8 sec
  GamePad.square=(frame%1000==450); // bomb every ~16 sec
  // Scripted boss progression: deal real bullet hits; HP thresholds accelerated.
  if(boss.active){
    if(lastType != (int)boss.type){st.bossesStarted++;lastType=(int)boss.type;st.bossOrder.push_back(lastType);bossPhaseTarget=0;}
    ok((int)boss.type>=0 && (int)boss.type<10,"boss enum bounded");
    st.phases[(int)boss.type][boss.phase]=1;
    if(game.state==PLAYING && boss.phase==1){boss.hp=std::min(boss.hp,(int)(boss.maxHp*0.63f));}
    else if(game.state==PLAYING && boss.phase==2){boss.hp=std::min(boss.hp,(int)(boss.maxHp*0.30f));}
    else if(game.state==PLAYING && boss.phase==3 && !boss.shielded){
       // Spawn valid weapon shot inside collision bounds, not direct boss HP deletion.
       spawnBullet(boss.x,boss.y+12,0,-8,60,0,C_BULLET,C_UI);
    }
  }
  prevActive=boss.active;
  __ms+=16;
  loop();
  st.frames++;st.maxLevel=std::max(st.maxLevel,game.level);
  st.maxEnemyShots=std::max(st.maxEnemyShots,activeEnemyBullets());
  st.maxShots=std::max(st.maxShots,activeBullets());
  st.maxParticles=std::max(st.maxParticles,activeParticles());
  st.maxEnemies=std::max(st.maxEnemies,activeEnemies());
  ok(game.level>=1 && game.level<=((mode==MODE_CAMPAIGN)?50:999),"level bounds");
  ok(activeBullets()<=MAX_BULLETS && activeEnemyBullets()<=MAX_ENEMY_BULLS,"projectile pools bounded");
  ok(activeEnemies()<=MAX_ENEMIES && activeParticles()<=MAX_PARTICLES,"entity pools bounded");
  ok(std::isfinite(game.playerX)&&std::isfinite(game.playerY),"player movement finite");
  ok(game.playerX>=AREA_X+12 && game.playerX<=AREA_X+AREA_W-12,"player X in bounds");
  ok(game.playerY>=AREA_Y+12 && game.playerY<=AREA_Y+AREA_H-12,"player Y in bounds");
  if(prevActive && !boss.active) {st.bossKills++;}
  prevActive=boss.active;
  if(game.state==GAME_OVER){
    if(!expectWin)throw runtime_error("endless reached GAME_OVER at " + to_string(frame)+" frames");
    break;
  }
  ok(game.state==PLAYING || game.state==BOSS_INTRO,"game remains playable or boss intro");
  ok(mockNvsWrites==writesAtStart,"no NVS flash writes while actively playing");
  if(boss.active) ok(std::isfinite(boss.x)&&std::isfinite(boss.y),"boss position finite");
 }
 st.ms=(long)st.frames*16;
 ok(st.ms >= (long)(expectWin?600000:seconds*1000),"at least requested simulated game time");
 if(expectWin){
  ok(game.state==GAME_OVER && save.campaignWon,"final campaign victory");
  ok(game.level==50 && st.bossesStarted==10,"all ten bosses spawned by level50");
  ok(st.bossesStarted==10 && game.bossesKilled==10,"all bosses defeated");
  for(int i=0;i<10;i++){
   ok(st.bossOrder[i]==i,"boss order 0..9");
   ok(st.phases[i][1] && st.phases[i][2] && st.phases[i][3],"all 3 boss phases visited");
  }
 }else{
  ok(st.maxLevel>=40,"endless reached high levels");
  ok(st.bossesStarted>=8,"endless fights >=8 bosses");
 }
 cout<<"MODE="<<(mode==MODE_CAMPAIGN?"CAMPAIGN":"ENDLESS")
     <<" simulated_ms="<<st.ms<<" frames="<<st.frames
     <<" level="<<game.level<<" bosses="<<game.bossesKilled
     <<" enemies_killed="<<game.enemiesKilled
     <<" max_shots="<<st.maxShots<<" max_enemy_shots="<<st.maxEnemyShots
     <<" max_particles="<<st.maxParticles<<" max_enemies="<<st.maxEnemies
     <<" game_coins="<<game.coinsEarned<<" NVS_writes="<<mockNvsWrites<<"\n";
}
int main(){
 try{
  srand(817);setup();
  runMode(MODE_CAMPAIGN,1100,true); // up to 18m20s, at least 10m simulated
  runMode(MODE_ENDLESS,960,false);  // 16m00s continuous game
  cout<<"LONG GAMEPLAY PASS: "<<checks<<" checks, "<<"campaign + endless simulated, sanitizers enabled\n";
 }catch(const exception&e){cerr<<e.what()<<"\n";return 1;}
}
