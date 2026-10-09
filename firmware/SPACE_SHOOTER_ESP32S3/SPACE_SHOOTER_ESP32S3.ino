/*
   =============================================================
   SPACE SHOOTER v4.0.0
   ESP32-S3 N16R8 + ILI9341 2.8" + Dabble BLE
   =============================================================

   NOVIDADES v4.0:
     - 5 bosses unicos com mecanicas diferentes
     - Campanha de 20 niveis (fases longas, 600 pts cada)
     - 6 tipos de arma (Padrao, Duplo, Triplo, Spread, Laser, Missil)
     - Laser perfurante, missil teleguiado
     - 5 cenarios de fundo que mudam ao progredir
     - Power-ups de arma com cores proprias

   CONTROLES (Dabble):
     Analogico/D-pad  -> mover nave
     CROSS / CIRCLE   -> atirar
     START            -> pausar
     SELECT           -> voltar
*/

#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

#define GAME_VERSION "v4.0.0"
// Candidato anti-travamentos: mantem a versao enquanto passa por testes.
// ESP32-S3 + ILI9341 SPI: 320*240 pixels precisam ser enviados a cada desenho.
constexpr unsigned long DISPLAY_FRAME_MS = 42; // ~24 FPS, evita saturar o SPI
constexpr bool GAME_EVENT_LOGS = false; // Nunca imprimir no caminho critico de gameplay


#include <Arduino.h>
#include <TFT_eSPI.h>
#include <DabbleESP32.h>
#include <Preferences.h>
#include <math.h>

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
#define MAX_BULLETS     24
#define MAX_ENEMY_BULLS 30
#define MAX_ENEMIES     24
#define MAX_PARTICLES   80
#define MAX_STARS       28
#define MAX_POWERUPS     5

// =============================================================
// CORES
// =============================================================
#define C_BG          0x0000
#define C_STAR        0xCE79
#define C_PLAYER      0x07E0
#define C_BULLET      0xFFE0
#define C_ENEMY1      0xF800
#define C_ENEMY2      0xF81F
#define C_ENEMY3      0xFD20
#define C_ENEMY4      0x8010
#define C_EXPLOSION   0xFC00
#define C_POWERUP     0x07FF
#define C_UI          0xFFFF
#define C_BORDER      0x4208
#define C_HIGHLIGHT   0x0841
#define C_DIM         0x7BEF
#define C_HP_BG       0x3000

// =============================================================
// TIPOS DE ARMA
// =============================================================
enum WeaponType {
  WPN_DEFAULT = 0,
  WPN_DOUBLE,
  WPN_TRIPLE,
  WPN_SPREAD,
  WPN_LASER,
  WPN_MISSILE
};

struct WeaponInfo {
  const char* name;
  uint16_t    color;
  uint16_t    colorLight;
  int         cooldown;
  int         dmg;
};

const WeaponInfo WEAPONS[] = {
  { "PADRAO",  0xFFE0, 0xFFFF, 180, 1 },
  { "DUPLO",   0x07FF, 0xBFFF, 160, 1 },
  { "TRIPLO",  0xF81F, 0xFF9F, 150, 1 },
  { "SPREAD",  0x07E0, 0x87E0, 200, 1 },
  { "LASER",   0xF800, 0xFFE0, 260, 2 },
  { "MISSIL",  0xFD20, 0xFFF0, 220, 2 }
};

// =============================================================
// CENARIOS
// =============================================================
enum Scenario {
  SC_SPACE = 0,
  SC_NEBULA,
  SC_ASTEROIDS,
  SC_STORM,
  SC_CHAOS
};

struct ScenarioInfo {
  const char* name;
  uint16_t    starColor;
  uint16_t    bgTint;
  float       starSpeedMul;
  bool        diagonal;
};

const ScenarioInfo SCENARIOS[] = {
  { "ESPACO",     0xCE79, 0x0000, 1.0f, false },
  { "NEBULOSA",   0xF81F, 0x1008, 0.8f, false },
  { "ASTEROIDES", 0x8410, 0x2104, 1.3f, false },
  { "TEMPESTADE", 0x07FF, 0x0010, 2.0f, true  },
  { "CAOS",       0xFD20, 0x4000, 1.6f, true  }
};

// =============================================================
// ESTRUTURAS
// =============================================================
struct Bullet {
  float x, y;
  float vx, vy;
  int   dmg;
  int   pierce;
  bool  guided;
  WeaponType weapon;
  uint32_t hitEnemies;  // 1 bit per enemy slot: evita acertar mesmo alvo a cada frame
  bool hitBoss;
  uint16_t color;
  uint16_t colorLight;
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
  int type;    // 0=escudo, 1=vida, 2..6=armas
  bool active;
};

// =============================================================
// BOSSES
// =============================================================
enum BossType {
  BOSS_SENTINEL = 0,
  BOSS_HYDRA,
  BOSS_PRIME,
  BOSS_DEVOURER,
  BOSS_OMEGA
};

struct Boss {
  bool active;
  BossType type;
  float x, y;
  float vx, vy;
  int hp, maxHp;
  int w, h;
  unsigned long lastAttack;
  unsigned long attackInterval;
  int phase;
  float angleAccum;
  int   attackMode;
  unsigned long lastPhaseSwap;
  bool  shielded;
  unsigned long shieldUntil;
  int spawnLevel;
};

Boss boss;

struct BossConfig {
  int hp;
  int w, h;
  float speed;
  int baseAttackMs;
  int enragedAttackMs;
};

const BossConfig BOSS_CFG[] = {
  {  40, 70, 50, 1.5f, 1200, 800 },
  {  80, 90, 60, 1.2f, 1000, 650 },
  { 120, 80, 55, 2.0f, 900,  550 },
  { 180, 100, 70, 1.0f, 850,  500 },
  { 220, 90, 65, 1.8f, 800,  450 }
};

// =============================================================
// SKINS
// =============================================================
enum ShipSkin {
  SKIN_DEFAULT = 0,
  SKIN_CYAN,
  SKIN_GOLD,
  SKIN_PURPLE
};

const int SKIN_COUNT = 4;
const char* skinNames[SKIN_COUNT] = { "VERDE", "CIANO", "DOURADA", "ROXA" };
// A v3 aprovada exibia estes nomes no menu, sem alterar indices/cores/NVS.
const char* skinMenuNames[SKIN_COUNT] = { "VERDE", "DOURADA", "CIANO", "ROXA" };

// =============================================================
// MODOS
// =============================================================
enum GameMode { MODE_CAMPAIGN, MODE_ENDLESS };

// =============================================================
// ESTADO
// =============================================================
enum GameState { MENU, MODE_SELECT, SKIN_SELECT, PLAYING, PAUSED, GAME_OVER, SCORES, RESET_CONFIRM };

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
  int shieldTimer;
  WeaponType weapon;
  int weaponTimer;
  int nextBossLevel;
  Scenario scenario;
  unsigned long invulnerableUntil;
  bool gameOverHandled;
  bool newHighScore;
  bool wonThisGame;
} game;

// Relogio de gameplay: nao corre em pausa, menu ou GAME OVER.
unsigned long gameClockMs = 0;
// Grava NVS somente com a tela GAME OVER ja visivel, nao durante boss/morte.
bool pendingGameSave = false;
unsigned long gameFinishedWallMs = 0;
unsigned long bossCooldownUntil = 0;

