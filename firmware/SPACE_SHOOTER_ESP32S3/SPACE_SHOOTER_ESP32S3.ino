/*
   =============================================================
   SPACE SHOOTER v3.0.0
   ESP32-S3 N16R8 + ILI9341 2.8" + Dabble BLE
   =============================================================

   NOVIDADES v3.0:
     - Mini-boss a cada 5 niveis (com barra de vida no topo)
     - 3 padroes de ataque do boss (spread, radial, laser)
     - Modo CAMPANHA (10 niveis com boss final)
     - Modo ENDLESS (infinito)
     - 4 skins de nave destravaveis (persistente em NVS)
     - Suporte a joystick analogico do Dabble
     - Deteccao automatica de tipo de gamepad

   CONTROLES (Dabble):
     Analogico/D-pad  -> mover nave
     CROSS / CIRCLE   -> atirar
     START            -> pausar / iniciar
     SELECT           -> voltar
*/

#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <DabbleESP32.h>
#include <Preferences.h>
#include <math.h>

#define GAME_VERSION "v3.0.0"

// =============================================================
// DISPLAY
// =============================================================
TFT_eSPI tft = TFT_eSPI(320, 240);
TFT_eSprite canvas = TFT_eSprite(&tft);

// =============================================================
// TELA
// =============================================================
const int SCREEN_W = 320;
const int SCREEN_H = 240;

const int AREA_X = 5;
const int AREA_Y = 30;
const int AREA_W = 310;
const int AREA_H = 205;

// =============================================================
// LIMITES
// =============================================================
#define MAX_BULLETS     16
#define MAX_ENEMY_BULLS 20
#define MAX_ENEMIES     24
#define MAX_PARTICLES   80
#define MAX_STARS       28
#define MAX_POWERUPS     4

// =============================================================
// CORES
// =============================================================
#define C_BG          0x0000
#define C_STAR        0xCE79
#define C_PLAYER      0x07E0
#define C_PLAYER2     0x03E0
#define C_BULLET      0xFFE0
#define C_ENEMY1      0xF800
#define C_ENEMY2      0xF81F
#define C_ENEMY3      0xFD20
#define C_ENEMY4      0x8010
#define C_BOSS        0xF800
#define C_BOSS2       0xFBE0
#define C_EXPLOSION   0xFC00
#define C_POWERUP     0x07FF
#define C_UI          0xFFFF
#define C_BORDER      0x4208
#define C_HIGHLIGHT   0x0841
#define C_DIM         0x7BEF
#define C_HP_BAR      0xF800
#define C_HP_BG       0x3000

// =============================================================
// ESTRUTURAS
// =============================================================
struct Bullet {
  float x, y;
  float vx, vy;
  bool active;
};

struct Enemy {
  float x, y;
  float vy;
  int hp;
  int maxHp;
  int type;
  bool active;
  int w, h;
  unsigned long lastShot;
};

struct Particle {
  float x, y;
  float vx, vy;
  int life;
  uint16_t color;
  bool active;
};

struct Star {
  float x, y;
  float speed;
  uint8_t bright;
};

struct PowerUp {
  float x, y;
  float vy;
  int type;
  bool active;
};

// =============================================================
// BOSS
// =============================================================
enum BossAttack { ATK_SPREAD, ATK_RADIAL, ATK_LASER };

struct Boss {
  bool active;
  float x, y;
  float vx;
  int hp;
  int maxHp;
  int w, h;
  BossAttack attack;
  unsigned long lastAttack;
  unsigned long attackInterval;
  int phase;              // 1 = normal, 2 = enraged (< 40% hp)
  unsigned long spawnTime;
};

Boss boss;

// =============================================================
// SKINS
// =============================================================
enum ShipSkin {
  SKIN_DEFAULT = 0,   // verde (desde o início)
  SKIN_CYAN,          // 5.000 pts acumulados
  SKIN_GOLD,          // vencer a campanha
  SKIN_PURPLE         // 20.000 pts acumulados
};

const int SKIN_COUNT = 4;
const char* skinNames[SKIN_COUNT] = { "VERDE", "CIANO", "DOURADA", "ROXA" };
// Nomes exibidos no menu, conforme os previews vistos no display deste projeto.
// Nao altera indices, cores, criterios de desbloqueio ou dados salvos em NVS.
const char* skinMenuNames[SKIN_COUNT] = { "VERDE", "DOURADA", "CIANO", "ROXA" };
const int skinReq[SKIN_COUNT] = { 0, 5000, -1, 20000 };  // -1 = derrotar boss final

// =============================================================
// MODOS DE JOGO
// =============================================================
enum GameMode { MODE_CAMPAIGN, MODE_ENDLESS };

// =============================================================
// ESTADO
// =============================================================
enum GameState { MENU, MODE_SELECT, SKIN_SELECT, PLAYING, PAUSED, GAME_OVER, SCORES, BOSS_WARN };

struct Game {
  GameState state;
  GameMode  mode;
  float playerX, playerY;
  int playerW, playerH;
  int lives;
  int score;
  int level;
  int combo;
  unsigned long lastKillTime;
  unsigned long lastFireTime;
  unsigned long fireCooldown;
  unsigned long lastEnemySpawn;
  unsigned long enemySpawnInterval;
  int tripleShotTimer;
  int shieldTimer;
  int bossPending;            // nivel em que o boss vai aparecer
  unsigned long bossWarnTimer;
  unsigned long stateTimer;
  unsigned long invulnerableUntil;
  bool gameOverHandled;
  bool newHighScore;
} game;

Bullet bullets[MAX_BULLETS];
Bullet enemyBullets[MAX_ENEMY_BULLS];
Enemy enemies[MAX_ENEMIES];
Particle particles[MAX_PARTICLES];
Star stars[MAX_STARS];
PowerUp powerups[MAX_POWERUPS];

// =============================================================
// NVS
// =============================================================
Preferences prefs;
#define NVS_NS       "shooter"
#define NVS_HISCORE  "hiscore"
#define NVS_GAMES    "games"
#define NVS_TOTAL    "total"
#define NVS_SKIN     "skin"
#define NVS_UNLOCK   "unlock"
#define NVS_CAMPWON  "campwon"

struct SaveData {
  int  highScore;
  int  gamesPlayed;
  int  totalScore;
  int  currentSkin;
  int  unlockedSkins;    // bitmask
  bool campaignWon;
};

SaveData save = {0, 0, 0, 0, 0x01, false};   // começa com skin 0 destravada

// =============================================================
// INPUT CACHE
// =============================================================
struct InputState {
  bool up, down, left, right;
  bool cross, circle, triangle, square;
  bool start, select;
  int axisX, axisY;       // -7 a +7 (gamepad) ou -7..+7 (joystick)
  bool hasAnalog;         // true se joystick analógico detectado
};

InputState input = {false,false,false,false,false,false,false,false,false,false,0,0,false};

// =============================================================
// MENU
// =============================================================
int menuSelection = 0;
const int MENU_ITEMS = 4;
const char* menuLabels[4] = { "START GAME", "SKIN", "SCORE", "RESET DATA" };

int modeSelection = 0;
const int MODE_ITEMS = 2;
const char* modeLabels[2] = { "CAMPANHA", "ENDLESS" };

int skinSelection = 0;

// =============================================================
// UTIL
// =============================================================
float randf(float lo, float hi) {
  return lo + (float)random(0, 10000) / 10000.0f * (hi - lo);
}

