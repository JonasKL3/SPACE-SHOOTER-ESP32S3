#include "../firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino"
#include <iostream>
#include <stdexcept>
#include <string>

static long long checks=0;
static void check(bool x, const char *msg) { ++checks; if(!x) throw std::runtime_error(msg); }
static void tick() { __ms+=32; loop(); }
static void release() { GamePad=GamePadT{}; tick(); tick(); }
static void cross(){GamePad.cross=true;tick();release();}
static void down(){GamePad.down=true;tick();release();}
static void right(){GamePad.right=true;tick();release();}
static void select(){GamePad.select=true;tick();release();}
int main(){
 try {
  srand(1001);
  mockNvs.clear();
  setup();
  check(std::string(GAME_VERSION)=="v6.6.0", "release version");
  check(MODE_ITEMS==3 && MENU_ITEMS==6 && SETTINGS_ITEMS==3, "menu counts");
  check(game.state==MENU && !bossRushUnlocked(), "boss rush starts locked");
  check(save.currentShip==0 && skinForShip(0)==0, "fresh skin defaults");
  // Actual navigation via GamePad edge-polls (not assigning Game.state directly).
  menuSelection=0; cross();
  check(game.state==MODE_SELECT, "mode selector opened");
  modeSelection=2;cross();
  check(game.state==MODE_SELECT, "locked boss rush cannot start");
  select(); check(game.state==MENU, "mode selector exits");
  // Each hull and four personal colorways must render through real drawing code.
  for (int ship=0; ship<SHIP_COUNT; ++ship) {
   for (int skin=0; skin<SKIN_COUNT; ++skin) {
    drawMechShip(ship,160,100,1,skin);
    drawMechShip(ship,160,100,2,skin);
    drawMechShip(ship,160,100,3,skin);
    check(shipSkinNames[ship][skin] && shipSkinNames[ship][skin][0],"skin with personal name");
   }
  }
  // Unlock five ships for UI. Change ship 2 and ship 4 skins independently.
  save.unlockedShips=(1<<SHIP_COUNT)-1;
  menuSelection=1;cross(); check(game.state==SHIP_SELECT, "ship selector from menu");
  shipSelection=2;right(); check(game.state==SKIN_SELECT && skinSelection==0,"right opens personal skins");
  skinSelection=3;cross(); check(game.state==SHIP_SELECT && skinForShip(2)==3,"ship 2 palette saved");
  shipSelection=4;right();check(game.state==SKIN_SELECT && skinSelection==0,"ship 4 starts separate skin");
  skinSelection=1;cross();check(skinForShip(4)==1 && skinForShip(2)==3,"ship skins independent");
  shipSelection=2;cross();check(save.currentShip==2 && save.currentSkin==3,"equipping restores personal palette");
  menuSelection=1;cross();shipSelection=4;cross();check(save.currentShip==4 && save.currentSkin==1,"other ship restores its palette");
  initSaveDefaults();loadSave();check(skinForShip(2)==3 && skinForShip(4)==1, "per-ship palettes survive NVS reload");
  // Legacy v6 save (single skin only) migrates to independent palettes if missing keys.
  mockNvs.clear();mockNvs["shooter6:hiscore"]=7; mockNvs["shooter6:skin"]=2;
  initSaveDefaults();loadSave();
  for(int id=0;id<SHIP_COUNT;id++) check(skinForShip(id)==2,"legacy skin fallback");
  // Debug achievement gives persistent unlock; reset revokes it.
  check(!bossRushUnlocked(),"no unlock from skin changes");
  unlockAchv(ACHV_KONAMI); saveAll();
  check(bossRushUnlocked(),"debug achievement unlocks boss rush");
  initSaveDefaults();loadSave();check(bossRushUnlocked(),"debug unlock saved permanently");
  // Launch from menu, confirming proper input routing and no normal wave spawning.
  game.state=MENU;menuSelection=0;release();cross();check(game.state==MODE_SELECT,"mode reopened");
  modeSelection=2;cross();check(game.mode==MODE_BOSS_RUSH && game.state==BOSS_INTRO,"boss rush starts");
  check(boss.active && boss.type==BOSS_SENTINEL && game.level==5, "starts with sentinel");
  game.state=PLAYING;
  __ms+=45000;
  game.lastEnemySpawn=0;
  updateTimers();
  int active=0;for(auto &e: enemies) if(e.active) active++;
  check(active==0 && game.level==5, "no time levels/spawns in rush");
  // Ten boss defeats with real progression and no campaign unlock.
  for(int k=0;k<10;k++){
    check(boss.active && (int)boss.type==k && game.level==(k+1)*5, "boss order 0..9");
    int reward=game.score;onBossKilled();
    check(game.bossesKilled==k+1 && game.score>reward, "boss kill count/reward");
    if(k<9){check(boss.active && boss.type==(BossType)(k+1) && game.state==BOSS_INTRO,"next boss cutscene");game.state=PLAYING;}
  }
  check(game.state==GAME_OVER && game.wonThisGame && game.bossesKilled==10,"rush final victory");
  check(!save.campaignWon,"rush completion cannot forge campaign victory");
  check(save.gamesPlayed>=1, "rush game statistics persisted");
  // Campaign can unlock rush without debug.
  resetSave();check(!bossRushUnlocked(), "reset revokes debug access");
  save.campaignWon=true;check(bossRushUnlocked(), "campaign completion opens rush");
  saveAll();initSaveDefaults();loadSave();check(bossRushUnlocked(), "campaign unlock survives reboot");
  // Configurations open from main. Select status, return, and reset confirmation.
  game.state=MENU;menuSelection=5;release();cross();
  check(game.state==SETTINGS,"settings menu from main");
  settingsSelection=1;cross();check(game.state==STATS,"status is under settings");
  select();check(game.state==SETTINGS,"back to settings from status");
  settingsSelection=0;cross();check(game.state==CTRL_SELECT,"controls under settings");
  select();check(game.state==SETTINGS,"back to settings from controls");
  settingsSelection=2;cross();check(game.state==RESET_CONFIRM,"reset confirmation nested in settings");
  select();check(game.state==SETTINGS && save.campaignWon,"cancel reset preserves save");
  // Guarded integer signature prevents Arduino auto-prototype enum-order failure.
  void (*konamiFunc)(int)=&feedKonami;
  check(konamiFunc!=nullptr,"Arduino-compatible int signature retained");
  std::cout << "V6.6.0 FEATURES PASS " << checks << " assertions\n";
 } catch(const std::exception &e){std::cerr << "V6.6.0 FEATURE FAIL: "<<e.what()<<" at "<<__ms<<"ms\n";return 1;}
}