// Estatisticas leves: exibidas ao PAUSAR para diagnostico no hardware.
struct FramePerf {
  uint32_t maxDrawUs;
  uint32_t maxPushUs;
  uint32_t maxLogicUs;
  uint32_t slowDraws;
};
FramePerf perf = {0, 0, 0, 0};

unsigned long gameNow() { return gameClockMs; }


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
#define NVS_CAMP20   "camp20won"

struct SaveData {
  int  highScore;
  int  gamesPlayed;
  int  totalScore;
  int  currentSkin;
  int  unlockedSkins;
  bool campaignWon;    // legacy campaign/Gold skin from v3
  bool campaign20Won;  // victory of this 20-level campaign
};

SaveData save = {0, 0, 0, 0, 0x01, false, false};

// =============================================================
// INPUT
// =============================================================
struct InputState {
  bool up, down, left, right;
  bool cross, circle, triangle, square;
  bool start, select;
  int axisX, axisY;
};

InputState input = {false,false,false,false,false,false,false,false,false,false,0,0};

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
    if (GAME_EVENT_LOGS) Serial.printf(">>> SKIN DESBLOQUEADA: %s <<<\n", skinNames[idx]);
  }
}

void checkSkinUnlocks() {
  if (save.totalScore >= 5000)  unlockSkin(SKIN_CYAN);
  if (save.totalScore >= 20000) unlockSkin(SKIN_PURPLE);
  if (save.campaignWon)         unlockSkin(SKIN_GOLD);
}

// =============================================================
// NVS
// =============================================================
void loadSave() {
  prefs.begin(NVS_NS, true);
  save.highScore     = prefs.getInt(NVS_HISCORE, 0);
  save.gamesPlayed   = prefs.getInt(NVS_GAMES, 0);
  save.totalScore    = prefs.getInt(NVS_TOTAL, 0);
  save.currentSkin   = prefs.getInt(NVS_SKIN, 0);
  save.unlockedSkins = prefs.getInt(NVS_UNLOCK, 0x01);
  save.campaignWon   = prefs.getBool(NVS_CAMPWON, false);
  save.campaign20Won = prefs.getBool(NVS_CAMP20, false);
  prefs.end();

  // Recupera dados antigos sem permitir indices/valores NVS corrompidos.
  if (save.highScore < 0) save.highScore = 0;
  if (save.gamesPlayed < 0) save.gamesPlayed = 0;
  if (save.totalScore < 0) save.totalScore = 0;
  save.unlockedSkins = (save.unlockedSkins & ((1 << SKIN_COUNT) - 1)) | 0x01;
  checkSkinUnlocks();
  if (save.currentSkin < 0 || save.currentSkin >= SKIN_COUNT ||
      !skinUnlocked(save.currentSkin)) save.currentSkin = SKIN_DEFAULT;

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
  prefs.putBool(NVS_CAMP20, save.campaign20Won);
  prefs.end();
}

void resetSave() {
  save.highScore = 0;
  save.gamesPlayed = 0;
  save.totalScore = 0;
  save.currentSkin = 0;
  save.unlockedSkins = 0x01;
  save.campaignWon = false;
  save.campaign20Won = false;
  saveAll();
  Serial.println("NVS resetado.");
}

void spawnExplosion(float x, float y, int power);

void finishGame(bool won = false) {
  if (game.gameOverHandled) return;
  game.gameOverHandled = true;
  game.wonThisGame = won;
  game.newHighScore = (game.score > save.highScore);
  if (won) { save.campaignWon = true; save.campaign20Won = true; }
  save.gamesPlayed++;
  if (game.score > 0 && save.totalScore <= INT32_MAX - game.score)
    save.totalScore += game.score;
  else if (game.score > 0) save.totalScore = INT32_MAX;
  if (game.newHighScore) save.highScore = game.score;
  checkSkinUnlocks();
  // Flash NVS pode bloquear o sistema por dezenas/centenas de ms.
  // Adiamos ate a tela de GAME OVER ja estar sendo exibida.
  pendingGameSave = true;
  gameFinishedWallMs = millis();
  game.state = GAME_OVER;
}

void commitFinishedGame() {
  if (!pendingGameSave) return;
  saveAll();
  pendingGameSave = false;
}

void damagePlayer() {
  if (game.state != PLAYING || game.shieldTimer > 0 ||
      gameNow() < game.invulnerableUntil) return;
  if (game.lives <= 0) return;
  game.lives--;
  game.invulnerableUntil = gameNow() + 700;
  spawnExplosion(game.playerX, game.playerY, 1);
  if (game.lives <= 0) finishGame(false);
}

// =============================================================
// INPUT DABBLE
// =============================================================
void pollDabble() {
  // Processar BLE em todas as iteracoes, sem limitar a 30 ms.
  Dabble.processInput();
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

  int ax = GamePad.getXaxisData();
  int ay = GamePad.getYaxisData();
  if (abs(ax) <= 1) ax = 0;
  if (abs(ay) <= 1) ay = 0;
  input.axisX = constrain(ax, -7, 7);
  input.axisY = constrain(ay, -7, 7);  // No Dabble, cima eh positivo.
}

// =============================================================
// ESTRELAS / CENARIOS
// =============================================================
void initStars() {
  const ScenarioInfo& sc = SCENARIOS[(int)game.scenario];
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(AREA_X, AREA_X + AREA_W);
    stars[i].y = random(AREA_Y, AREA_Y + AREA_H);
    stars[i].speed = randf(0.4f, 2.6f) * sc.starSpeedMul;
    stars[i].bright = random(90, 255);
  }
}

void updateStars() {
  const ScenarioInfo& sc = SCENARIOS[(int)game.scenario];
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].y += stars[i].speed;
    if (sc.diagonal) stars[i].x += stars[i].speed * 0.3f;
    if (stars[i].y > AREA_Y + AREA_H || stars[i].x > AREA_X + AREA_W) {
      stars[i].y = AREA_Y;
      stars[i].x = random(AREA_X, AREA_X + AREA_W);
    }
  }
}

void drawStars() {
  const ScenarioInfo& sc = SCENARIOS[(int)game.scenario];

  if (sc.bgTint != 0x0000) {
    canvas.fillRect(AREA_X, AREA_Y, AREA_W, AREA_H, sc.bgTint);
  }

  for (int i = 0; i < MAX_STARS; i++) {
    uint16_t c;
    if (game.scenario == SC_NEBULA) {
      uint8_t r = stars[i].bright;
      c = canvas.color565(r, r / 3, r);
    } else if (game.scenario == SC_CHAOS) {
      uint8_t r = stars[i].bright;
      c = canvas.color565(r, r / 2, r / 4);
    } else {
      c = sc.starColor == C_STAR ? canvas.color565(stars[i].bright, stars[i].bright, stars[i].bright) : sc.starColor;
    }

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
  game.lastEnemySpawn = 0;
  game.shieldTimer = 0;
  game.weapon = WPN_DEFAULT;
  game.weaponTimer = 0;
  game.nextBossLevel = 5;
  game.scenario = SC_SPACE;
  bossCooldownUntil = 0;
  game.invulnerableUntil = 0;
  game.gameOverHandled = false;
  game.newHighScore = false;
  game.wonThisGame = false;
  gameClockMs = 0;

  boss.active = false;

  for (int i = 0; i < MAX_BULLETS; i++)      bullets[i].active = false;
  for (int i = 0; i < MAX_ENEMY_BULLS; i++)  enemyBullets[i].active = false;
  for (int i = 0; i < MAX_ENEMIES; i++)      enemies[i].active = false;
  for (int i = 0; i < MAX_PARTICLES; i++)    particles[i].active = false;
  for (int i = 0; i < MAX_POWERUPS; i++)     powerups[i].active = false;

  initStars();
}

// =============================================================
// SPAWN
// =============================================================
void spawnBullet(float x, float y, float vx, float vy,
                 int dmg, int pierce,
                 uint16_t color, uint16_t colorLight) {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].x = x;
      bullets[i].y = y;
      bullets[i].vx = vx;
      bullets[i].vy = vy;
      bullets[i].dmg = dmg;
      bullets[i].pierce = pierce;
      bullets[i].guided = false;
      bullets[i].weapon = game.weapon;
      bullets[i].hitEnemies = 0;
      bullets[i].hitBoss = false;
      bullets[i].color = color;
      bullets[i].colorLight = colorLight;
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
      enemies[i].lastShot = gameNow();
      enemies[i].active = true;
      return;
    }
  }
}