bool skinUnlocked(int idx) {
  return (save.unlockedSkins & (1 << idx)) != 0;
}

void unlockSkin(int idx) {
  if (!skinUnlocked(idx)) {
    save.unlockedSkins |= (1 << idx);
    Serial.printf(">>> SKIN DESBLOQUEADA: %s <<<\n", skinNames[idx]);
  }
}

void checkSkinUnlocks() {
  // Skin 1 - 5000 pts acumulados
  if (save.totalScore >= 5000) unlockSkin(SKIN_CYAN);
  // Skin 3 - 20000 pts acumulados
  if (save.totalScore >= 20000) unlockSkin(SKIN_PURPLE);
  // Skin 2 - vencer campanha
  if (save.campaignWon) unlockSkin(SKIN_GOLD);
}

// =============================================================
// NVS
// =============================================================
void loadSave() {
  prefs.begin(NVS_NS, true);
  save.highScore      = prefs.getInt(NVS_HISCORE, 0);
  save.gamesPlayed    = prefs.getInt(NVS_GAMES, 0);
  save.totalScore     = prefs.getInt(NVS_TOTAL, 0);
  save.currentSkin    = prefs.getInt(NVS_SKIN, 0);
  save.unlockedSkins  = prefs.getInt(NVS_UNLOCK, 0x01);
  save.campaignWon    = prefs.getBool(NVS_CAMPWON, false);
  prefs.end();

  Serial.printf("NVS: HI=%d Jogos=%d Total=%d Skin=%d Unlock=0x%X CampWon=%d\n",
                save.highScore, save.gamesPlayed, save.totalScore,
                save.currentSkin, save.unlockedSkins, save.campaignWon);
}

void saveAll() {
  prefs.begin(NVS_NS, false);
  prefs.putInt(NVS_HISCORE, save.highScore);
  prefs.putInt(NVS_GAMES,   save.gamesPlayed);
  prefs.putInt(NVS_TOTAL,   save.totalScore);
  prefs.putInt(NVS_SKIN,    save.currentSkin);
  prefs.putInt(NVS_UNLOCK,  save.unlockedSkins);
  prefs.putBool(NVS_CAMPWON, save.campaignWon);
  prefs.end();
}

void resetSave() {
  save.highScore = 0;
  save.gamesPlayed = 0;
  save.totalScore = 0;
  save.currentSkin = 0;
  save.unlockedSkins = 0x01;
  save.campaignWon = false;
  saveAll();
  Serial.println("NVS resetado.");
}

void spawnExplosion(float x, float y, int power);

void onGameOver() {
  save.gamesPlayed++;
  save.totalScore += game.score;
  if (game.score > save.highScore) save.highScore = game.score;
  checkSkinUnlocks();
  saveAll();
}

void finishGame() {
  if (game.gameOverHandled) return;

  game.gameOverHandled = true;
  game.newHighScore = (game.score > save.highScore);
  onGameOver();
  game.state = GAME_OVER;
}

bool playerInvulnerable() {
  return (long)(millis() - game.invulnerableUntil) < 0;
}

void damagePlayer() {
  if (game.state != PLAYING) return;
  if (game.shieldTimer > 0) return;
  if (playerInvulnerable()) return;

  game.lives--;
  game.invulnerableUntil = millis() + 700;
  spawnExplosion(game.playerX, game.playerY, 1);

  if (game.lives <= 0) {
    game.lives = 0;
    finishGame();
  }
}

// =============================================================
// INPUT DABBLE
// =============================================================
void pollDabble() {
  // Processa o BLE continuamente para reduzir latencia e nao perder toques curtos.
  Dabble.processInput();

  // D-pad e botoes digitais permanecem independentes do analogico.
  input.up       = GamePad.isUpPressed();
  input.down     = GamePad.isDownPressed();
  input.left     = GamePad.isLeftPressed();
  input.right    = GamePad.isRightPressed();
  input.cross    = GamePad.isCrossPressed();
  input.circle   = GamePad.isCirclePressed();
  input.triangle = GamePad.isTrianglePressed();
  input.square   = GamePad.isSquarePressed();
  input.start    = GamePad.isStartPressed();
  input.select   = GamePad.isSelectPressed();

  int ax = constrain(GamePad.getXaxisData(), -7, 7);
  int rawY = constrain(GamePad.getYaxisData(), -7, 7);

  // Zona morta real: +/-1 nao movimenta a nave.
  if (abs(ax) <= 1) ax = 0;
  if (abs(rawY) <= 1) rawY = 0;

  input.axisX = ax;

  // No Dabble, empurrar para cima retorna Y positivo.
  // Na tela, subir significa diminuir Y; por isso o eixo e invertido aqui.
  input.axisY = -rawY;

  // Deteccao imediata, sem atraso artificial de ~1 segundo.
  input.hasAnalog = (input.axisX != 0 || input.axisY != 0);
}

// =============================================================
// ESTRELAS
// =============================================================
void initStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(AREA_X, AREA_X + AREA_W);
    stars[i].y = random(AREA_Y, AREA_Y + AREA_H);
    stars[i].speed = randf(0.4f, 2.6f);
    stars[i].bright = random(90, 255);
  }
}

void updateStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].y += stars[i].speed;
    if (stars[i].y > AREA_Y + AREA_H) {
      stars[i].y = AREA_Y;
      stars[i].x = random(AREA_X, AREA_X + AREA_W);
    }
  }
}

void drawStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    uint16_t c = canvas.color565(stars[i].bright, stars[i].bright, stars[i].bright);
    int sx = (int)stars[i].x, sy = (int)stars[i].y;
    canvas.drawPixel(sx, sy, c);
    if (stars[i].speed > 1.8f) canvas.drawPixel(sx, sy - 1, c);
  }
}

// =============================================================
// RESET
// =============================================================
void resetGame() {
  game.playerX = AREA_X + AREA_W / 2.0f;
  game.playerY = AREA_Y + AREA_H - 25;
  game.playerW = 20;
  game.playerH = 22;
  game.lives = 3;
  game.score = 0;
  game.level = 1;
  game.combo = 0;
  game.lastKillTime = 0;
  game.fireCooldown = 180;
  game.lastFireTime = 0;
  game.enemySpawnInterval = 900;
  game.lastEnemySpawn = millis();
  game.tripleShotTimer = 0;
  game.shieldTimer = 0;
  game.bossPending = 5;      // primeiro boss no nivel 5
  game.bossWarnTimer = 0;
  game.stateTimer = 0;
  game.invulnerableUntil = 0;
  game.gameOverHandled = false;
  game.newHighScore = false;

  boss.active = false;
  boss.hp = 0;
  boss.maxHp = 0;

  for (int i = 0; i < MAX_BULLETS; i++)      bullets[i].active = false;
  for (int i = 0; i < MAX_ENEMY_BULLS; i++)  enemyBullets[i].active = false;
  for (int i = 0; i < MAX_ENEMIES; i++)      enemies[i].active = false;
  for (int i = 0; i < MAX_PARTICLES; i++)    particles[i].active = false;
  for (int i = 0; i < MAX_POWERUPS; i++)     powerups[i].active = false;
}

// =============================================================
// SPAWN
// =============================================================
void spawnBullet(float x, float y) {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].x = x; bullets[i].y = y;
      bullets[i].vx = 0; bullets[i].vy = -8.0f;
      bullets[i].active = true;
      return;
    }
  }
}

