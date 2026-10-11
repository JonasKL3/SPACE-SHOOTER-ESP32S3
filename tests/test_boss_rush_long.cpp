#include "../firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino"
#include <iostream>
#include <stdexcept>
#include <string>
static unsigned long long checks=0;
static void check(bool x,const char *msg){++checks;if(!x)throw std::runtime_error(msg);}
int main(){
 try {
  srand(883);
  mockNvs.clear();setup();
  save.campaignWon=true;save.currentShip=SHIP_HUNTER;
  save.unlockedShips=(1<<SHIP_COUNT)-1;
  for(int i=0;i<UPG_COUNT;i++)save.upgradeLevels[i]=UPGRADES[i].maxLevel;
  game.mode=MODE_BOSS_RUSH;resetGame();game.level=5;spawnBoss(BOSS_SENTINEL);game.state=BOSS_INTRO;
  game.shieldTimer=1000000;
  int expectedBoss=0;
  unsigned long lastSwitch=__ms;
  int enters=0;
  int introFrames=0;
  const unsigned long start=__ms;
  for(int frame=0;frame<56000;frame++){
   if(boss.active){
    check((int)boss.type>=0 && (int)boss.type<10,"valid boss type");
    if((int)boss.type!=expectedBoss){
      check((int)boss.type==expectedBoss+1,"bosses must be sequential");
      expectedBoss=(int)boss.type;lastSwitch=__ms;enters++;
    }
    if(game.state==BOSS_INTRO)introFrames++;
    // Each boss lives for 66+ seconds simulated, with real updateBoss and checkBulletHits.
    if(game.state==PLAYING && (__ms-lastSwitch)>66000){
      if(boss.phase==1)boss.hp=(int)(boss.maxHp*0.62f);
      else if(boss.phase==2)boss.hp=(int)(boss.maxHp*0.29f);
      else if(boss.phase==3) {
        boss.hp=std::min(boss.hp,1);
        boss.shielded=false;
        spawnBullet(boss.x,boss.y,0,0,20,0,C_BULLET,C_UI);
      }
    }
   }
   GamePad.cross=false;GamePad.circle=false;
   GamePad.ax=0;GamePad.ay=0;GamePad.up=false;GamePad.down=false;
   GamePad.right=false;GamePad.left=false;
   // Prevent scripted survivability powerups from overriding the test shield.
   game.shieldTimer=1000000;
   __ms+=16;loop();
   check(game.level<=50 && game.level>=5,"boss rush level bounds");
   check(game.bossesKilled<=10,"boss cap");
   int ecount=0;for(auto &e:enemies)if(e.active)ecount++;
   check(ecount==0,"no enemy waves in boss rush");
   if(game.state==GAME_OVER)break;
   check(game.state==BOSS_INTRO || game.state==PLAYING,"boss rush live states");
  }
  check(game.state==GAME_OVER && game.wonThisGame,"boss rush full victory");
  check(game.bossesKilled==10 && expectedBoss==9,"ten ordered boss kills");
  check((__ms-start)>=600000,"at least ten minutes of simulated boss rush");
  check(introFrames>=10*BOSS_INTRO_FRAMES - 20,"all 10 intros run");
  std::cout<<"BOSS RUSH LONG PASS simulated_ms="<<(__ms-start)
           <<" boss_kills="<<game.bossesKilled<<" intros="<<introFrames
           <<" checks="<<checks<<"\n";
 }catch(const std::exception &e){std::cerr<<"BOSS RUSH LONG FAIL: "<<e.what()<<" at="<<__ms<<" state="<<game.state<<" type="<<(int)boss.type<<" phase="<<boss.phase<<"\n";return 1;}
}