void spawnParticles(float x, float y, int count, uint16_t color) {
  // Busca O(MAX_PARTICLES) por chamada, mesmo que o pool esteja cheio.
  // Evita o custo de reiniciar a busca desde zero para cada nova particula.
  static int nextParticle = 0;
  int spawned = 0;
  int pos = nextParticle;
  for (int checked = 0; checked < MAX_PARTICLES && spawned < count; ++checked) {
    int i = pos;
    pos = (pos + 1) % MAX_PARTICLES;
    if (particles[i].active) continue;
    float ang = randf(0, TWO_PI);
    float sp = randf(1.0f, 4.0f);
    particles[i].x = x; particles[i].y = y;
    particles[i].vx = cosf(ang) * sp;
    particles[i].vy = sinf(ang) * sp;
    particles[i].life = random(15, 30);
    particles[i].color = color;
    particles[i].active = true;
    spawned++;
  }
  nextParticle = pos;
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
      powerups[i].x = x;
      powerups[i].y = y;
      powerups[i].vy = 1.5f;

      int r = random(0, 100);
      if (r < 10)      powerups[i].type = 0;                  // escudo
      else if (r < 18) powerups[i].type = 1;                  // vida
      else             powerups[i].type = 2 + random(0, 5);   // arma 2..6

      powerups[i].active = true;
      return;
    }
  }
}

// =============================================================
// BOSS - SPAWN E ATAQUES
// =============================================================
void spawnBoss(BossType type) {
  boss.active = true;
  boss.type = type;
  boss.spawnLevel = game.nextBossLevel;

  const BossConfig& cfg = BOSS_CFG[(int)type];
  boss.maxHp = cfg.hp;
  boss.hp = cfg.hp;
  boss.w = cfg.w;
  boss.h = cfg.h;
  boss.x = AREA_X + AREA_W / 2.0f;
  boss.y = AREA_Y + 45;
  boss.vx = cfg.speed;
  boss.vy = 0;
  boss.phase = 1;
  boss.angleAccum = 0;
  boss.attackMode = 0;
  boss.lastAttack = gameNow();
  boss.attackInterval = cfg.baseAttackMs;
  boss.lastPhaseSwap = gameNow();
  boss.shielded = false;
  boss.shieldUntil = 0;

  if (GAME_EVENT_LOGS) Serial.printf("BOSS SPAWNED tipo=%d HP=%d\n", (int)type, boss.maxHp);
}

void bossShootAtPlayer(float speed, int count, float spreadRad) {
  float dx = game.playerX - boss.x;
  float dy = game.playerY - boss.y;
  float baseAng = atan2f(dy, dx);
  for (int i = 0; i < count; i++) {
    float a = baseAng + (i - (count - 1) / 2.0f) * spreadRad;
    spawnEnemyBullet(boss.x, boss.y, cosf(a) * speed, sinf(a) * speed);
  }
}

void bossShootRadial(int count, float speed, float offset) {
  for (int i = 0; i < count; i++) {
    float a = offset + (float)i * TWO_PI / count;
    spawnEnemyBullet(boss.x, boss.y, cosf(a) * speed, sinf(a) * speed);
  }
}

// ---- SENTINEL ----
void updateBossSentinel() {
  boss.x += boss.vx;
  if (boss.x < AREA_X + boss.w / 2) { boss.x = AREA_X + boss.w / 2; boss.vx = -boss.vx; }
  if (boss.x > AREA_X + AREA_W - boss.w / 2) { boss.x = AREA_X + AREA_W - boss.w / 2; boss.vx = -boss.vx; }

  if (gameNow() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = gameNow();
    bossShootAtPlayer(2.8f, (boss.phase == 2) ? 7 : 5, 0.25f);
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.4f) {
    boss.phase = 2;
    boss.attackInterval = BOSS_CFG[0].enragedAttackMs;
  }
}

// ---- HYDRA ----
void updateBossHydra() {
  boss.x += boss.vx;
  if (boss.x < AREA_X + boss.w / 2) { boss.x = AREA_X + boss.w / 2; boss.vx = -boss.vx; }
  if (boss.x > AREA_X + AREA_W - boss.w / 2) { boss.x = AREA_X + AREA_W - boss.w / 2; boss.vx = -boss.vx; }

  if (gameNow() - boss.lastAttack > boss.attackInterval / 3) {
    boss.lastAttack = gameNow();
    for (int h = -1; h <= 1; h++) {
      float hx = boss.x + h * 25;
      float hy = boss.y - 10;
      float dx = game.playerX - hx;
      float dy = game.playerY - hy;
      float a = atan2f(dy, dx) + randf(-0.15f, 0.15f);
      spawnEnemyBullet(hx, hy, cosf(a) * 3.0f, sinf(a) * 3.0f);
    }
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.35f) {
    boss.phase = 2;
    boss.attackInterval = BOSS_CFG[1].enragedAttackMs;
  }
}

// ---- PRIME ----
void updateBossPrime() {
  boss.angleAccum += 0.05f;
  boss.x += sinf(boss.angleAccum) * 2.5f;
  boss.y += cosf(boss.angleAccum * 0.7f) * 0.8f;
  boss.x = constrain(boss.x, AREA_X + boss.w / 2, AREA_X + AREA_W - boss.w / 2);
  boss.y = constrain(boss.y, AREA_Y + 30, AREA_Y + 90);

  if (gameNow() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = gameNow();
    if (gameNow() - boss.lastPhaseSwap > 4000) {
      boss.attackMode = (boss.attackMode + 1) % 2;
      boss.lastPhaseSwap = gameNow();
    }
    if (boss.attackMode == 0) {
      float off = boss.angleAccum * 2.0f;
      bossShootRadial((boss.phase == 2) ? 8 : 6, 2.5f, off);
    } else {
      boss.x = randf(AREA_X + 30, AREA_X + AREA_W - 30);
      boss.y = randf(AREA_Y + 35, AREA_Y + 80);
      bossShootAtPlayer(3.2f, (boss.phase == 2) ? 9 : 6, 0.3f);
    }
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.4f) {
    boss.phase = 2;
    boss.attackInterval = BOSS_CFG[2].enragedAttackMs;
  }
}