void spawnEnemyBullet(float x, float y, float vx, float vy) {
  for (int i = 0; i < MAX_ENEMY_BULLS; i++) {
    if (!enemyBullets[i].active) {
      enemyBullets[i].x = x; enemyBullets[i].y = y;
      enemyBullets[i].vx = vx; enemyBullets[i].vy = vy;
      enemyBullets[i].active = true;
      return;
    }
  }
}

void spawnEnemy() {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) {
      int t = random(0, 100);

      if (t < 60 || game.level < 2) {
        enemies[i].type = 0; enemies[i].hp = 1;
        enemies[i].w = 20; enemies[i].h = 18;
      } else if (t < 82) {
        enemies[i].type = 1; enemies[i].hp = 2;
        enemies[i].w = 22; enemies[i].h = 20;
      } else if (t < 95) {
        enemies[i].type = 2; enemies[i].hp = 4;
        enemies[i].w = 28; enemies[i].h = 24;
      } else {
        enemies[i].type = 3; enemies[i].hp = 3;
        enemies[i].w = 24; enemies[i].h = 20;
      }

      enemies[i].maxHp = enemies[i].hp;
      enemies[i].x = randf(AREA_X + 20, AREA_X + AREA_W - 20);
      enemies[i].y = AREA_Y - 20;
      enemies[i].vy = 1.2f + game.level * 0.15f + randf(0, 0.5f);
      enemies[i].lastShot = millis();
      enemies[i].active = true;
      return;
    }
  }
}

void spawnParticles(float x, float y, int count, uint16_t color) {
  int spawned = 0;
  for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
    if (!particles[i].active) {
      float ang = randf(0, TWO_PI);
      float sp  = randf(1.0f, 4.0f);
      particles[i].x = x; particles[i].y = y;
      particles[i].vx = cosf(ang) * sp;
      particles[i].vy = sinf(ang) * sp;
      particles[i].life = random(15, 30);
      particles[i].color = color;
      particles[i].active = true;
      spawned++;
    }
  }
}

void spawnExplosion(float x, float y, int power) {
  spawnParticles(x, y, 6 + power,      0xFFFF);
  spawnParticles(x, y, 10 + power * 2, C_EXPLOSION);
  spawnParticles(x, y, 8 + power * 2,  C_BULLET);
  spawnParticles(x, y, 5,              0x7BEF);
}

void spawnPowerUp(float x, float y) {
  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (!powerups[i].active) {
      powerups[i].x = x; powerups[i].y = y;
      powerups[i].vy = 1.5f;
      int r = random(0, 100);
      powerups[i].type = (r < 15) ? 2 : random(0, 2);
      powerups[i].active = true;
      return;
    }
  }
}

// =============================================================
// BOSS
// =============================================================
void spawnBoss() {
  boss.active = true;
  boss.x = AREA_X + AREA_W / 2.0f;
  boss.y = AREA_Y + 40;
  boss.vx = 1.5f;
  // HP escala com o nivel
  boss.maxHp = 30 + (game.level / 5) * 20;
  boss.hp = boss.maxHp;
  boss.w = 70;
  boss.h = 50;
  boss.phase = 1;
  boss.attack = ATK_SPREAD;
  boss.lastAttack = millis();
  boss.attackInterval = 1200;
  boss.spawnTime = millis();

  Serial.printf("BOSS SPAWNED! HP=%d level=%d\n", boss.maxHp, game.level);
}

void updateBoss() {
  if (!boss.active) return;

  // Movimento horizontal (ping-pong)
  boss.x += boss.vx;
  if (boss.x < AREA_X + boss.w / 2) {
    boss.x = AREA_X + boss.w / 2;
    boss.vx = -boss.vx;
  }
  if (boss.x > AREA_X + AREA_W - boss.w / 2) {
    boss.x = AREA_X + AREA_W - boss.w / 2;
    boss.vx = -boss.vx;
  }

  // Fase 2 quando HP < 40%
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.4f) {
    boss.phase = 2;
    boss.attackInterval = 800;
    Serial.println("BOSS ENRAGED!");
  }

  // Trocar ataque a cada ~5 s
  if (millis() - boss.spawnTime > 5000) {
    boss.spawnTime = millis();
    int a = random(0, 3);
    boss.attack = (BossAttack)a;
  }

  // Atira
  if (millis() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = millis();

    if (boss.attack == ATK_SPREAD) {
      // 5 balas em leque mirando no jogador
      float dx = game.playerX - boss.x;
      float dy = game.playerY - (boss.y + boss.h / 2);
      float baseAng = atan2f(dy, dx);
      int n = (boss.phase == 2) ? 7 : 5;
      for (int i = 0; i < n; i++) {
        float a = baseAng + (i - (n - 1) / 2.0f) * 0.25f;
        float sp = 2.8f;
        spawnEnemyBullet(boss.x, boss.y + boss.h / 2,
                         cosf(a) * sp, sinf(a) * sp);
      }
    } else if (boss.attack == ATK_RADIAL) {
      // 12 balas em círculo
      int n = (boss.phase == 2) ? 16 : 12;
      for (int i = 0; i < n; i++) {
        float a = (float)i * TWO_PI / n;
        float sp = 2.2f;
        spawnEnemyBullet(boss.x, boss.y, cosf(a) * sp, sinf(a) * sp);
      }
    } else if (boss.attack == ATK_LASER) {
      // Rajada rápida de 3 balas verticais
      for (int i = -1; i <= 1; i++) {
        spawnEnemyBullet(boss.x + i * 10, boss.y + boss.h / 2, 0, 4.0f);
      }
    }
  }

  // Colisão com jogador
  float dx = boss.x - game.playerX;
  float dy = boss.y - game.playerY;
  if (fabsf(dx) < boss.w / 2 && fabsf(dy) < boss.h / 2 + 10) {
    if (game.shieldTimer > 0) {
      spawnParticles(game.playerX, game.playerY, 5, C_POWERUP);
    } else {
      damagePlayer();
    }
  }
}

void onBossKilled() {
  Serial.println("BOSS DERROTADO!");
  spawnExplosion(boss.x, boss.y, 4);
  spawnExplosion(boss.x - 20, boss.y, 3);
  spawnExplosion(boss.x + 20, boss.y, 3);
  game.score += 500;
  // Power-up garantido
  spawnPowerUp(boss.x, boss.y);
  spawnPowerUp(boss.x - 20, boss.y);
  spawnPowerUp(boss.x + 20, boss.y);
  boss.active = false;

  // Campanha: venceu no nivel 10
  if (game.mode == MODE_CAMPAIGN && game.level >= 10) {
    save.campaignWon = true;
    checkSkinUnlocks();
    finishGame();
    Serial.println(">>> CAMPANHA VENCIDA! <<<");
  }
}

