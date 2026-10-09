#include "../firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino"
#include <iostream>
static int assertions=0;
void ok(bool b,const char* test){ assertions++; if(!b){std::cerr << "FAIL: " << test << "\n"; std::exit(1);} }
void tick(bool up=false,bool down=false,bool cross=false,bool start=false,bool select=false){
  GamePad.up=up;GamePad.down=down;GamePad.cross=cross;GamePad.start=start;GamePad.select=select;
  __ms+=16;loop();
}
int main(){
  setup();
  ok(game.state==MENU,"initial menu");
  ok(save.unlockedSkins==1,"initial unlock");
  ok(strcmp(GAME_VERSION,"v4.0.0")==0,"version label");
  // movement and direction
  game.mode=MODE_ENDLESS;
  resetGame(); game.state=PLAYING;
  float x=game.playerX,y=game.playerY;
  GamePad.ax=7; GamePad.ay=7; pollDabble(); updatePlayer();
  ok(game.playerX>x && game.playerY<y, "analog X right, Y up");
  ok(hypotf(game.playerX-x,game.playerY-y)<=6.001f,"analog diagonal normalized");
  x=game.playerX; y=game.playerY;
  GamePad.ax=-7;GamePad.ay=7;GamePad.left=false;GamePad.right=true;pollDabble();updatePlayer();
  ok(game.playerX>x && game.playerY==y,"D-pad priority over contradictory analog");
  x=game.playerX;y=game.playerY;
  GamePad.right=false;GamePad.ax=1;GamePad.ay=-1;pollDabble();updatePlayer();
  ok(game.playerX==x && game.playerY==y,"deadzone +/-1");
  // single hit & iframes
  game.state=PLAYING;gameClockMs=100;game.lives=3;game.invulnerableUntil=0;
  damagePlayer();damagePlayer();ok(game.lives==2,"damage once during invulnerable frames");
  gameClockMs=801; damagePlayer(); ok(game.lives==1,"damage after iframe expires");
  int previousGames=save.gamesPlayed; gameClockMs=1700;game.score=200;
  damagePlayer();damagePlayer();finishGame();
  ok(game.state==GAME_OVER && game.lives==0 && save.gamesPlayed==previousGames+1,"GameOver saved once");
  // tie record
  resetGame(); game.state=PLAYING; game.score=100;save.highScore=100;
  finishGame(false);ok(!game.newHighScore,"no false record on tie");
  // boss laser pierce (same target only hit once)
  resetGame();game.state=PLAYING;
  boss.active=true;boss.type=BOSS_SENTINEL;boss.hp=10;boss.maxHp=10;boss.x=160;boss.y=70;boss.w=70;boss.h=50;
  spawnBullet(160,70,0,0,2,3,C_BULLET,C_UI);
  checkBulletHits();checkBulletHits();ok(boss.hp==8,"pierce does not repeat boss damage");
  // two enemies, one laser, each hit once
  boss.active=false;for(auto &e:enemies)e.active=false;
  enemies[0].active=true;enemies[0].x=150;enemies[0].y=90;enemies[0].hp=8;enemies[0].w=20;enemies[0].h=20;
  enemies[1].active=true;enemies[1].x=150;enemies[1].y=90;enemies[1].hp=8;enemies[1].w=20;enemies[1].h=20;
  for(auto &b:bullets)b.active=false;
  spawnBullet(150,90,0,0,2,3,C_BULLET,C_UI);
  checkBulletHits();checkBulletHits();
  ok(enemies[0].hp==6 && enemies[1].hp==6,"pierce each enemy at most once");
  // victory exact final boss identity, not level/previous victory
  resetGame();game.mode=MODE_CAMPAIGN;game.level=20;game.state=PLAYING;
  boss.active=true;boss.spawnLevel=5;boss.type=BOSS_SENTINEL;boss.x=160;boss.y=60;
  onBossKilled();ok(!game.wonThisGame && game.state==PLAYING,"boss level5 cannot finish campaign");
  resetGame();game.mode=MODE_CAMPAIGN;game.level=20;game.state=PLAYING;
  boss.active=true;boss.spawnLevel=20;boss.type=BOSS_DEVOURER;
  onBossKilled();ok(game.wonThisGame && game.state==GAME_OVER && save.campaignWon,"final boss victory recorded");
  // menus: cannot double-activate held Cross
  GamePad=GamePadT(); __ms=0;lastFrame=0;resetGame();game.state=MENU;menuSelection=2;
  tick(false,false,true); ok(game.state==SCORES,"open scores with Cross");
  tick(false,false,true);ok(game.state==SCORES,"held Cross does not close score screen");
  tick(); ok(game.state==SCORES,"release Cross stays scores");
  tick(false,false,true); ok(game.state==MENU,"new Cross closes scores");
  // reset data confirmation is required
  tick();menuSelection=3;tick(false,false,true);ok(game.state==RESET_CONFIRM,"reset requires second confirmation");
  tick();tick(false,false,false,false,true);ok(game.state==MENU,"reset cancels on SELECT");
  // pause game clock frozen
  GamePad=GamePadT(); game.mode=MODE_ENDLESS;resetGame();game.state=PLAYING;
  tick();tick();unsigned long before=gameNow();
  tick(false,false,false,true);ok(game.state==PAUSED,"START pauses");
  for(int i=0;i<100;i++){tick();}
  ok(gameNow()==before,"game clock frozen in pause");
  tick(false,false,false,true);ok(game.state==PLAYING,"START resumes");
  tick();ok(gameNow()>before,"game clock resumes");
  ok(strcmp(skinMenuNames[1],"DOURADA")==0 && strcmp(skinMenuNames[2],"CIANO")==0,"keep v3 skin label fix");
  // All six firing modes create valid projectiles; metadata remains stable.
  game.state = PLAYING;
  for(int mode=0;mode<6;mode++) {
    for (auto &b : bullets) b.active=false;
    game.weapon = (WeaponType)mode;
    fireWeapon();
    int active=0;
    for(auto &b:bullets) if(b.active) {
      active++;
      ok(b.weapon == game.weapon, "weapon type retained per projectile");
      ok(b.dmg >= 1 && b.dmg <= 2, "weapon projectile damage within configuration");
    }
    ok(active > 0 && active <= 5, "weapon shot count bounded");
  }
  // Projectiles no longer fall indefinitely below gameplay area.
  for(auto &b:bullets)b.active=false;
  spawnBullet(150, AREA_Y + AREA_H + 20, 0, 1, 1, 0, C_BULLET, C_UI);
  updateBullets();
  ok(!bullets[0].active,"projectile expires below visible area");
  // All five bosses initialize and update for a sustained interval.
  game.invulnerableUntil = 999999999;
  for(int t=0;t<5;t++) {
    game.nextBossLevel = 5 + t*5;
    spawnBoss((BossType)t);
    ok(boss.type==(BossType)t && boss.hp==BOSS_CFG[t].hp,"boss configuration correct");
    for(int frame=0;frame<240;frame++) {
      gameClockMs += 16;
      updateBoss();
      updateEnemyBullets();
      ok(std::isfinite(boss.x) && std::isfinite(boss.y),"boss movement finite");
    }
    boss.active=false;
    for(auto &b:enemyBullets)b.active=false;
  }
  // NVS migration: campaign 10-level win must not imply campaign20 victory.
  ok(strcmp(NVS_NS,"shooter")==0,"NVS namespace compatible with v3");
  save.campaignWon=true;save.campaign20Won=false;
  checkSkinUnlocks();
  ok(skinUnlocked(SKIN_GOLD) && !save.campaign20Won,"legacy Gold retained; v4 victory separate");
  // Scenario updates after boss defeat.
  resetGame();game.mode=MODE_ENDLESS;game.level=5;game.state=PLAYING;
  boss.active=true;boss.spawnLevel=5;boss.type=BOSS_SENTINEL;
  onBossKilled();
  ok(game.scenario==SC_NEBULA && game.nextBossLevel==10,"next scenario and boss threshold updated");
  // A derrota do boss nao escreve no NVS imediatamente: 
  // a persistencia so ocorre depois da transicao ao GAME OVER.
  resetGame(); game.mode=MODE_CAMPAIGN; game.state=PLAYING;
  game.level=20;boss.active=true;boss.spawnLevel=20;boss.type=BOSS_DEVOURER;
  pendingGameSave=false; onBossKilled();
  ok(pendingGameSave && game.state==GAME_OVER,"victory NVS write deferred");
  commitFinishedGame();
  ok(!pendingGameSave,"deferred save flush clears pending");
  ok(DISPLAY_FRAME_MS>=40,"render interval does not saturate 40MHz TFT SPI");
  ok(!GAME_EVENT_LOGS,"gameplay logs disabled by default");
  // Spawn de boss bloqueado brevemente apos boss anterior para nao empilhar lutas.
  resetGame(); game.mode=MODE_ENDLESS; game.state=PLAYING;
  game.level=10; game.nextBossLevel=10;
  boss.active=false; gameClockMs=5000; bossCooldownUntil=6000;
  updateTimers(); ok(!boss.active,"boss cooldown prevents immediate chain boss");
  gameClockMs=6100;updateTimers();ok(boss.active,"boss spawns after cooldown");
  // Stress de explosoes: nunca ativar mais que 80 particulas.
  for(auto &pt:particles) pt.active=false;
  for(int j=0;j<15;j++)spawnExplosion(160,60,5);
  int activeParticles=0;
  for(auto &pt:particles)if(pt.active)++activeParticles;
  ok(activeParticles==MAX_PARTICLES,"particle pool bounded under explosion storm");
  // Menu e controle responsivos mesmo entre frames visuais.
  GamePad=GamePadT(); __ms=0; lastFrame=0; lastDisplay=0;
  lastDisplayedState=MENU;game.state=MENU; menuSelection=0;
  tick();tick(false,true);ok(menuSelection==1,"menu input after render throttle");
  std::cout << "PASS: "<< assertions <<" behavior assertions; clock=" << gameNow() << "ms\n";
}