// ---- DEVOURER ----
void updateBossDevourer() {
  boss.x += boss.vx * 0.6f;
  if (boss.x < AREA_X + boss.w / 2) { boss.x = AREA_X + boss.w / 2; boss.vx = -boss.vx; }
  if (boss.x > AREA_X + AREA_W - boss.w / 2) { boss.x = AREA_X + AREA_W - boss.w / 2; boss.vx = -boss.vx; }

  if (!boss.shielded && gameNow() - boss.lastPhaseSwap > 8000) {
    boss.shielded = true;
    boss.shieldUntil = gameNow() + 2500;
    boss.lastPhaseSwap = gameNow();
    if (GAME_EVENT_LOGS) Serial.println("Devourer shielded!");
  }
  if (boss.shielded && (long)(gameNow() - boss.shieldUntil) >= 0) {
    boss.shielded = false;
  }

  if (boss.shielded) return;

  if (gameNow() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = gameNow();
    boss.angleAccum += 0.3f;
    for (int i = 0; i < 4; i++) {
      float a = boss.angleAccum + i * PI / 2;
      spawnEnemyBullet(boss.x, boss.y, cosf(a) * 3.5f, sinf(a) * 3.5f);
    }
    if (boss.phase == 2) {
      bossShootAtPlayer(2.5f, 5, 0.3f);
    }
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.5f) {
    boss.phase = 2;
    boss.attackInterval = BOSS_CFG[3].enragedAttackMs;
  }
}

// ---- OMEGA ----
void updateBossOmega() {
  boss.angleAccum += 0.08f;
  boss.x = AREA_X + AREA_W / 2 + sinf(boss.angleAccum) * (AREA_W / 2 - 50);
  boss.y = AREA_Y + 50 + cosf(boss.angleAccum * 1.3f) * 15;

  if (gameNow() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = gameNow();
    boss.attackMode = (boss.attackMode + 1) % 4;
    switch (boss.attackMode) {
      case 0: bossShootRadial(12, 2.5f, gameNow() / 500.0f); break;
      case 1: bossShootAtPlayer(3.0f, 9, 0.25f); break;
      case 2: bossShootRadial(16, 1.8f, 0); break;
      case 3: bossShootAtPlayer(4.0f, 3, 0.5f); break;
    }
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.4f) {
    boss.phase = 2;
    boss.attackInterval = BOSS_CFG[4].enragedAttackMs;
  }
}

void updateBoss() {
  if (!boss.active) return;

  switch (boss.type) {
    case BOSS_SENTINEL: updateBossSentinel(); break;
    case BOSS_HYDRA:    updateBossHydra();    break;
    case BOSS_PRIME:    updateBossPrime();    break;
    case BOSS_DEVOURER: updateBossDevourer(); break;
    case BOSS_OMEGA:    updateBossOmega();    break;
  }

  float dx = boss.x - game.playerX;
  float dy = boss.y - game.playerY;
  if (fabsf(dx) < boss.w / 2 && fabsf(dy) < boss.h / 2 + 10) {
    damagePlayer();
  }
}

void onBossKilled() {
  // Periodo de descanso apos a luta: evita 2 bosses consecutivos
  // se a recompensa ultrapassar varios niveis de uma vez.
  bossCooldownUntil = gameNow() + 1100;
  spawnExplosion(boss.x, boss.y, 5);
  spawnExplosion(boss.x - 25, boss.y, 3);
  spawnExplosion(boss.x + 25, boss.y, 3);
  spawnExplosion(boss.x, boss.y - 15, 3);

  int reward = 500 + (int)boss.type * 250;
  game.score += reward;

  spawnPowerUp(boss.x, boss.y);
  spawnPowerUp(boss.x - 30, boss.y);
  spawnPowerUp(boss.x + 30, boss.y);

  boss.active = false;
  game.lastEnemySpawn = gameNow();

  int nextScen = ((int)game.level / 5) % 5;
  game.scenario = (Scenario)nextScen;
  initStars();  // nova velocidade/cor do cenario imediatamente
  game.nextBossLevel += 5;

  if (GAME_EVENT_LOGS) Serial.printf("Boss derrotado! Proximo em nivel %d, cenario %s\n",
                game.nextBossLevel, SCENARIOS[nextScen].name);

  if (game.mode == MODE_CAMPAIGN && boss.spawnLevel == 20) {
    finishGame(true);
    if (GAME_EVENT_LOGS) Serial.println(">>> CAMPANHA COMPLETA! PARABENS! <<<");
  }
}