// =============================================================
// DESENHO - NAVE (com skins)
// =============================================================
void drawPlayer() {
  int x = (int)game.playerX;
  int y = (int)game.playerY;

  // Cores por skin
  uint16_t cMain, cLight, cDark;

  switch (save.currentSkin) {
    case SKIN_CYAN:
      cMain = 0x07FF; cLight = 0xBFFF; cDark = 0x0410; break;
    case SKIN_GOLD:
      cMain = 0xFE60; cLight = 0xFFF0; cDark = 0x8200; break;
    case SKIN_PURPLE:
      cMain = 0x8010; cLight = 0xE81F; cDark = 0x4008; break;
    default:  // verde
      cMain = 0x07E0; cLight = 0x07FF; cDark = 0x0320; break;
  }

  if (game.shieldTimer > 0) cMain = C_POWERUP;

  // Sombra
  canvas.fillTriangle(x, y - 10, x - 9, y + 9, x + 9, y + 9, 0x0000);

  // Asas
  canvas.fillTriangle(x - 5, y - 2, x - 12, y + 6, x - 5, y + 8, cDark);
  canvas.fillTriangle(x + 5, y - 2, x + 12, y + 6, x + 5, y + 8, cDark);

  // Corpo
  canvas.fillTriangle(x, y - 13, x - 10, y + 8, x + 10, y + 8, cMain);

  // Highlight
  canvas.fillTriangle(x, y - 13, x - 5, y - 2, x + 5, y - 2, cLight);

  // Cockpit
  canvas.fillCircle(x, y - 3, 3, 0x001F);
  canvas.fillCircle(x, y - 3, 2, 0x07FF);
  canvas.drawPixel(x, y - 4, 0xFFFF);

  // Detalhes
  canvas.drawPixel(x - 7, y + 3, cLight);
  canvas.drawPixel(x + 7, y + 3, cLight);

  // Chama
  int flameLen = 10 + ((millis() / 50) % 3) * 2;
  canvas.fillTriangle(x - 4, y + 8, x + 4, y + 8, x, y + 8 + flameLen, 0xFC00);
  canvas.fillTriangle(x - 2, y + 8, x + 2, y + 8, x, y + 6 + flameLen, 0xFFE0);
  canvas.fillTriangle(x - 1, y + 8, x + 1, y + 8, x, y + 4 + flameLen, 0xFFFF);

  // Escudo
  if (game.shieldTimer > 0) {
    uint16_t sCol = (game.shieldTimer % 10 < 5) ? C_POWERUP : 0x07FF;
    canvas.drawCircle(x, y, 16, sCol);
    canvas.drawCircle(x, y, 17, sCol);
    canvas.drawPixel(x - 12, y - 12, 0xFFFF);
    canvas.drawPixel(x + 12, y + 12, 0xFFFF);
  }
}

// =============================================================
// DESENHO - INIMIGOS
// =============================================================
void drawEnemy(Enemy &e) {
  int x = (int)e.x, y = (int)e.y;

  if (e.type == 0) {
    canvas.fillTriangle(x, y + 9, x - 10, y - 7, x + 10, y - 7, 0xC000);
    canvas.fillTriangle(x, y + 7, x - 8, y - 5, x + 8, y - 5, C_ENEMY1);
    canvas.drawPixel(x - 11, y - 3, 0xFFFF);
    canvas.drawPixel(x + 11, y - 3, 0xFFFF);
    canvas.fillRect(x - 4, y - 3, 3, 3, 0xFFFF);
    canvas.fillRect(x + 1, y - 3, 3, 3, 0xFFFF);
    canvas.drawPixel(x - 3, y - 2, 0x0000);
    canvas.drawPixel(x + 2, y - 2, 0x0000);
  }
  else if (e.type == 1) {
    canvas.fillRoundRect(x - 10, y - 8, 20, 16, 3, 0x8000);
    canvas.fillRoundRect(x - 9, y - 7, 18, 14, 3, C_ENEMY2);
    canvas.drawLine(x - 8, y - 8, x - 12, y - 12, 0xFFFF);
    canvas.drawLine(x + 8, y - 8, x + 12, y - 12, 0xFFFF);
    canvas.fillCircle(x, y, 3, 0xFFFF);
    canvas.fillCircle(x, y, 2, 0xF800);
  }
  else if (e.type == 2) {
    canvas.fillRoundRect(x - 14, y - 12, 28, 24, 4, 0x8000);
    canvas.fillRoundRect(x - 13, y - 11, 26, 22, 4, C_ENEMY3);
    canvas.drawRect(x - 13, y - 11, 26, 22, 0xFC00);
    canvas.fillRect(x - 10, y - 6, 20, 5, 0xFFFF);
    canvas.fillRect(x - 10, y + 2, 20, 3, 0xFC00);
    canvas.fillCircle(x, y, 4, 0xFFE0);
    canvas.fillCircle(x, y, 3, 0xF800);
    canvas.drawPixel(x - 1, y - 1, 0xFFFF);
  }
  else if (e.type == 3) {
    canvas.fillTriangle(x, y + 10, x - 12, y - 6, x + 12, y - 6, 0x4000);
    canvas.fillTriangle(x, y + 8, x - 10, y - 4, x + 10, y - 4, C_ENEMY4);
    canvas.fillRect(x - 2, y - 4, 4, 10, 0xFFFF);
    canvas.fillRect(x - 1, y + 4, 2, 4, 0xFFFF);
    if ((millis() / 150) % 2 == 0) {
      canvas.fillCircle(x - 8, y - 2, 2, 0xF800);
      canvas.fillCircle(x + 8, y - 2, 2, 0xF800);
    } else {
      canvas.fillCircle(x - 8, y - 2, 2, 0xFC00);
      canvas.fillCircle(x + 8, y - 2, 2, 0xFC00);
    }
  }
}

// =============================================================
// DESENHO - BOSS
// =============================================================
void drawBoss() {
  if (!boss.active) return;

  int x = (int)boss.x, y = (int)boss.y;
  uint16_t cBody = (boss.phase == 2) ? C_BOSS : 0xC000;
  uint16_t cTrim = (boss.phase == 2) ? C_BOSS2 : 0xFC00;

  // Corpo principal
  canvas.fillRoundRect(x - 35, y - 22, 70, 40, 8, 0x4000);
  canvas.fillRoundRect(x - 34, y - 21, 68, 38, 8, cBody);

  // Asas laterais
  canvas.fillTriangle(x - 34, y - 15, x - 45, y + 5, x - 34, y + 15, cTrim);
  canvas.fillTriangle(x + 34, y - 15, x + 45, y + 5, x + 34, y + 15, cTrim);

  // Núcleo central
  canvas.fillCircle(x, y, 12, 0x4000);
  canvas.fillCircle(x, y, 10, cTrim);
  canvas.fillCircle(x, y, 6, 0xFFFF);
  canvas.fillCircle(x, y, 4, (boss.phase == 2) ? 0xF800 : 0x001F);

  // Detalhes superiores
  canvas.fillRect(x - 25, y - 20, 50, 4, cTrim);
  canvas.fillRect(x - 20, y + 12, 40, 4, cTrim);

  // Canhões
  canvas.fillRect(x - 30, y + 10, 6, 12, 0xFFFF);
  canvas.fillRect(x + 24, y + 10, 6, 12, 0xFFFF);
  canvas.fillRect(x - 4, y + 15, 8, 8, 0xFFFF);

  // Olhos
  if ((millis() / 200) % 2 == 0) {
    canvas.fillRect(x - 18, y - 8, 8, 4, 0xFFE0);
    canvas.fillRect(x + 10, y - 8, 8, 4, 0xFFE0);
  } else {
    canvas.fillRect(x - 18, y - 8, 8, 4, 0xF800);
    canvas.fillRect(x + 10, y - 8, 8, 4, 0xF800);
  }

  // Flash quando toma dano (nos últimos 150ms)
  static unsigned long lastHit = 0;
  // (o flash é controlado externamente)
}

