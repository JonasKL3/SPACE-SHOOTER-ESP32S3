#include "../firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino"
#include <iostream>
#include <stdexcept>
#include <string>
static long long checks=0;
static void ck(bool x,const char*m){checks++;if(!x)throw std::runtime_error(std::string("FAIL: ")+m);}
static void cleanGame(){GamePad=GamePadT();input=InputState{};game.mode=MODE_ENDLESS; resetGame();game.state=PLAYING;}
int main(){
 try{
  srand(41);
  // Old v5 NVS values are migrated only if no v6 namespace exists
  mockNvs["shooter:hiscore"]=10472;mockNvs["shooter:coins"]=3600;
  mockNvs["shooter:ship"]=2; mockNvs["shooter:shipunl"]=7;
  mockNvs["shooter:upg0"]=3; mockNvs["shooter:upg5"]=2;
  mockNvs["shooter:achv"]=3;mockNvs["shooter:camp50won"]=1;
  setup();
  ck(std::string(GAME_VERSION)=="v6.6.0","version follows user release");
  ck(game.state==MENU,"boot menu");
  ck(save.highScore==10472 && save.coins>=3600,"v5 save migrated and achievement rewards may be added");
  ck(save.currentShip==2 && save.upgradeLevels[0]==3 && save.upgradeLevels[5]==2,"v5 ship and upgrade migration");
  ck(save.campaignWon,"v5 campaign50 won migration");
  ck(mockNvs["shooter6:coins"]==3600 && mockNvs["shooter:coins"]==3600,"migration preserves legacy namespace");
  cleanGame();
  // the v5 bug in enemy type 5 must not return
  enemies[0]=Enemy{}; enemies[0].active=true;enemies[0].hp=6;enemies[0].maxHp=6;enemies[0].w=26;enemies[0].h=26;enemies[0].type=5;enemies[0].x=160;enemies[0].y=80;
  spawnBullet(160,88,0,-8,1,0,C_BULLET,C_UI);
  checkBulletHits();ck(enemies[0].hp==5,"armored enemy hit from below");
  cleanGame();
  enemies[0]=Enemy{}; enemies[0].active=true;enemies[0].hp=20;enemies[0].w=30;enemies[0].h=30;enemies[0].x=160;enemies[0].y=80;
  spawnBullet(160,80,0,-8,2,3,C_BULLET,C_UI);
  checkBulletHits();int h=enemies[0].hp;for(int k=0;k<20;k++)checkBulletHits();
  ck(h==18 && enemies[0].hp==h,"piercing bullet cannot repeatedly damage same enemy");
  cleanGame();
  boss.active=true;boss.type=BOSS_SENTINEL;boss.hp=40;boss.maxHp=40;boss.x=160;boss.y=80;boss.w=70;boss.h=50;
  spawnBullet(160,80,0,-8,2,3,C_BULLET,C_UI);
  checkBulletHits();h=boss.hp;for(int k=0;k<20;k++)checkBulletHits();
  ck(h==38 && boss.hp==h,"piercing bullet cannot repeatedly damage same boss");
  cleanGame();
  save.controlMode=CTRL_AUTO;save.invertY=false;
  float initialX=game.playerX;
  GamePad.ax=-7;GamePad.ay=0;GamePad.right=true;
  __ms+=35;pollDabble();updatePlayer();
  ck(game.playerX>initialX,"AUTO D-pad takes priority over contradictory analog");
  initialX=game.playerX;GamePad.right=false;
  __ms+=35;pollDabble();updatePlayer();
  ck(game.playerX<initialX,"AUTO analog works when D-pad released");
  cleanGame();
  // after damage: no instantaneous hit-kill from touching bullets for 0.9 s
  int lives=game.lives;damagePlayer(1,1);damagePlayer(1,1);
  ck(game.lives==lives-1,"damage iframe protects after a collision");
  __ms+=1000;damagePlayer(1,1);ck(game.lives==lives-2,"damage iframe expires");
  cleanGame();
  // all five purchased pets should work, not silently drop the last two
  save.unlockedPets=(1<<PET_COUNT)-1;resetGame();int active=0;bool seen[PET_COUNT]={};
  for(auto &p:pets){if(p.active){active++;seen[(int)p.type]=true;}}
  ck(active==PET_COUNT,"all purchased pets active");
  for(bool b:seen)ck(b,"pet type present");
  cleanGame();
  game.bombsLeft=1;boss.active=true;boss.hp=100;boss.maxHp=100;boss.type=BOSS_SENTINEL;
  enemies[0].active=true;enemies[0].hp=5;enemies[0].x=130;enemies[0].y=100;
  enemyBullets[0].active=true;int oldKills=save.totalKills;
  useBomb();ck(game.bombsLeft==0&&boss.hp==70,"bomb damages boss and consumes bomb");
  ck(!enemies[0].active && !enemyBullets[0].active && save.totalKills==oldKills+1,"bomb clears enemies/bullets and records kill");
  useBomb();ck(boss.hp==70,"bomb cannot be reused with zero remaining");
  cleanGame();
  save.highScore=555;game.score=555;onGameOver();ck(!game.newHighScore,"ties are not new records");
  cleanGame();
  save.highScore=555;game.score=556;onGameOver();ck(game.newHighScore&&save.highScore==556,"genuine highscore");
  cleanGame();
  game.mode=MODE_CAMPAIGN;game.level=50;game.score=1000;
  boss.active=true;boss.type=BOSS_VOID;boss.x=160;boss.y=70;int c0=save.coins;
  onBossKilled();ck(game.state==GAME_OVER && !boss.active && save.campaignWon,"final boss ends campaign");
  ck(game.coinsEarned>=90 && save.coins>=c0+90,"final reward credited when game over blocks pickup");
  ck(game.wonThisGame,"win flag set only on genuine boss victory");
  cleanGame();ck(!game.wonThisGame && save.campaignWon,"previous campaign win does not make new game a victory");
  cleanGame();
  save.coins=100;konamiActive=false;konamiProgress=0;game.state=SHOP;
  for(auto k:KONAMI_SEQ)feedKonami(k);
  ck(konamiActive && save.coins==999999,"Konami debug code");
  ck(popup.active && popup.isDebug && popup.idx==-1,"Konami popup initialized safely");
  cleanGame();
  // Prevent premature evolution after a long paused interval.
  game.level=1;GamePad=GamePadT();
  __ms+=16;loop();
  GamePad.start=true;__ms+=32;loop();ck(game.state==PAUSED,"pause input");
  unsigned long startLevel=game.levelStartTime;
  for(int n=0;n<500;n++){__ms+=100;loop();}
  ck(game.state==PAUSED && game.level==1,"pause freezes level progression");
  GamePad.start=false;__ms+=32;loop();GamePad.start=true;__ms+=32;loop();
  ck(game.state==PLAYING,"resume input");ck(game.level==1,"resume without skipped levels");
  ck(game.levelStartTime>=startLevel+50000,"paused timer offset restored");
  cleanGame();
  // Missiles should not occupy the projectile pool forever after losing a target.
  spawnBullet(160,120,0,0,1,0,C_BULLET,C_UI);
  bullets[0].guided=true; bullets[0].ageFrames=238;
  for(int i=0;i<5;i++)updateBullets();
  ck(!bullets[0].active,"orphan guided projectile expires");
  // Destructive RESET DATA must require a SECOND deliberate press, not menu selection.
  game.state=SETTINGS;settingsSelection=2;save.coins=789;GamePad=GamePadT();
  __ms+=32;loop(); // release any previous button edges
  GamePad.cross=true;__ms+=32;loop();
  ck(game.state==RESET_CONFIRM && save.coins==789,"RESET opens confirmation without deleting NVS");
  for(int i=0;i<20;i++){__ms+=32;loop();}
  ck(game.state==RESET_CONFIRM && save.coins==789,"held button cannot confirm reset");
  GamePad.cross=false;__ms+=32;loop();
  GamePad.cross=true;__ms+=32;loop();
  ck(game.state==SETTINGS && save.coins==0,"RESET deletes only on fresh second confirmation");
  initSaveDefaults();loadSave();
  ck(save.coins==0 && save.highScore==0,"v6 reset does not accidentally reimport v5 NVS data");

  std::cout<<"REGRESSION PASS: "<<checks<<" assertions; version="<<GAME_VERSION<<"\n";
 }catch(std::exception&e){std::cerr<<e.what()<<"\n";return 1;}
}