// =============================================================
// DESENHO - NAVE
// =============================================================
void drawPlayer() {
  int x = (int)game.playerX;
  int y = (int)game.playerY;

  uint16_t cMain, cLight, cDark;
  switch (save.currentSkin) {
    case SKIN_CYAN:   cMain = 0x07FF; cLight = 0xBFFF; cDark = 0x0410; break;
    case SKIN_GOLD:   cMain = 0xFE60; cLight = 0xFFF0; cDark = 0x8200; break;
    case SKIN_PURPLE: cMain = 0x8010; cLight = 0xE81F; cDark = 0x4008; break;
    default:          cMain = 0x07E0; cLight = 0x07FF; cDark = 0x0320; break;
  }
  if (game.shieldTimer > 0) cMain = C_POWERUP;

  canvas.fillTriangle(x, y - 10, x - 9, y + 9, x + 9, y + 9, 0x0000);
  canvas.fillTriangle(x - 5, y - 2, x - 12, y + 6, x - 5, y + 8, cDark);
  canvas.fillTriangle(x + 5, y - 2, x + 12, y + 6, x + 5, y + 8, cDark);
  canvas.fillTriangle(x, y - 13, x - 10, y + 8, x + 10, y + 8, cMain);
  canvas.fillTriangle(x, y - 13, x - 5, y - 2, x + 5, y - 2, cLight);
  canvas.fillCircle(x, y - 3, 3, 0x001F);
  canvas.fillCircle(x, y - 3, 2, 0x07FF);
  canvas.drawPixel(x, y - 4, 0xFFFF);
  canvas.drawPixel(x - 7, y + 3, cLight);
  canvas.drawPixel(x + 7, y + 3, cLight);

  int flameLen = 10 + ((millis() / 50) % 3) * 2;
  canvas.fillTriangle(x - 4, y + 8, x + 4, y + 8, x, y + 8 + flameLen, 0xFC00);
  canvas.fillTriangle(x - 2, y + 8, x + 2, y + 8, x, y + 6 + flameLen, 0xFFE0);
  canvas.fillTriangle(x - 1, y + 8, x + 1, y + 8, x, y + 4 + flameLen, 0xFFFF);

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

  if (boss.shielded) {
    canvas.drawCircle(x, y, boss.w / 2 + 8, 0x07FF);
    canvas.drawCircle(x, y, boss.w / 2 + 9, 0xFFFF);
  }

  switch (boss.type) {
    case BOSS_SENTINEL: {
      uint16_t c = (boss.phase == 2) ? 0xF800 : 0xC000;
      canvas.fillRoundRect(x - 35, y - 22, 70, 40, 8, 0x4000);
      canvas.fillRoundRect(x - 34, y - 21, 68, 38, 8, c);
      canvas.fillCircle(x, y, 10, 0x4000);
      canvas.fillCircle(x, y, 8, 0xFC00);
      canvas.fillCircle(x, y, 4, 0xFFFF);
      canvas.fillRect(x - 30, y + 10, 6, 12, 0xFFFF);
      canvas.fillRect(x + 24, y + 10, 6, 12, 0xFFFF);
      break;
    }
    case BOSS_HYDRA: {
      for (int h = -1; h <= 1; h++) {
        int hx = x + h * 28;
        canvas.fillCircle(hx, y - 10, 14, 0x8000);
        canvas.fillCircle(hx, y - 10, 12, 0xF81F);
        canvas.fillCircle(hx, y - 10, 6, 0xFFE0);
        canvas.fillCircle(hx - 3, y - 13, 2, 0xFFFF);
        canvas.fillCircle(hx + 3, y - 13, 2, 0xFFFF);
      }
      canvas.fillRoundRect(x - 45, y + 5, 90, 22, 6, 0x8000);
      canvas.fillRoundRect(x - 44, y + 6, 88, 20, 6, 0xF81F);
      break;
    }
    case BOSS_PRIME: {
      uint16_t c = (boss.phase == 2) ? 0xF800 : 0xFD20;
      canvas.fillRoundRect(x - 40, y - 28, 80, 52, 10, 0x4000);
      canvas.fillRoundRect(x - 39, y - 27, 78, 50, 10, c);
      int r = 12 + (int)(sinf(millis() / 200.0f) * 3);
      canvas.fillCircle(x, y - 3, r, 0xFFE0);
      canvas.fillCircle(x, y - 3, r - 4, 0xFFFF);
      canvas.fillCircle(x, y - 3, 3, 0x001F);
      canvas.fillTriangle(x - 40, y - 28, x - 50, y - 18, x - 40, y - 8, c);
      canvas.fillTriangle(x + 40, y - 28, x + 50, y - 18, x + 40, y - 8, c);
      break;
    }
    case BOSS_DEVOURER: {
      uint16_t c = boss.shielded ? 0x07FF : 0x8010;
      canvas.fillRoundRect(x - 50, y - 30, 100, 55, 12, 0x2000);
      canvas.fillRoundRect(x - 49, y - 29, 98, 53, 12, c);
      canvas.fillCircle(x - 20, y - 5, 10, 0x0000);
      canvas.fillCircle(x + 20, y - 5, 10, 0x0000);
      canvas.fillCircle(x - 20, y - 5, 8, 0xF800);
      canvas.fillCircle(x + 20, y - 5, 8, 0xF800);
      canvas.fillCircle(x - 18, y - 7, 3, 0xFFFF);
      canvas.fillCircle(x + 22, y - 7, 3, 0xFFFF);
      canvas.fillRect(x - 25, y + 12, 50, 6, 0x0000);
      for (int i = 0; i < 10; i++) {
        canvas.drawPixel(x - 22 + i * 5, y + 12, 0xFFFF);
        canvas.drawPixel(x - 22 + i * 5, y + 17, 0xFFFF);
      }
      if (!boss.shielded) {
        float ang = millis() / 200.0f;
        for (int i = 0; i < 4; i++) {
          float a = ang + i * PI / 2;
          int ex = x + (int)(cosf(a) * 40);
          int ey = y + (int)(sinf(a) * 40);
          canvas.fillCircle(ex, ey, 3, 0xF800);
        }
      }
      break;
    }
    case BOSS_OMEGA: {
      float ang = millis() / 300.0f;
      for (int arm = 0; arm < 6; arm++) {
        float a = ang + arm * PI / 3;
        int ex = x + (int)(cosf(a) * 45);
        int ey = y + (int)(sinf(a) * 45);
        canvas.drawLine(x, y, ex, ey, 0xF800);
        canvas.fillCircle(ex, ey, 5, 0xFFE0);
        canvas.fillCircle(ex, ey, 3, 0xFFFF);
      }
      canvas.fillCircle(x, y, 18, 0x8000);
      canvas.fillCircle(x, y, 14, 0xF800);
      canvas.fillCircle(x, y, 8, 0xFFE0);
      canvas.fillCircle(x, y, 4, 0xFFFF);
      break;
    }
  }
}

// =============================================================
// HUD
// =============================================================
void drawUI() {
  canvas.setTextColor(C_UI, C_BG);
  canvas.setTextSize(1);

  canvas.setCursor(6, 6);
  canvas.printf("SCORE %d", game.score);

  canvas.setCursor(SCREEN_W / 2 - 40, 6);
  if (game.mode == MODE_CAMPAIGN) {
    canvas.printf("CAMP %d/20", game.level);
  } else {
    canvas.printf("ENDLESS %d", game.level);
  }

  for (int i = 0; i < game.lives; i++) {
    int hx = SCREEN_W - 14 - i * 14;
    canvas.fillTriangle(hx, 8, hx - 5, 18, hx + 5, 18, C_PLAYER);
    canvas.drawPixel(hx, 12, 0xFFFF);
  }

  if (game.shieldTimer > 0) {
    canvas.fillRect(6, 18, 50 * game.shieldTimer / 800, 3, C_POWERUP);
    canvas.drawRect(5, 17, 52, 5, C_POWERUP);
  }

  if (game.weapon != WPN_DEFAULT) {
    const WeaponInfo& wi = WEAPONS[(int)game.weapon];
    canvas.setTextColor(wi.color, C_BG);
    canvas.setCursor(65, 19);
    canvas.printf("%s", wi.name);
    if (game.weaponTimer > 0) {
      canvas.fillRect(65, 28, 50 * game.weaponTimer / 900, 2, wi.color);
    }
  }

  if (game.combo > 1 && gameNow() - game.lastKillTime < 1500) {
    canvas.setTextColor(C_BULLET, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(SCREEN_W / 2 - 18, 90);
    canvas.printf("x%d", game.combo);
    canvas.setTextSize(1);
  }

  canvas.drawRect(AREA_X - 1, AREA_Y - 1, AREA_W + 2, AREA_H + 2, C_BORDER);

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
// UPDATE - JOGADOR
// =============================================================
void updatePlayer() {
  const float SPEED = 6.0f;
  float dx = 0, dy = 0;
  bool digital = input.up || input.down || input.left || input.right;
  if (digital) {
    if (input.up) dy -= 1.0f;
    if (input.down) dy += 1.0f;
    if (input.left) dx -= 1.0f;
    if (input.right) dx += 1.0f;
  } else {
    dx = input.axisX / 7.0f;
    dy = -(input.axisY / 7.0f); // Y na tela cresce para baixo
  }
  float mag2 = dx * dx + dy * dy;
  if (mag2 > 1.0f) {
    float inv = 1.0f / sqrtf(mag2);
    dx *= inv;
    dy *= inv;
  }
  game.playerX += dx * SPEED;
  game.playerY += dy * SPEED;
  game.playerX = constrain(game.playerX, (float)(AREA_X + 12),
                          (float)(AREA_X + AREA_W - 12));
  game.playerY = constrain(game.playerY, (float)(AREA_Y + 12),
                          (float)(AREA_Y + AREA_H - 12));
}

// =============================================================
// ARMAS
// =============================================================
void fireWeapon() {
  const WeaponInfo& w = WEAPONS[(int)game.weapon];

  switch (game.weapon) {
    case WPN_DEFAULT:
      spawnBullet(game.playerX, game.playerY - 12, 0, -8, w.dmg, 0, w.color, w.colorLight);
      break;
    case WPN_DOUBLE:
      spawnBullet(game.playerX - 5, game.playerY - 12, 0, -8, w.dmg, 0, w.color, w.colorLight);
      spawnBullet(game.playerX + 5, game.playerY - 12, 0, -8, w.dmg, 0, w.color, w.colorLight);
      break;
    case WPN_TRIPLE:
      spawnBullet(game.playerX,     game.playerY - 12, 0,  -8, w.dmg, 0, w.color, w.colorLight);
      spawnBullet(game.playerX - 8, game.playerY - 6,  -1, -8, w.dmg, 0, w.color, w.colorLight);
      spawnBullet(game.playerX + 8, game.playerY - 6,   1, -8, w.dmg, 0, w.color, w.colorLight);
      break;
    case WPN_SPREAD:
      for (int i = -2; i <= 2; i++) {
        spawnBullet(game.playerX, game.playerY - 10, i * 1.5f, -7, w.dmg, 0, w.color, w.colorLight);
      }
      break;
    case WPN_LASER:
      spawnBullet(game.playerX, game.playerY - 12, 0, -12, w.dmg, 3, w.color, w.colorLight);
      break;
    case WPN_MISSILE: {
      int idx1 = -1, idx2 = -1;
      for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
          if (idx1 < 0) idx1 = i;
          else if (idx2 < 0) { idx2 = i; break; }
        }
      }
      if (idx1 >= 0) {
        bullets[idx1].x = game.playerX - 8;
        bullets[idx1].y = game.playerY - 10;
        bullets[idx1].vx = -0.5f;
        bullets[idx1].vy = -6;
        bullets[idx1].dmg = w.dmg;
        bullets[idx1].pierce = 0;
        bullets[idx1].guided = true;
        bullets[idx1].weapon = WPN_MISSILE;
        bullets[idx1].hitEnemies = 0;
        bullets[idx1].hitBoss = false;
        bullets[idx1].color = w.color;
        bullets[idx1].colorLight = w.colorLight;
        bullets[idx1].active = true;
      }
      if (idx2 >= 0) {
        bullets[idx2].x = game.playerX + 8;
        bullets[idx2].y = game.playerY - 10;
        bullets[idx2].vx = 0.5f;
        bullets[idx2].vy = -6;
        bullets[idx2].dmg = w.dmg;
        bullets[idx2].pierce = 0;
        bullets[idx2].guided = true;
        bullets[idx2].weapon = WPN_MISSILE;
        bullets[idx2].hitEnemies = 0;
        bullets[idx2].hitBoss = false;
        bullets[idx2].color = w.color;
        bullets[idx2].colorLight = w.colorLight;
        bullets[idx2].active = true;
      }
      break;
    }
  }
}