// =============================================================
// HUD
// =============================================================
void drawUI() {
  canvas.setTextColor(C_UI, C_BG);
  canvas.setTextSize(1);

  canvas.setCursor(6, 6);
  canvas.printf("SCORE %d", game.score);

  // Modo
  canvas.setCursor(SCREEN_W / 2 - 40, 6);
  if (game.mode == MODE_CAMPAIGN) {
    canvas.printf("CAMP %d/10", game.level);
  } else {
    canvas.printf("ENDLESS %d", game.level);
  }

  // Vidas
  for (int i = 0; i < game.lives; i++) {
    int hx = SCREEN_W - 14 - i * 14;
    canvas.fillTriangle(hx, 8, hx - 5, 18, hx + 5, 18, C_PLAYER);
    canvas.drawPixel(hx, 12, 0xFFFF);
  }

  // Barras
  if (game.tripleShotTimer > 0) {
    canvas.fillRect(6, 18, 50 * game.tripleShotTimer / 600, 3, C_BULLET);
    canvas.drawRect(5, 17, 52, 5, C_BULLET);
  }
  if (game.shieldTimer > 0) {
    canvas.fillRect(6, 23, 50 * game.shieldTimer / 800, 3, C_POWERUP);
    canvas.drawRect(5, 22, 52, 5, C_POWERUP);
  }

  // Combo
  if (game.combo > 1 && millis() - game.lastKillTime < 1500) {
    canvas.setTextColor(C_BULLET, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(SCREEN_W / 2 - 18, 90);
    canvas.printf("x%d", game.combo);
    canvas.setTextSize(1);
  }

  // Borda
  canvas.drawRect(AREA_X - 1, AREA_Y - 1, AREA_W + 2, AREA_H + 2, C_BORDER);

  // Barra de vida do boss
  if (boss.active) {
    int barW = AREA_W - 20;
    int barX = AREA_X + 10;
    int barY = AREA_Y + 3;

    canvas.fillRect(barX, barY, barW, 8, C_HP_BG);
    float pct = (float)boss.hp / boss.maxHp;
    int fill = (int)(barW * pct);
    uint16_t hpCol = (pct > 0.5f) ? 0x07E0 : (pct > 0.25f) ? 0xFFE0 : 0xF800;
    canvas.fillRect(barX, barY, fill, 8, hpCol);
    canvas.drawRect(barX, barY, barW, 8, 0xFFFF);

    canvas.setTextColor(0xFFFF, C_HP_BG);
    canvas.setCursor(barX + 2, barY - 1);
    canvas.print("BOSS");

    char hpTxt[16];
    sprintf(hpTxt, "%d/%d", boss.hp, boss.maxHp);
    canvas.setCursor(barX + barW - 40, barY - 1);
    canvas.print(hpTxt);
  }
}

// =============================================================
// UPDATE - JOGADOR (analógico + digital)
// =============================================================
void updatePlayer() {
  const float SPEED = 6.0f;
  float dx = 0.0f, dy = 0.0f;

  bool dpadActive = input.up || input.down || input.left || input.right;

  if (dpadActive) {
    // D-pad tem prioridade total. Isso evita drift do analogico interferindo.
    if (input.up)    dy -= 1.0f;
    if (input.down)  dy += 1.0f;
    if (input.left)  dx -= 1.0f;
    if (input.right) dx += 1.0f;
  } else if (input.hasAnalog) {
    dx = input.axisX / 7.0f;
    dy = input.axisY / 7.0f;
  }

  // Limita o vetor a magnitude 1 para a diagonal nao ficar mais rapida.
  float mag = sqrtf(dx * dx + dy * dy);
  if (mag > 1.0f) {
    dx /= mag;
    dy /= mag;
  }

  game.playerX += dx * SPEED;
  game.playerY += dy * SPEED;

  if (game.playerX < AREA_X + 12) game.playerX = AREA_X + 12;
  if (game.playerX > AREA_X + AREA_W - 12) game.playerX = AREA_X + AREA_W - 12;
  if (game.playerY < AREA_Y + 12) game.playerY = AREA_Y + 12;
  if (game.playerY > AREA_Y + AREA_H - 12) game.playerY = AREA_Y + AREA_H - 12;
}

// =============================================================
// TIRO
// =============================================================
void tryFire() {
  bool fire = input.cross || input.circle;

  if (fire && millis() - game.lastFireTime > game.fireCooldown) {
    if (game.tripleShotTimer > 0) {
      spawnBullet(game.playerX, game.playerY - 12);
      spawnBullet(game.playerX - 8, game.playerY - 6);
      spawnBullet(game.playerX + 8, game.playerY - 6);
    } else {
      spawnBullet(game.playerX, game.playerY - 12);
    }
    game.lastFireTime = millis();
  }
}

void updateBullets() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) continue;
    bullets[i].x += bullets[i].vx;
    bullets[i].y += bullets[i].vy;
    if (bullets[i].y < AREA_Y - 5) bullets[i].active = false;
  }
}

void updateEnemyBullets() {
  for (int i = 0; i < MAX_ENEMY_BULLS; i++) {
    if (!enemyBullets[i].active) continue;
    enemyBullets[i].x += enemyBullets[i].vx;
    enemyBullets[i].y += enemyBullets[i].vy;

    if (enemyBullets[i].y > AREA_Y + AREA_H + 5 ||
        enemyBullets[i].y < AREA_Y - 10 ||
        enemyBullets[i].x < AREA_X - 10 ||
        enemyBullets[i].x > AREA_X + AREA_W + 10) {
      enemyBullets[i].active = false;
      continue;
    }

    if (fabsf(enemyBullets[i].x - game.playerX) < 10 &&
        fabsf(enemyBullets[i].y - game.playerY) < 10) {
      if (game.shieldTimer > 0) {
        spawnParticles(enemyBullets[i].x, enemyBullets[i].y, 5, C_POWERUP);
      } else {
        damagePlayer();
      }
      enemyBullets[i].active = false;

      if (game.state == GAME_OVER) return;
    }
  }
}

// =============================================================
// INIMIGOS
// =============================================================
void updateEnemies() {
  static float zigzagPhase = 0;
  zigzagPhase += 0.1f;

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) continue;
    enemies[i].y += enemies[i].vy;

    if (enemies[i].type == 1) {
      enemies[i].x += sinf(zigzagPhase + i) * 1.5f;
      if (enemies[i].x < AREA_X + 15) enemies[i].x = AREA_X + 15;
      if (enemies[i].x > AREA_X + AREA_W - 15) enemies[i].x = AREA_X + AREA_W - 15;
    }

    if (enemies[i].type == 3) {
      if (enemies[i].y > AREA_Y + 40) enemies[i].vy = 0.2f;
      if (millis() - enemies[i].lastShot > 1400 && random(0, 100) < 8) {
        float dx = game.playerX - enemies[i].x;
        float dy = game.playerY - enemies[i].y;
        float d = sqrtf(dx*dx + dy*dy);
        if (d > 5) {
          float sp = 3.0f;
          spawnEnemyBullet(enemies[i].x, enemies[i].y + 8,
                           dx / d * sp, dy / d * sp);
        }
        enemies[i].lastShot = millis();
      }
    }

    float dx = enemies[i].x - game.playerX;
    float dy = enemies[i].y - game.playerY;
    if (fabsf(dx) < (enemies[i].w / 2 + game.playerW / 2 - 4) &&
        fabsf(dy) < (enemies[i].h / 2 + game.playerH / 2 - 4)) {
      if (game.shieldTimer > 0) {
        spawnParticles(enemies[i].x, enemies[i].y, 10, C_POWERUP);
      } else {
        damagePlayer();
      }
      enemies[i].active = false;

      if (game.state == GAME_OVER) return;
      continue;
    }

    if (enemies[i].y > AREA_Y + AREA_H + 30) {
      enemies[i].active = false;
      if (game.score >= 5) game.score -= 5; else game.score = 0;
    }
  }
}

// =============================================================
// COLISÃO
// =============================================================
void checkBulletHits() {
  for (int b = 0; b < MAX_BULLETS; b++) {
    if (!bullets[b].active) continue;

    // Colisão com boss
    if (boss.active) {
      if (fabsf(bullets[b].x - boss.x) < boss.w / 2 &&
          fabsf(bullets[b].y - boss.y) < boss.h / 2) {
        bullets[b].active = false;
        boss.hp--;
        spawnParticles(bullets[b].x, bullets[b].y, 3, C_BULLET);
        if (boss.hp <= 0) {
          onBossKilled();
        }
        continue;
      }
    }

    // Colisão com inimigos
    for (int e = 0; e < MAX_ENEMIES; e++) {
      if (!enemies[e].active) continue;
      float dx = bullets[b].x - enemies[e].x;
      float dy = bullets[b].y - enemies[e].y;
      if (fabsf(dx) < enemies[e].w / 2 && fabsf(dy) < enemies[e].h / 2) {
        bullets[b].active = false;
        enemies[e].hp--;
        spawnParticles(bullets[b].x, bullets[b].y, 4, C_BULLET);

        if (enemies[e].hp <= 0) {
          int pts = (enemies[e].type == 0) ? 10 :
                    (enemies[e].type == 1) ? 25 :
                    (enemies[e].type == 2) ? 60 : 80;

          unsigned long now = millis();
          if (now - game.lastKillTime < 1500) {
            if (game.combo < 10) game.combo++;
          } else {
            game.combo = 1;
          }
          game.lastKillTime = now;

          int mult = (game.combo >= 5) ? 3 : (game.combo >= 3) ? 2 : 1;
          game.score += pts * mult;

          spawnExplosion(enemies[e].x, enemies[e].y,
                         (enemies[e].type == 2) ? 2 : 1);
          if (random(0, 100) < 14) spawnPowerUp(enemies[e].x, enemies[e].y);
          enemies[e].active = false;
        }
        break;
      }
    }
  }
}

// =============================================================
// PARTÍCULAS
// =============================================================
void updateParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
    particles[i].x += particles[i].vx;
    particles[i].y += particles[i].vy;
    particles[i].vx *= 0.92f;
    particles[i].vy *= 0.92f;
    particles[i].life--;
    if (particles[i].life <= 0) particles[i].active = false;
  }
}

// =============================================================
// POWER-UPS
// =============================================================
void updatePowerUps() {
  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (!powerups[i].active) continue;
    powerups[i].y += powerups[i].vy;

    float dx = powerups[i].x - game.playerX;
    float dy = powerups[i].y - game.playerY;
    if (fabsf(dx) < 14 && fabsf(dy) < 16) {
      if (powerups[i].type == 0)      game.tripleShotTimer = 600;
      else if (powerups[i].type == 1) game.shieldTimer = 800;
      else if (powerups[i].type == 2) { if (game.lives < 5) game.lives++; }
      spawnParticles(powerups[i].x, powerups[i].y, 10, C_POWERUP);
      powerups[i].active = false;
      continue;
    }
    if (powerups[i].y > AREA_Y + AREA_H + 10) powerups[i].active = false;
  }
}

// =============================================================
// TIMERS + DIFICULDADE + BOSS SCHEDULER
// =============================================================
void updateTimers() {
  if (game.tripleShotTimer > 0) game.tripleShotTimer--;
  if (game.shieldTimer    > 0) game.shieldTimer--;

  // Level up
  int newLevel = 1 + game.score / 300;
  if (newLevel > game.level) {
    game.level = newLevel;
    if (game.enemySpawnInterval > 350) game.enemySpawnInterval -= 100;
    if (game.fireCooldown > 100)       game.fireCooldown       -= 15;

    // Aviso de boss a cada 5 níveis
    if (game.level % 5 == 0 && game.level != 0 && game.level != 5 && game.level != 10) {
      // Nada - boss já apareceu no 5
    }
  }

  // Verifica se é hora de spawnar boss
  if (!boss.active && game.level >= game.bossPending) {
    // Limpa inimigos pequenos para dar espaço
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_ENEMY_BULLS; i++) enemyBullets[i].active = false;

    spawnBoss();
    game.bossPending += 5;   // próximo boss no próximo múltiplo de 5

    // Campanha: só vai até o nível 10
    if (game.mode == MODE_CAMPAIGN && game.level >= 10) {
      // último boss
      game.bossPending = 999;
    }
  }

  // Spawn normal de inimigos (pausa durante boss)
  if (!boss.active && millis() - game.lastEnemySpawn > game.enemySpawnInterval) {
    spawnEnemy();
    game.lastEnemySpawn = millis();
  }
}