void tryFire() {
  bool fire = input.cross || input.circle;
  int cooldown = WEAPONS[(int)game.weapon].cooldown - (180 - (int)game.fireCooldown);
  if (cooldown < 90) cooldown = 90;
  if (fire && gameNow() - game.lastFireTime > (unsigned long)cooldown) {
    fireWeapon();
    game.lastFireTime = gameNow();
  }
}

void updateBullets() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) continue;

    if (bullets[i].guided) {
      float bestD = 99999;
      int bestIdx = -1;
      for (int e = 0; e < MAX_ENEMIES; e++) {
        if (!enemies[e].active) continue;
        float dx = enemies[e].x - bullets[i].x;
        float dy = enemies[e].y - bullets[i].y;
        float d = dx * dx + dy * dy;
        if (d < bestD) { bestD = d; bestIdx = e; }
      }
      // Também mira no boss se estiver ativo
      if (boss.active) {
        float dx = boss.x - bullets[i].x;
        float dy = boss.y - bullets[i].y;
        float d = dx * dx + dy * dy;
        if (d < bestD) { bestD = d; bestIdx = 999; }
      }

      if (bestIdx >= 0) {
        float tx, ty;
        if (bestIdx == 999) { tx = boss.x; ty = boss.y; }
        else { tx = enemies[bestIdx].x; ty = enemies[bestIdx].y; }

        float dx = tx - bullets[i].x;
        float dy = ty - bullets[i].y;
        float len = sqrtf(dx * dx + dy * dy);
        if (len > 1) {
          float targetVX = (dx / len) * 6.0f;
          float targetVY = (dy / len) * 6.0f;
          bullets[i].vx = bullets[i].vx * 0.8f + targetVX * 0.2f;
          bullets[i].vy = bullets[i].vy * 0.8f + targetVY * 0.2f;
        }
      }
    }

    bullets[i].x += bullets[i].vx;
    bullets[i].y += bullets[i].vy;

    if (bullets[i].y < AREA_Y - 10 ||
        bullets[i].y > AREA_Y + AREA_H + 10 ||
        bullets[i].x < AREA_X - 10 ||
        bullets[i].x > AREA_X + AREA_W + 10) {
      bullets[i].active = false;
    }
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
      } else damagePlayer();
      enemyBullets[i].active = false;
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
      if (gameNow() - enemies[i].lastShot > 1400 && random(0, 100) < 8) {
        float dx = game.playerX - enemies[i].x;
        float dy = game.playerY - enemies[i].y;
        float d = sqrtf(dx*dx + dy*dy);
        if (d > 5) {
          float sp = 3.0f;
          spawnEnemyBullet(enemies[i].x, enemies[i].y + 8,
                           dx / d * sp, dy / d * sp);
        }
        enemies[i].lastShot = gameNow();
      }
    }

    float dx = enemies[i].x - game.playerX;
    float dy = enemies[i].y - game.playerY;
    if (fabsf(dx) < (enemies[i].w / 2 + game.playerW / 2 - 4) &&
        fabsf(dy) < (enemies[i].h / 2 + game.playerH / 2 - 4)) {
      if (game.shieldTimer > 0) {
        spawnParticles(enemies[i].x, enemies[i].y, 10, C_POWERUP);
      } else damagePlayer();
      enemies[i].active = false;
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
    Bullet& bullet = bullets[b];
    if (!bullet.active) continue;

    if (boss.active && !bullet.hitBoss &&
        fabsf(bullet.x - boss.x) < boss.w / 2 &&
        fabsf(bullet.y - boss.y) < boss.h / 2) {
      bullet.hitBoss = true;
      if (boss.type == BOSS_DEVOURER && boss.shielded) {
        spawnParticles(bullet.x, bullet.y, 3, 0x07FF);
        bullet.active = false;
        continue;
      }
      boss.hp -= bullet.dmg;
      spawnParticles(bullet.x, bullet.y, 3, bullet.color);
      if (bullet.pierce > 0) bullet.pierce--;
      else bullet.active = false;
      if (boss.hp <= 0) onBossKilled();
      if (game.state != PLAYING) return;
      if (!bullet.active) continue;
    }

    for (int e = 0; e < MAX_ENEMIES; e++) {
      if (!bullet.active) break;
      if (!enemies[e].active || (bullet.hitEnemies & (1UL << e))) continue;
      if (fabsf(bullet.x - enemies[e].x) >= enemies[e].w / 2 ||
          fabsf(bullet.y - enemies[e].y) >= enemies[e].h / 2) continue;
      bullet.hitEnemies |= (1UL << e); // Cada alvo no maximo uma vez por bala
      enemies[e].hp -= bullet.dmg;
      spawnParticles(bullet.x, bullet.y, 3, bullet.color);
      if (bullet.pierce > 0) bullet.pierce--;
      else bullet.active = false;

      if (enemies[e].hp <= 0) {
        int pts = (enemies[e].type == 0) ? 10 :
                  (enemies[e].type == 1) ? 25 :
                  (enemies[e].type == 2) ? 60 : 80;
        unsigned long now = gameNow();
        if (game.combo > 0 && now - game.lastKillTime < 1500) {
          if (game.combo < 10) game.combo++;
        } else game.combo = 1;
        game.lastKillTime = now;
        int mult = (game.combo >= 5) ? 3 : (game.combo >= 3) ? 2 : 1;
        game.score += pts * mult;
        spawnExplosion(enemies[e].x, enemies[e].y,
                       (enemies[e].type == 2) ? 2 : 1);
        if (random(0, 100) < 14) spawnPowerUp(enemies[e].x, enemies[e].y);
        enemies[e].active = false;
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
      if (powerups[i].type == 0) {
        game.shieldTimer = 800;
      } else if (powerups[i].type == 1) {
        if (game.lives < 5) game.lives++;
      } else {
        game.weapon = (WeaponType)(powerups[i].type - 1);
        game.weaponTimer = 900;
        if (GAME_EVENT_LOGS) Serial.printf("Arma: %s\n", WEAPONS[(int)game.weapon].name);
      }
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
  if (game.shieldTimer > 0) game.shieldTimer--;
  if (game.weaponTimer > 0) {
    game.weaponTimer--;
    if (game.weaponTimer == 0 && game.weapon != WPN_DEFAULT) {
      game.weapon = WPN_DEFAULT;
      if (GAME_EVENT_LOGS) Serial.println("Arma voltou para PADRAO");
    }
  }

  int ptsPerLevel = 600;
  int maxLevel = (game.mode == MODE_CAMPAIGN) ? 20 : 999;
  int newLevel = 1 + game.score / ptsPerLevel;
  if (newLevel > maxLevel) newLevel = maxLevel;

  if (newLevel > game.level) {
    game.level = newLevel;
    if (game.enemySpawnInterval > 300) game.enemySpawnInterval -= 40;
    if (game.fireCooldown > 90)       game.fireCooldown       -= 5;
    if (GAME_EVENT_LOGS) Serial.printf("Nivel %d!\n", game.level);
  }

  if (!boss.active && game.level >= game.nextBossLevel &&
      gameNow() >= bossCooldownUntil) {
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_ENEMY_BULLS; i++) enemyBullets[i].active = false;

    BossType bt;
    if (game.mode == MODE_CAMPAIGN) {
      switch (game.nextBossLevel) {
        case 5:  bt = BOSS_SENTINEL; break;
        case 10: bt = BOSS_HYDRA;    break;
        case 15: bt = BOSS_PRIME;    break;
        case 20: bt = BOSS_DEVOURER; break;
        default: bt = BOSS_OMEGA;    break;
      }
    } else {
      int idx = (game.nextBossLevel / 5 - 1) % 5;
      bt = (BossType)idx;
    }
    spawnBoss(bt);
  }

  if (!boss.active && gameNow() - game.lastEnemySpawn > game.enemySpawnInterval) {
    spawnEnemy();
    game.lastEnemySpawn = gameNow();
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
      canvas.fillCircle(px, py, 8, C_POWERUP);
      canvas.drawCircle(px, py, 8, 0xFFFF);
      canvas.drawCircle(px, py, 5, 0x001F);
      canvas.setTextColor(0x0000, C_POWERUP);
      canvas.setTextSize(1);
      canvas.setCursor(px - 3, py - 3);
      canvas.print("S");
    } else if (powerups[i].type == 1) {
      canvas.fillCircle(px - 4, py - 3, 4, 0xF800);
      canvas.fillCircle(px + 4, py - 3, 4, 0xF800);
      canvas.fillTriangle(px - 8, py - 1, px + 8, py - 1, px, py + 8, 0xF800);
      canvas.drawPixel(px - 4, py - 4, 0xFFFF);
    } else {
      WeaponType wt = (WeaponType)(powerups[i].type - 1);
      const WeaponInfo& wi = WEAPONS[(int)wt];
      canvas.fillCircle(px, py, 9, wi.color);
      canvas.drawCircle(px, py, 9, 0xFFFF);
      canvas.drawCircle(px, py, 6, wi.colorLight);

      canvas.setTextColor(0x0000, wi.color);
      canvas.setTextSize(1);
      canvas.setCursor(px - 3, py - 3);
      switch (wt) {
        case WPN_DOUBLE:  canvas.print("2"); break;
        case WPN_TRIPLE:  canvas.print("3"); break;
        case WPN_SPREAD:  canvas.print("W"); break;
        case WPN_LASER:   canvas.print("L"); break;
        case WPN_MISSILE: canvas.print("M"); break;
        default:          canvas.print("?"); break;
      }
    }
  }

  // Inimigos
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) drawEnemy(enemies[i]);
  }

  // Boss
  drawBoss();

  // Balas do jogador
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) continue;
    int bx = (int)bullets[i].x, by = (int)bullets[i].y;

    int trailX = bx - (int)(bullets[i].vx * 0.5f);
    int trailY = by - (int)(bullets[i].vy * 0.5f);
    canvas.drawPixel(trailX, trailY, 0x4208);

    if (bullets[i].weapon == WPN_LASER) {
      canvas.fillRect(bx - 1, by - 8, 3, 16, bullets[i].color);
      canvas.drawRect(bx - 1, by - 8, 3, 16, bullets[i].colorLight);
      canvas.drawPixel(bx, by - 9, 0xFFFF);
    } else if (bullets[i].guided) {
      canvas.fillTriangle(bx, by - 6, bx - 3, by + 4, bx + 3, by + 4, bullets[i].color);
      canvas.drawPixel(bx, by - 5, 0xFFFF);
    } else {
      canvas.fillRect(bx - 1, by - 4, 2, 8, bullets[i].color);
      canvas.drawPixel(bx, by - 5, bullets[i].colorLight);
      canvas.drawPixel(bx, by - 3, 0xFFFF);
    }
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

  // MENU
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
    canvas.setTextColor(C_DIM, C_BG);
    canvas.setTextSize(1);
    canvas.setCursor(SCREEN_W - 44, SCREEN_H - 11);
    canvas.print(GAME_VERSION);
  }

  // MODE SELECT
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
    canvas.setCursor(20, 200); canvas.print("CAMPANHA = 20 niveis + 4 bosses");
  }

  // SKIN SELECT
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

      int px = 40, py = y + 12;
      canvas.fillTriangle(px, py - 10, px - 8, py + 8, px + 8, py + 8, c);
      canvas.fillCircle(px, py - 2, 2, 0xFFFF);

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

  // RESET CONFIRM
  if (game.state == RESET_CONFIRM) {
    canvas.fillRect(25, 72, 270, 98, C_BG);
    canvas.drawRect(25, 72, 270, 98, C_ENEMY1);
    canvas.setTextColor(C_ENEMY1, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(64, 86); canvas.print("RESET DATA?");
    canvas.setTextSize(1);
    canvas.setTextColor(C_UI, C_BG);
    canvas.setCursor(56, 121); canvas.print("CROSS = APAGAR PROGRESSO");
    canvas.setCursor(76, 146); canvas.print("SELECT = CANCELAR");
  }

  // SCORES
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

    int unlockedCount = __builtin_popcount(save.unlockedSkins);
    canvas.setCursor(45, 150);
    canvas.printf("SKINS:       %d / %d", unlockedCount, SKIN_COUNT);

    canvas.setCursor(45, 170);
    if (save.campaign20Won) {
      canvas.setTextColor(0x07E0, C_BG);
      canvas.print("CAMPANHA:    VENCIDA!");
    } else {
      canvas.setTextColor(0xF800, C_BG);
      canvas.print("CAMPANHA:    nao vencida");
    }

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(70, 210); canvas.print("CROSS/SELECT p/ voltar");
  }

  // PAUSA
  if (game.state == PAUSED) {
    canvas.fillRect(90, 95, 140, 50, C_BG);
    canvas.drawRect(90, 95, 140, 50, TFT_YELLOW);
    canvas.drawRect(92, 97, 136, 46, TFT_YELLOW);
    canvas.setTextColor(TFT_YELLOW, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(125, 113); canvas.print("PAUSA");
    canvas.setTextSize(1);
    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(58, 155);
    canvas.printf("DRAW %lu ms  SPI %lu ms", (unsigned long)(perf.maxDrawUs / 1000),
                  (unsigned long)(perf.maxPushUs / 1000));
    canvas.setCursor(58, 166);
    canvas.printf("LOGIC %lu ms  SLOW %lu", (unsigned long)(perf.maxLogicUs / 1000),
                  (unsigned long)perf.slowDraws);
  }

  // GAME OVER
  if (game.state == GAME_OVER) {
    canvas.fillRect(30, 70, SCREEN_W - 60, 110, C_BG);
    canvas.drawRect(30, 70, SCREEN_W - 60, 110, C_ENEMY1);
    canvas.drawRect(32, 72, SCREEN_W - 64, 106, C_ENEMY1);

    bool venceu = game.wonThisGame;

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

  unsigned long pushStartUs = micros();
  canvas.pushSprite(0, 0);
  uint32_t pushUs = (uint32_t)(micros() - pushStartUs);
  if (pushUs > perf.maxPushUs) perf.maxPushUs = pushUs;
}

// =============================================================
// SETUP
// =============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println(" SPACE SHOOTER v4.0.0");
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

  game.scenario = SC_SPACE;
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
unsigned long lastDisplay = 0;
GameState lastDisplayedState = GAME_OVER; // Forca a primeira renderizacao
const unsigned long FRAME_MS = 16;

bool prevStart = false;
bool prevSelect = false;
bool prevCross = false;
bool prevUp = false;
bool prevDown = false;

bool navUpNow() {
  if (input.up || input.down) return input.up;
  return input.axisY > 2;  // Dabble: positivo = cima
}
bool navDownNow() {
  if (input.up || input.down) return input.down;
  return input.axisY < -2;
}

void loop() {
  pollDabble();
  unsigned long now = millis();
  if (now - lastFrame < FRAME_MS) return;
  unsigned long dt = now - lastFrame;
  lastFrame = now;
  if (dt > 50) dt = 50; // evita saltos enormes apos atraso no hardware

  uint32_t logicStartUs = micros();
  bool navUp = navUpNow(), navDown = navDownNow();
  bool pressUp = navUp && !prevUp;
  bool pressDown = navDown && !prevDown;
  bool pressCross = input.cross && !prevCross;
  bool pressStart = input.start && !prevStart;
  bool pressSelect = input.select && !prevSelect;

  // Um unico estado e tratado por frame: impede que botoes atravessem telas.
  GameState stateAtFrameStart = game.state;
  if (stateAtFrameStart == PLAYING && (pressStart || pressSelect)) {
    if (pressStart) game.state = PAUSED;
    // SELECT durante o jogo nao encerra a partida.
  } else if (stateAtFrameStart == PAUSED) {
    if (pressSelect) game.state = MENU;
    else if (pressStart) game.state = PLAYING;
  } else if (pressSelect && stateAtFrameStart != MENU && stateAtFrameStart != PLAYING) {
    if (stateAtFrameStart == GAME_OVER) commitFinishedGame();
    game.state = MENU;
  }

  // Navegacao e processada conforme estado de entrada no frame.
  if (game.state == MENU && stateAtFrameStart == MENU) {
    if (pressUp) menuSelection = (menuSelection + MENU_ITEMS - 1) % MENU_ITEMS;
    if (pressDown) menuSelection = (menuSelection + 1) % MENU_ITEMS;
    if (pressCross || pressStart) {
      if (menuSelection == 0) { modeSelection = 0; game.state = MODE_SELECT; }
      else if (menuSelection == 1) { skinSelection = save.currentSkin; game.state = SKIN_SELECT; }
      else if (menuSelection == 2) game.state = SCORES;
      else game.state = RESET_CONFIRM;
    }
    updateStars();
  }
  else if (game.state == MODE_SELECT && stateAtFrameStart == MODE_SELECT) {
    if (pressUp || pressDown) modeSelection = (modeSelection + 1) % MODE_ITEMS;
    if (pressCross || pressStart) {
      game.mode = (modeSelection == 0) ? MODE_CAMPAIGN : MODE_ENDLESS;
      resetGame();
      game.state = PLAYING;
    }
  }
  else if (game.state == SKIN_SELECT && stateAtFrameStart == SKIN_SELECT) {
    if (pressUp) skinSelection = (skinSelection + SKIN_COUNT - 1) % SKIN_COUNT;
    if (pressDown) skinSelection = (skinSelection + 1) % SKIN_COUNT;
    if (pressCross || pressStart) {
      if (skinUnlocked(skinSelection)) {
        save.currentSkin = skinSelection;
        saveAll();
        game.state = MENU;
      }
    }
  }
  else if (game.state == RESET_CONFIRM && stateAtFrameStart == RESET_CONFIRM) {
    if (pressCross) {
      resetSave();
      skinSelection = 0;
      game.state = MENU;
    }
  }
  else if (game.state == SCORES && stateAtFrameStart == SCORES) {
    if (pressCross || pressStart) game.state = MENU;
  }
  else if (game.state == PLAYING && stateAtFrameStart == PLAYING) {
    gameClockMs += dt;
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
    updateParticles();
  }
  else if (game.state == GAME_OVER && stateAtFrameStart == GAME_OVER) {
    updateStars();
    updateParticles();
    if (pressStart) {
      commitFinishedGame();  // salva antes de sair da tela de resultado
      game.state = MENU;
    } else if (pendingGameSave && millis() - gameFinishedWallMs >= 900) {
      commitFinishedGame();  // flash apenas quando resultado ja foi exibido
    }
  }
  // PAUSED: nenhuma atualizacao de gameplay nem de relogio.

  uint32_t logicUs = (uint32_t)(micros() - logicStartUs);
  if (logicUs > perf.maxLogicUs) perf.maxLogicUs = logicUs;
  // Atualiza a logica/controle mais frequentemente que a tela.
  // Enviar 320x240 em RGB565 custa no minimo ~31 ms a 40MHz SPI.
  unsigned long displayNow = millis();
  if (displayNow - lastDisplay >= DISPLAY_FRAME_MS ||
      game.state != lastDisplayedState) {
    lastDisplay = displayNow;  // Inicio da transferencia: cadencia inicio-a-inicio
    uint32_t drawStartUs = micros();
    render();
    uint32_t drawUs = (uint32_t)(micros() - drawStartUs);
    if (drawUs > perf.maxDrawUs) perf.maxDrawUs = drawUs;
    if (drawUs > DISPLAY_FRAME_MS * 1000UL) ++perf.slowDraws;
    lastDisplayedState = game.state;
  }
  prevStart = input.start;
  prevSelect = input.select;
  prevCross = input.cross;
  prevUp = navUp;
  prevDown = navDown;
}