// =============================================================
// RENDER
// =============================================================
void render() {
  canvas.fillSprite(C_BG);

  drawStars();

  // Partículas
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
    int px = (int)particles[i].x, py = (int)particles[i].y;
    if (px < 0 || px >= SCREEN_W || py < 0 || py >= SCREEN_H) continue;
    int rad = (particles[i].life > 20) ? 2 : 1;
    canvas.fillCircle(px, py, rad, particles[i].color);
  }

  // Power-ups
  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (!powerups[i].active) continue;
    int px = (int)powerups[i].x, py = (int)powerups[i].y;

    if (powerups[i].type == 0) {
      canvas.fillCircle(px, py, 7, C_BULLET);
      canvas.drawCircle(px, py, 7, 0xFFFF);
      canvas.setTextColor(0x0000, C_BULLET);
      canvas.setCursor(px - 3, py - 3); canvas.print("T");
    } else if (powerups[i].type == 1) {
      canvas.fillCircle(px, py, 7, C_POWERUP);
      canvas.drawCircle(px, py, 7, 0xFFFF);
      canvas.setTextColor(0x0000, C_POWERUP);
      canvas.setCursor(px - 3, py - 3); canvas.print("S");
    } else if (powerups[i].type == 2) {
      canvas.fillCircle(px - 3, py - 2, 3, 0xF800);
      canvas.fillCircle(px + 3, py - 2, 3, 0xF800);
      canvas.fillTriangle(px - 6, py - 1, px + 6, py - 1, px, py + 6, 0xF800);
      canvas.drawPixel(px - 3, py - 3, 0xFFFF);
    }
  }

  // Inimigos
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) drawEnemy(enemies[i]);
  }

  // Boss
  drawBoss();

  // Balas jogador
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) continue;
    int bx = (int)bullets[i].x, by = (int)bullets[i].y;
    canvas.drawPixel(bx, by + 6, 0x4208);
    canvas.drawPixel(bx, by + 5, 0x8410);
    canvas.drawPixel(bx, by + 4, 0xC618);
    canvas.fillRect(bx - 1, by - 4, 2, 8, C_BULLET);
    canvas.drawPixel(bx, by - 5, 0xFFFF);
  }

  // Balas inimigas
  for (int i = 0; i < MAX_ENEMY_BULLS; i++) {
    if (!enemyBullets[i].active) continue;
    int bx = (int)enemyBullets[i].x, by = (int)enemyBullets[i].y;
    canvas.fillCircle(bx, by, 3, 0xF800);
    canvas.fillCircle(bx, by, 2, 0xFFE0);
    canvas.drawPixel(bx, by, 0xFFFF);
  }

  // Jogador
  if (game.state == PLAYING || game.state == PAUSED) drawPlayer();

  drawUI();

  // ===== MENU =====
  if (game.state == MENU) {
    canvas.setTextSize(3);
    canvas.setTextColor(0x0010, C_BG);
    canvas.setCursor(92, 33); canvas.print("SPACE");
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setCursor(90, 30); canvas.print("SPACE");

    canvas.setTextSize(2);
    canvas.setTextColor(0x0010, C_BG);
    canvas.setCursor(82, 63); canvas.print("SHOOTER");
    canvas.setTextColor(C_BULLET, C_BG);
    canvas.setCursor(80, 60); canvas.print("SHOOTER");

    canvas.setTextSize(1);
    canvas.setTextColor(C_UI, C_BG);
    canvas.setCursor(SCREEN_W / 2 - 40, 92);
    canvas.printf("HI  %d", save.highScore);

    for (int i = 0; i < MENU_ITEMS; i++) {
      int y = 115 + i * 20;
      bool sel = (i == menuSelection);

      if (sel) {
        if ((millis() / 300) % 2 == 0) {
          canvas.setTextColor(C_BULLET, C_BG);
          canvas.setCursor(60, y); canvas.print(">");
        }
        canvas.fillRect(70, y - 2, 180, 16, C_HIGHLIGHT);
        canvas.drawRect(70, y - 2, 180, 16, C_BULLET);
        canvas.setTextColor(C_BULLET, C_HIGHLIGHT);
      } else {
        canvas.setTextColor(0xBDF7, C_BG);
      }
      canvas.setCursor(80, y); canvas.print(menuLabels[i]);
    }

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(60, 210); canvas.print("UP/DOWN  CROSS=OK");

    // Versao atual no canto inferior direito.
    canvas.setTextSize(1);
    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(278, 229);
    canvas.print(GAME_VERSION);
  }

  // ===== MODE SELECT =====
  if (game.state == MODE_SELECT) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(70, 50); canvas.print("MODO DE JOGO");

    for (int i = 0; i < MODE_ITEMS; i++) {
      int y = 100 + i * 40;
      bool sel = (i == modeSelection);

      if (sel) {
        if ((millis() / 300) % 2 == 0) {
          canvas.setTextColor(C_BULLET, C_BG);
          canvas.setTextSize(2);
          canvas.setCursor(55, y); canvas.print(">");
        }
        canvas.fillRect(75, y - 4, 170, 26, C_HIGHLIGHT);
        canvas.drawRect(75, y - 4, 170, 26, C_BULLET);
        canvas.setTextColor(C_BULLET, C_HIGHLIGHT);
      } else {
        canvas.setTextColor(0xBDF7, C_BG);
      }
      canvas.setTextSize(2);
      canvas.setCursor(85, y); canvas.print(modeLabels[i]);
    }

    canvas.setTextSize(1);
    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(30, 200); canvas.print("CAMPANHA = 10 niveis + boss final");
  }

  // ===== SKIN SELECT =====
  if (game.state == SKIN_SELECT) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(85, 15); canvas.print("SKINS");

    for (int i = 0; i < SKIN_COUNT; i++) {
      int y = 55 + i * 38;
      bool sel = (i == skinSelection);
      bool unl = skinUnlocked(i);

      uint16_t c = 0x8410;
      if (i == SKIN_DEFAULT) c = 0x07E0;
      else if (i == SKIN_CYAN) c = 0x07FF;
      else if (i == SKIN_GOLD) c = 0xFE60;
      else if (i == SKIN_PURPLE) c = 0x8010;

      // Preview nave
      int px = 40, py = y + 12;
      canvas.fillTriangle(px, py - 10, px - 8, py + 8, px + 8, py + 8, c);
      canvas.fillCircle(px, py - 2, 2, 0xFFFF);

      // Nome
      if (sel) {
        if ((millis() / 300) % 2 == 0) {
          canvas.setTextColor(C_BULLET, C_BG);
          canvas.setCursor(58, y);
          canvas.setTextSize(2);
          canvas.print(">");
        }
        canvas.drawRect(64, y - 4, 220, 26, unl ? C_BULLET : 0x4000);
      }

      canvas.setTextSize(2);
      canvas.setTextColor(unl ? 0xFFFF : 0x4208, C_BG);
      canvas.setCursor(75, y);
      canvas.print(skinMenuNames[i]);

      // Status
      canvas.setTextSize(1);
      if (!unl) {
        canvas.setTextColor(0xF800, C_BG);
        canvas.setCursor(75, y + 18);
        if (i == SKIN_CYAN) canvas.print("Req: 5000 pts acum.");
        else if (i == SKIN_GOLD) canvas.print("Req: vencer campanha");
        else if (i == SKIN_PURPLE) canvas.print("Req: 20000 pts acum.");
      } else if (i == save.currentSkin) {
        canvas.setTextColor(0x07E0, C_BG);
        canvas.setCursor(75, y + 18);
        canvas.print("<< ATIVA >>");
      }
    }

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(60, 215); canvas.print("CROSS=OK  SELECT=voltar");
  }

  // ===== SCORES =====
  if (game.state == SCORES) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(3);
    canvas.setCursor(80, 20); canvas.print("SCORES");
    canvas.drawLine(60, 48, SCREEN_W - 60, 48, C_PLAYER);

    canvas.setTextSize(1);
    canvas.setTextColor(C_UI, C_BG);

    canvas.setCursor(45, 65);
    canvas.printf("HIGH SCORE:  %d", save.highScore);
    canvas.setCursor(45, 85);
    canvas.printf("PARTIDAS:    %d", save.gamesPlayed);
    int avg = (save.gamesPlayed > 0) ? (save.totalScore / save.gamesPlayed) : 0;
    canvas.setCursor(45, 105);
    canvas.printf("MEDIA:       %d", avg);
    canvas.setCursor(45, 125);
    canvas.printf("TOTAL:       %d", save.totalScore);

    canvas.setCursor(45, 150);
    canvas.printf("SKINS:       %d / %d", __builtin_popcount(save.unlockedSkins), SKIN_COUNT);

    canvas.setCursor(45, 170);
    if (save.campaignWon) {
      canvas.setTextColor(0x07E0, C_BG);
      canvas.print("CAMPANHA:    VENCIDA!");
    } else {
      canvas.setTextColor(0xF800, C_BG);
      canvas.print("CAMPANHA:    nao vencida");
    }

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(70, 210); canvas.print("CROSS/SELECT p/ voltar");
  }

  // ===== PAUSA =====
  if (game.state == PAUSED) {
    canvas.fillRect(90, 95, 140, 50, C_BG);
    canvas.drawRect(90, 95, 140, 50, TFT_YELLOW);
    canvas.drawRect(92, 97, 136, 46, TFT_YELLOW);
    canvas.setTextColor(TFT_YELLOW, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(125, 113); canvas.print("PAUSA");
  }

  // ===== GAME OVER =====
  if (game.state == GAME_OVER) {
    canvas.fillRect(30, 70, SCREEN_W - 60, 110, C_BG);
    canvas.drawRect(30, 70, SCREEN_W - 60, 110, C_ENEMY1);
    canvas.drawRect(32, 72, SCREEN_W - 64, 106, C_ENEMY1);

    bool venceu = (save.campaignWon && game.mode == MODE_CAMPAIGN && game.level >= 10 && boss.hp <= 0);

    canvas.setTextColor(venceu ? 0x07E0 : C_ENEMY1, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(venceu ? 75 : 85, 78);
    canvas.print(venceu ? "VITORIA!" : "GAME OVER");

    canvas.setTextColor(C_UI, C_BG);
    canvas.setTextSize(1);
    canvas.setCursor(80, 110);
    canvas.printf("SCORE: %d", game.score);
    canvas.setCursor(80, 123);
    canvas.printf("NIVEL: %d", game.level);

    if (game.newHighScore) {
      canvas.setTextColor(C_BULLET, C_BG);
      canvas.setCursor(75, 140);
      canvas.print("*** NOVO RECORDE ***");
    } else {
      canvas.setTextColor(C_DIM, C_BG);
      canvas.setCursor(80, 140);
      canvas.printf("HI: %d", save.highScore);
    }

    canvas.setTextColor(C_UI, C_BG);
    canvas.setCursor(80, 160);
    canvas.print("START p/ reiniciar");
  }

  canvas.pushSprite(0, 0);
}

// =============================================================
// SETUP
// =============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println(" SPACE SHOOTER v3.0.0");
  Serial.println(" ESP32-S3 + ILI9341 + DABBLE");
  Serial.println("================================");

  Serial.printf("PSRAM livre: %d bytes\n", ESP.getFreePsram());
  Serial.printf("SRAM livre:  %d bytes\n", ESP.getFreeHeap());
  Serial.printf("CPU freq:    %d MHz\n", getCpuFrequencyMhz());

  loadSave();
  checkSkinUnlocks();

  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);

  canvas.setColorDepth(8);
  if (!canvas.createSprite(SCREEN_W, SCREEN_H)) {
    Serial.println("ERRO: falha ao criar sprite!");
    while (1) delay(100);
  }
  canvas.fillSprite(C_BG);

  randomSeed(esp_random());

  initStars();
  resetGame();
  game.state = MENU;

  Serial.println("Iniciando BLE...");
  Dabble.begin("ESP32-S3-GAMEPAD");
  Serial.println("BLE OK. Nome: ESP32-S3-GAMEPAD");
  Serial.println();
  Serial.println(">>> Dica: no app Dabble, escolha 'Joystick' ou");
  Serial.println(">>> 'GamePad Advanced' para usar analogico!");
  Serial.println();
}

// =============================================================
// LOOP
// =============================================================
unsigned long lastFrame = 0;
const int FRAME_MS = 16;

bool prevStart = false;
bool prevSelect = false;
bool prevCross = false;
bool prevNavUp = false;
bool prevNavDown = false;

bool navUpNow() {
  // D-pad vertical tem prioridade sobre o analogico.
  if (input.up || input.down) return input.up;
  return input.axisY < -2;
}

bool navDownNow() {
  // D-pad vertical tem prioridade sobre o analogico.
  if (input.up || input.down) return input.down;
  return input.axisY > 2;
}

void loop() {
  // BLE deve ser processado o mais frequentemente possivel.
  pollDabble();

  unsigned long now = millis();
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;

  bool navUp = navUpNow();
  bool navDown = navDownNow();

  bool startPressed  = input.start  && !prevStart;
  bool selectPressed = input.select && !prevSelect;
  bool crossPressed  = input.cross  && !prevCross;
  bool upPressed     = navUp        && !prevNavUp;
  bool downPressed   = navDown      && !prevNavDown;

  // START: pausa/continua apenas durante uma partida.
  if (startPressed) {
    if (game.state == PLAYING) {
      game.state = PAUSED;
    } else if (game.state == PAUSED) {
      game.state = PLAYING;
    }
  }

  // SELECT: voltar para o menu nas telas secundarias.
  if (selectPressed) {
    if (game.state == GAME_OVER || game.state == SCORES ||
        game.state == PAUSED || game.state == SKIN_SELECT ||
        game.state == MODE_SELECT) {
      game.state = MENU;
    }
  }

  // ===== MENU =====
  if (game.state == MENU) {
    if (upPressed) {
      menuSelection--;
      if (menuSelection < 0) menuSelection = MENU_ITEMS - 1;
    }

    if (downPressed) {
      menuSelection++;
      if (menuSelection >= MENU_ITEMS) menuSelection = 0;
    }

    if (crossPressed || startPressed) {
      if (menuSelection == 0) {
        modeSelection = 0;
        game.state = MODE_SELECT;
      } else if (menuSelection == 1) {
        skinSelection = save.currentSkin;
        if (skinSelection < 0 || skinSelection >= SKIN_COUNT) skinSelection = 0;
        game.state = SKIN_SELECT;
      } else if (menuSelection == 2) {
        game.state = SCORES;
      } else if (menuSelection == 3) {
        resetSave();
        skinSelection = 0;
      }
    }

    updateStars();
  }

  // ===== MODE SELECT =====
  else if (game.state == MODE_SELECT) {
    if (upPressed) {
      modeSelection--;
      if (modeSelection < 0) modeSelection = MODE_ITEMS - 1;
    }

    if (downPressed) {
      modeSelection++;
      if (modeSelection >= MODE_ITEMS) modeSelection = 0;
    }

    if (crossPressed || startPressed) {
      game.mode = (modeSelection == 0) ? MODE_CAMPAIGN : MODE_ENDLESS;
      resetGame();
      game.state = PLAYING;
      Serial.printf("Iniciando modo %s\n",
                    (game.mode == MODE_CAMPAIGN) ? "CAMPANHA" : "ENDLESS");
    }
  }

  // ===== SKIN SELECT =====
  else if (game.state == SKIN_SELECT) {
    if (upPressed) {
      skinSelection--;
      if (skinSelection < 0) skinSelection = SKIN_COUNT - 1;
    }

    if (downPressed) {
      skinSelection++;
      if (skinSelection >= SKIN_COUNT) skinSelection = 0;
    }

    if (crossPressed || startPressed) {
      if (skinUnlocked(skinSelection)) {
        save.currentSkin = skinSelection;
        saveAll();
        Serial.printf("Skin ativa: %s\n", skinNames[skinSelection]);
        game.state = MENU;
      }
    }
  }

  // ===== SCORES =====
  else if (game.state == SCORES) {
    if (crossPressed || selectPressed || startPressed) {
      game.state = MENU;
    }
  }

  // ===== PLAYING =====
  else if (game.state == PLAYING) {
    updateStars();
    updatePlayer();
    tryFire();
    updateBullets();

    updateEnemyBullets();
    if (game.state == PLAYING) updateEnemies();
    if (game.state == PLAYING) updateBoss();
    if (game.state == PLAYING) checkBulletHits();
    if (game.state == PLAYING) updatePowerUps();
    if (game.state == PLAYING) updateTimers();

    // Particulas podem continuar sendo atualizadas no frame da morte.
    updateParticles();
  }

  // ===== PAUSED =====
  else if (game.state == PAUSED) {
    // congela gameplay
  }

  // ===== GAME OVER =====
  else if (game.state == GAME_OVER) {
    updateStars();
    updateParticles();

    if (startPressed) {
      game.state = MENU;
    }
  }

  render();

  // Estado anterior e atualizado somente uma vez por frame.
  // Isso impede que o mesmo botao atravesse duas telas de menu.
  prevStart = input.start;
  prevSelect = input.select;
  prevCross = input.cross;
  prevNavUp = navUp;
  prevNavDown = navDown;
}
