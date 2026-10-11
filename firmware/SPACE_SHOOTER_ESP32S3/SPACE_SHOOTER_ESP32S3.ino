/*
   =============================================================
   SPACE SHOOTER v6.0.0
   ESP32-S3 N16R8 + ILI9341 2.8" + Dabble BLE
   =============================================================

   NOVIDADES v6.6.0 (base corrigida v6.5.6):
     - BOSS RUSH: 10 bosses seguidos, desbloqueado por campanha ou debug
     - Silhuetas MECHAPUNK distintas e skins individuais por nave
     - Menu CONFIGURACOES com CONTROLES, STATUS e RESET
     - BOMBA funcionando (SQUARE para usar)
     - Konami Code na LOJA: cima cima baixo baixo
       esquerda direita esquerda direita + CROSS + CIRCLE
       => 999.999 moedas (modo debug)
     - Popup visual "MODO DEBUG ATIVADO"
     - Versao do jogo no canto inferior direito do menu
     - useBomb() completo (inimigos + boss)
     - Throttle de NVS + yield no loop
*/

#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <DabbleESP32.h>
#include <Preferences.h>
#include <math.h>

#define GAME_VERSION "v6.6.0"

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
#define MAX_BULLETS     32
#define MAX_ENEMY_BULLS 40
#define MAX_ENEMIES     28
#define MAX_PARTICLES   80
#define MAX_STARS       28
#define MAX_POWERUPS     6
#define MAX_COINS       16
#define MAX_PETS         5

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
#define C_ENEMY5      0x07FF
#define C_ENEMY6      0xFFE0
#define C_ENEMY7      0xFFFF
#define C_ENEMY8      0x8BFF
#define C_ENEMY9      0xFC00
#define C_COIN        0xFFE0
#define C_COIN_LIGHT  0xFFF0
#define C_EXPLOSION   0xFC00
#define C_POWERUP     0x07FF
#define C_UI          0xFFFF
#define C_BORDER      0x4208
#define C_HIGHLIGHT   0x0841
#define C_DIM         0x7BEF
#define C_HP_BG       0x3000
#define C_DEBUG       0xFFE0

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

const uint8_t ALL_WEAPONS_MASK = 0x3F;

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
// CONTROLE
// =============================================================
enum ControlMode {
  CTRL_DIGITAL = 0,
  CTRL_JOYSTICK,
  CTRL_AUTO
};

const int CTRL_COUNT = 3;
const char* ctrlLabels[3] = { "DIGITAL", "JOYSTICK", "AUTO" };

// =============================================================
// NAVES + PALETAS
// =============================================================
enum ShipType {
  SHIP_STANDARD = 0,
  SHIP_SCOUT,
  SHIP_HEAVY,
  SHIP_LASER,
  SHIP_HUNTER
};

struct ShipPalette {
  uint16_t cMain;
  uint16_t cLight;
  uint16_t cDark;
};

struct ShipInfo {
  const char* name;
  const char* desc;
  WeaponType  startWeapon;
  float       speed;
  int         startLives;
  int         cost;
  ShipPalette palettes[4];
};

const ShipInfo SHIPS[] = {
  { "PADRAO", "Equilibrada", WPN_DEFAULT, 1.00f, 0, 0,
    { {0x07E0, 0x07FF, 0x0320}, {0x07FF, 0xBFFF, 0x0410},
      {0xFE60, 0xFFF0, 0x8200}, {0x8010, 0xE81F, 0x4008} }
  },
  { "EXPLORADORA", "Rapida + duplo", WPN_DOUBLE, 1.30f, 0, 500,
    { {0x001F, 0x07FF, 0x0008}, {0x07FF, 0xBFFF, 0x0410},
      {0xFFFF, 0xCE79, 0x8410}, {0x07E0, 0xAFF0, 0x0320} }
  },
  { "PESADA", "3 tiros + lenta", WPN_TRIPLE, 0.85f, 1, 800,
    { {0xFD20, 0xFFE0, 0x8000}, {0xF800, 0xFC00, 0x4000},
      {0xFFE0, 0xFFF0, 0x8400}, {0x8410, 0xC618, 0x2104} }
  },
  { "LASER", "Laser perfurante", WPN_LASER, 1.00f, 0, 1200,
    { {0xF800, 0xFC00, 0x8000}, {0xF81F, 0xFF9F, 0x4008},
      {0xFFFF, 0xCE79, 0x8410}, {0x07FF, 0xBFFF, 0x0410} }
  },
  { "CACADORA", "Misseis teleguiados", WPN_MISSILE, 1.05f, 0, 1500,
    { {0x8010, 0xE81F, 0x4008}, {0x4000, 0x8000, 0x2000},
      {0x4208, 0x8410, 0x2104}, {0xFE60, 0xFFF0, 0x8200} }
  }
};

const int SHIP_COUNT = 5;

// =============================================================
// SKINS (paleta dentro de cada nave)
// =============================================================
const int SKIN_COUNT = 4;
const char* shipSkinNames[SHIP_COUNT][SKIN_COUNT] = {
  {"NEON VERDE", "TITANIO", "AREIA", "NIGHT OPS"},
  {"PLASMA AZUL", "ARCTIC", "SUNSET", "PHANTOM"},
  {"FORJA", "VULCAN", "GUARDIAN", "DARK STEEL"},
  {"NOVA RUBRA", "SPECTRUM", "WHITE CORE", "QUANTUM"},
  {"VOID VIOLET", "OBSIDIAN", "GHOST", "SOLAR FLARE"}
};

// =============================================================
// UPGRADES
// =============================================================
enum UpgradeType {
  UPG_LIVES = 0,
  UPG_SHIELD,
  UPG_FIRERATE,
  UPG_DAMAGE,
  UPG_SPEED,
  UPG_DASH,
  UPG_CRIT,
  UPG_PIERCE,
  UPG_COIN_MAGNET,
  UPG_COIN_VALUE,
  UPG_REVIVE,
  UPG_START_BOMB,
  UPG_COUNT
};

struct UpgradeInfo {
  const char* name;
  const char* desc;
  int  maxLevel;
  int  baseCost;
  int  costStep;
};

const UpgradeInfo UPGRADES[UPG_COUNT] = {
  { "VIDAS",       "+1 vida inicial",     5, 300, 200 },
  { "ESCUDO",      "Escudo +2s",          5, 250, 150 },
  { "CADENCIA",    "Tiro +rapido",        5, 300, 200 },
  { "DANO",        "+1 dano por tiro",    3, 800, 600 },
  { "VELOCIDADE",  "Nave +rapida",        5, 250, 150 },
  { "DASH",        "Cooldown -20%",       3, 400, 300 },
  { "CRITICO",     "+10% chance critico", 5, 600, 400 },
  { "PERFURACAO",  "Tiro atravessa +1",   3, 700, 500 },
  { "MAGNETISMO",  "Moedas atraem longe", 5, 350, 250 },
  { "VALOR_MOEDA", "Moedas +25% valor",   5, 500, 350 },
  { "REVIVE",      "+1 revive automatico",2, 1500, 1000 },
  { "BOMBA_INICIO", "Comeca nivel c/ bomba",3, 900, 600 }
};

// =============================================================
// PETS
// =============================================================
enum PetType {
  PET_NONE = -1,
  PET_BLASTER = 0,
  PET_ORB,
  PET_HEALER,
  PET_COLLECTOR,
  PET_SHIELDER,
  PET_COUNT
};

struct PetInfo {
  const char* name;
  const char* desc;
  int  cost;
  uint16_t color;
  uint16_t colorLight;
};

const PetInfo PETS[PET_COUNT] = {
  { "BLASTER",   "Atira junto com voce",       800,  0x07FF, 0xBFFF },
  { "ORB",       "Orbita e machuca inimigos",  1000, 0xF81F, 0xFF9F },
  { "CURANDEIRO","+1 vida a cada 30s",         1200, 0x07E0, 0xAFF0 },
  { "COLETOR",   "Puxa moedas automatico",     900,  0xFFE0, 0xFFF0 },
  { "ESCUDEIRO", "Escudo a cada 20s",          1400, 0x8010, 0xE81F }
};

// =============================================================
// ACHIEVEMENTS
// =============================================================
enum AchvType {
  ACHV_FIRST_KILL = 0,
  ACHV_KILL_100,
  ACHV_KILL_500,
  ACHV_KILL_1000,
  ACHV_BOSS_1,
  ACHV_BOSS_5,
  ACHV_BOSS_10,
  ACHV_COMBO_5,
  ACHV_COMBO_10,
  ACHV_SURVIVE_5MIN,
  ACHV_SURVIVE_15MIN,
  ACHV_SCORE_5000,
  ACHV_SCORE_20000,
  ACHV_NO_HIT_LEVEL,
  ACHV_ALL_WEAPONS,
  ACHV_PET_OWNER,
  ACHV_PET_COLLECTOR,
  ACHV_MAXED_SHIP,
  ACHV_KONAMI,
  ACHV_COUNT
};

struct AchvInfo {
  const char* name;
  const char* desc;
  int  reward;
};

const AchvInfo ACHIEVEMENTS[ACHV_COUNT] = {
  { "Primeira Vitima",   "Mate 1 inimigo",          10 },
  { "Cacador",           "Mate 100 inimigos",       50 },
  { "Exterminador",      "Mate 500 inimigos",      150 },
  { "Aniquilador",       "Mate 1000 inimigos",     400 },
  { "Cacador de Chefes", "Derrote 1 boss",         100 },
  { "Mata-Chefes",       "Derrote 5 bosses",       300 },
  { "Carrasco",          "Derrote 10 bosses",      800 },
  { "Combo 5x",          "Alcance combo 5",         75 },
  { "Combo 10x",         "Alcance combo 10",       200 },
  { "Sobrevivente",      "Sobreviva 5 minutos",    150 },
  { "Veterano",          "Sobreviva 15 minutos",   400 },
  { "Pontuador",         "Alcance 5000 pontos",    100 },
  { "Elite",             "Alcance 20000 pontos",   500 },
  { "Perfeito",          "Passe nivel sem dano",   200 },
  { "Arsenal",           "Use todas as armas",     250 },
  { "Amigo Fiel",        "Compre um pet",          100 },
  { "Zoologico",         "Compre todos os pets",   500 },
  { "Engenheiro",        "Maxeie uma nave",        300 },
  { "Konami!",           "Descubra o codigo secreto", 0 }
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
  bool  crit;
  uint32_t hitMask; // cada projétil atinge cada inimigo no máximo uma vez
  bool hitBoss;     // evita dano repetido no mesmo boss
  uint16_t ageFrames; // evita misseis sem alvo ocupando slot para sempre
  uint16_t color;
  uint16_t colorLight;
  bool active;
};

struct Enemy {
  float x, y;
  float vy;
  float vx;
  int hp;
  int maxHp;
  int type;
  bool active;
  int w, h;
  unsigned long lastShot;
  unsigned long lastSpecial;
  int specialTimer;
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

struct Coin {
  float x, y;
  float vy;
  int value;
  int life;
  bool active;
};

struct Pet {
  bool active;
  PetType type;
  float x, y;
  float angle;
  unsigned long lastShot;
  unsigned long lastSpecial;
};

// =============================================================
// BOSSES
// =============================================================
enum BossType {
  BOSS_SENTINEL = 0,
  BOSS_HYDRA,
  BOSS_PRIME,
  BOSS_DEVOURER,
  BOSS_OMEGA,
  BOSS_PHANTOM,
  BOSS_WARLORD,
  BOSS_CRYSTAL,
  BOSS_TITAN,
  BOSS_VOID
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
  unsigned long spawnTime;
  int phase;
  float angleAccum;
  int   attackMode;
  unsigned long lastPhaseSwap;
  bool  shielded;
  unsigned long shieldUntil;
  int   extraState;
  float targetX, targetY;
};

Boss boss;

struct BossConfig {
  int hp;
  int w, h;
  float speed;
  int baseAttackMs;
  int enragedAttackMs;
  int phase3AttackMs;
};

const BossConfig BOSS_CFG[] = {
  {  40, 70, 50, 1.5f, 1200, 800, 600 },
  {  80, 90, 60, 1.2f, 1000, 650, 500 },
  { 120, 80, 55, 2.0f, 900,  550, 400 },
  { 180, 100, 70, 1.0f, 850,  500, 380 },
  { 220, 90, 65, 1.8f, 800,  450, 350 },
  { 260, 95, 70, 1.6f, 750,  420, 320 },
  { 300, 110, 75, 1.0f, 700,  400, 300 },
  { 340, 90, 70, 1.9f, 650,  380, 280 },
  { 420, 130, 85, 0.7f, 600,  350, 260 },
  { 500, 100, 80, 1.4f, 550,  320, 240 }
};

// =============================================================
// MODOS E TELAS
// =============================================================
enum GameMode { MODE_CAMPAIGN, MODE_ENDLESS, MODE_BOSS_RUSH };

enum GameState {
  MENU,
  MODE_SELECT,
  SKIN_SELECT,
  SHIP_SELECT,
  SHOP,
  PET_SHOP,
  ACHV_SCREEN,
  CTRL_SELECT,
  STATS,
  PLAYING,
  PAUSED,
  GAME_OVER,
  TRANSITION,
  BOSS_INTRO,
  RESET_CONFIRM,
  SETTINGS,
  GAME_STATE_COUNT
};

// =============================================================
// ESTADO
// =============================================================
struct Game {
  GameState state;
  GameMode  mode;
  float playerX, playerY;
  int playerW, playerH;
  int lives;
  int maxLives;
  int score;
  int level;
  int combo;
  unsigned long lastKillTime;
  unsigned long lastFireTime;
  unsigned long fireCooldown;
  unsigned long lastEnemySpawn;
  unsigned long enemySpawnInterval;
  int shieldTimer;
  int maxShieldTimer;
  WeaponType weapon;
  int weaponTimer;
  int nextBossLevel;
  bool bossPending;
  unsigned long bossPrepStartMs;
  Scenario scenario;

  float dashVX, dashVY;
  int dashTimer;
  int dashCooldown;
  bool invulnerable;

  int  shakeAmount;
  int  shakeTimer;

  int  enemiesKilled;
  int  bossesKilled;
  int  coinsEarned;
  int  maxCombo;
  unsigned long startTime;
  unsigned long levelStartTime;
  bool tookDamageThisLevel;

  int  levelAtLastNoLevel;
  unsigned long stateTimer;

  int revivesLeft;
  int bombsLeft;
  int bombCooldown;
  unsigned long lastReviveCheck;
  unsigned long hitInvulnerableUntil;
  bool newHighScore;
  bool wonThisGame;
  unsigned long activePlayMs;
  unsigned long lastGameTickMs;
} game;

Bullet bullets[MAX_BULLETS];
Bullet enemyBullets[MAX_ENEMY_BULLS];
Enemy enemies[MAX_ENEMIES];
Particle particles[MAX_PARTICLES];
Star stars[MAX_STARS];
PowerUp powerups[MAX_POWERUPS];
Coin coins[MAX_COINS];
Pet pets[MAX_PETS];

// Defer structural entity changes until collision iteration ends.
struct PendingEnemySpawn {float x,y;};
PendingEnemySpawn pendingEnemySpawns[MAX_ENEMIES*2];
int pendingEnemySpawnCount = 0;
bool bossKillPending = false;

uint16_t currentBgTint = 0x0000;

// =============================================================
// NVS
// =============================================================
Preferences prefs;
#define NVS_NS        "shooter6"
#define NVS_HISCORE   "hiscore"
#define NVS_GAMES     "games"
#define NVS_TOTAL     "total"
#define NVS_SKIN      "skin"
#define NVS_UNLOCK    "unlock"
#define NVS_CAMPWON   "campwon"
#define NVS_CTRLMODE  "ctrlmode"
#define NVS_INVERTY   "inverty"
#define NVS_COINS     "coins"
#define NVS_SHIP      "ship"
#define NVS_SHIPUNL   "shipunl"
#define NVS_ACHV      "achv"
#define NVS_TOTALKILL "totalkill"
#define NVS_TOTALBOSS "totalboss"
#define NVS_TOTALTIME "totaltime"
#define NVS_WEAPONS   "weapons"
#define NVS_PETS      "pets"

struct SaveData {
  int  highScore;
  int  gamesPlayed;
  int  totalScore;
  int  currentSkin; // legacy compatibility; mirrors the equipped ship
  uint8_t shipSkins[SHIP_COUNT]; // per-ship palettes, same namespace
  int  unlockedSkins;
  bool campaignWon;
  int  controlMode;
  bool invertY;

  int  coins;
  int  currentShip;
  int  unlockedShips;
  int  upgradeLevels[UPG_COUNT];
  int  achievements;
  int  totalKills;
  int  totalBosses;
  unsigned long totalTimeSec;
  uint8_t weaponsUsedMask;
  int  unlockedPets;
};

SaveData save;

int skinForShip(int id) {
  if (id < 0 || id >= SHIP_COUNT) return 0;
  const int index = save.shipSkins[id];
  return (index >= 0 && index < SKIN_COUNT) ? index : 0;
}
void equipShip(int id) {
  if (id < 0 || id >= SHIP_COUNT) return;
  save.currentShip = id;
  save.currentSkin = skinForShip(id);
}
void equipSkin(int id, int skin) {
  if (id < 0 || id >= SHIP_COUNT || skin < 0 || skin >= SKIN_COUNT) return;
  save.shipSkins[id] = (uint8_t)skin;
  if (save.currentShip == id) save.currentSkin = skin;
}




bool saveDirty = false;
unsigned long lastSaveTime = 0;
const unsigned long SAVE_MIN_INTERVAL = 3000;
const unsigned long LEVEL_DURATION_MS = 12000UL;
const unsigned long BOSS_CLEANUP_GRACE_MS = 6000UL;
const int BOSS_INTRO_FRAMES = 120;  // 1.92s a 16ms/quadro
const int BOMB_COOLDOWN_FRAMES = 180; // 2.88s a 16ms/quadro

void markSaveDirty() { saveDirty = true; }

void initSaveDefaults() {
  save.highScore = 0;
  save.gamesPlayed = 0;
  save.totalScore = 0;
  save.currentSkin = 0;
  for (int i = 0; i < 5; ++i) save.shipSkins[i] = 0;
  save.unlockedSkins = 0x0F;
  save.campaignWon = false;
  save.controlMode = CTRL_AUTO;
  save.invertY = true;
  save.coins = 0;
  save.currentShip = SHIP_STANDARD;
  save.unlockedShips = 0x01;
  for (int i = 0; i < UPG_COUNT; i++) save.upgradeLevels[i] = 0;
  save.achievements = 0;
  save.totalKills = 0;
  save.totalBosses = 0;
  save.totalTimeSec = 0;
  save.weaponsUsedMask = 0;
  save.unlockedPets = 0;
}

// Diagnosticos novos: o codigo recebido so ANUNCIAVA snapshots RTC,
// mas nao continha implementacao. Esta integracao acrescenta breadcrumbs
// (RAM RTC, nunca NVS/flash), tempos maximos e estado do heap.
// RTC_NOINIT_ATTR pode nao sobreviver a brownout/perda total de alimentacao.
#if defined(ESP32)
#include <esp_system.h>
#include <esp_attr.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
RTC_NOINIT_ATTR volatile uint32_t diagRtcMagic;
RTC_NOINIT_ATTR volatile uint32_t diagRtcStage;
RTC_NOINIT_ATTR volatile uint32_t diagRtcSub;
RTC_NOINIT_ATTR volatile uint32_t diagRtcFrame;
RTC_NOINIT_ATTR volatile uint32_t diagRtcLevel;
constexpr uint32_t DIAG_MAGIC = 0x53A65EEDu;
uint32_t diagFrameCounter = 0;
uint32_t diagMaxBLEus = 0, diagMaxGameus = 0, diagMaxRenderus = 0;
// Maxima duracao continua de cada checkpoint: BLE, gameplay, boss, etc.
uint32_t diagMaxStageUs[11] = {};
uint32_t diagLastCheckpointUs = 0;
uint32_t diagLastStage = 0;
uint32_t diagSlowFrameCount = 0;
unsigned long diagLastHealth = 0;
const char *diagStageName(uint32_t s) {
  switch(s) {
    case 1: return "BLE"; case 2: return "LOGIC"; case 3: return "PLAYER";
    case 4: return "ENEMIES"; case 5: return "BOSS"; case 6: return "COLLISIONS";
    case 7: return "TIMERS"; case 8: return "RENDER_SPI";
    case 9: return "NVS"; case 10: return "BOSS_INTRO";
    default: return "IDLE";
  }
}
void diagMark(uint32_t stage, uint32_t sub = 0) {
  const uint32_t now = micros();
  if (diagLastCheckpointUs && diagLastStage > 0 && diagLastStage <= 10) {
    const uint32_t elapsed = now - diagLastCheckpointUs;
    if (elapsed > diagMaxStageUs[diagLastStage]) diagMaxStageUs[diagLastStage]=elapsed;
  }
  diagLastStage=stage;
  diagLastCheckpointUs=now;
  diagRtcMagic = DIAG_MAGIC;
  diagRtcStage = stage;
  diagRtcSub = sub;
  diagRtcFrame = diagFrameCounter;
  diagRtcLevel = (uint32_t)game.level;
}
void printBootResetReason() {
  Serial.printf("[RESET_DIAG] reason=%d\n", (int)esp_reset_reason());
  if (diagRtcMagic == DIAG_MAGIC) {
    Serial.printf("[PREVIOUS] stage=%s sub=%lu frame=%lu level=%lu\n",
      diagStageName(diagRtcStage), (unsigned long)diagRtcSub,
      (unsigned long)diagRtcFrame, (unsigned long)diagRtcLevel);
  } else Serial.println("[PREVIOUS] RTC sem dados validos");
  diagMark(0);
}
void diagHealth() {
  if (millis() - diagLastHealth < 15000UL) return;
  diagLastHealth = millis();
  const size_t heapFree=heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  const size_t heapMin=heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
  const size_t heapLargest=heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  const size_t psramFree=heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const size_t psramMin=heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
  const UBaseType_t stackMin=uxTaskGetStackHighWaterMark(nullptr);
  Serial.printf("[HEALTH] level=%d state=%d heap=%u min=%u largest=%u psram=%u psramMin=%u stackHWM=%u maxBLEus=%lu maxGameus=%lu maxRenderus=%lu slow=%lu\n",
      game.level, (int)game.state, (unsigned)heapFree, (unsigned)heapMin,
      (unsigned)heapLargest, (unsigned)psramFree, (unsigned)psramMin,
      (unsigned)stackMin, (unsigned long)diagMaxBLEus,
      (unsigned long)diagMaxGameus, (unsigned long)diagMaxRenderus,
      (unsigned long)diagSlowFrameCount);
  Serial.printf("[SUBSYSTEMS] player=%lu enemies=%lu boss=%lu collisions=%lu timers=%lu nvs=%lu intro=%lu\n",
      (unsigned long)diagMaxStageUs[3], (unsigned long)diagMaxStageUs[4],
      (unsigned long)diagMaxStageUs[5], (unsigned long)diagMaxStageUs[6],
      (unsigned long)diagMaxStageUs[7], (unsigned long)diagMaxStageUs[9],
      (unsigned long)diagMaxStageUs[10]);
}
#else
void diagMark(uint32_t stage, uint32_t sub=0) { (void)stage; (void)sub; }
void printBootResetReason() {}
void diagHealth() {}
#endif

// =============================================================
// FORWARD DECLARATIONS
// =============================================================
bool shipUnlocked(int idx);
bool achvUnlocked(int idx);
void unlockAchv(AchvType type);
void checkAchievements();
void saveAll();
void resetSave();
void onGameOver();
void loadSave();
bool petUnlocked(int idx);
void updatePets();
void drawPets();
void useBomb();

void updateStars();
void initStars();
void resetGame();
void spawnEnemy();
void spawnBoss(BossType type);
void queueEnemySpawn(float x, float y);
void processPendingEnemySpawns();
void requestBossKill();
void processPendingBossKill();
void updateBossIntro();
void drawBossIntro();
void updateBoss();
void drawBoss();
void drawPlayer();
void drawEnemy(Enemy &e);
void render();
void updateEnemies();
void checkBulletHits();
void updateBullets();
void updateEnemyBullets();
void updateParticles();
void updatePowerUps();
void updateCoins();
void updatePlayer();
void updateTimers();
void drawUI();
void fireWeapon();
void tryFire();
void tryDash();
void onBossKilled();
void damagePlayer(float x, float y);
void spawnExplosion(float x, float y, int power);
void spawnParticles(float x, float y, int count, uint16_t color);
void spawnCoin(float x, float y, int baseValue);
void spawnCoinBoss(float x, float y);
void spawnPowerUp(float x, float y);
void shake(int amount);
void flash(uint16_t color, int frames);
void pollDabble();
void spawnBullet(float x, float y, float vx, float vy,
                 int dmg, int pierce,
                 uint16_t color, uint16_t colorLight);

// =============================================================
// INPUT
// =============================================================
struct InputState {
  bool up, down, left, right;
  bool cross, circle, triangle, square;
  bool start, select;
  int axisX, axisY;
  bool hasAnalog;
  bool dpadActive; // no AUTO, D-pad tem prioridade sobre o joystick
};

InputState input = {};  // all buttons and dpadActive start false
unsigned long lastDabblePoll = 0;
const unsigned long DABBLE_POLL_MS = 30;

// =============================================================
// MENU
// =============================================================
int menuSelection = 0;
const int MENU_ITEMS = 6;
const char* menuLabels[MENU_ITEMS] = {
  "START GAME", "NAVES", "LOJA", "PETS", "CONQUISTAS", "CONFIGURACOES"
};

int modeSelection = 0;
const int MODE_ITEMS = 3;
const char* modeLabels[MODE_ITEMS] = { "CAMPANHA", "ENDLESS", "BOSS RUSH" };
const int SETTINGS_ITEMS = 3;
const char* settingsLabels[SETTINGS_ITEMS] = { "CONTROLES", "STATUS", "RESET DATA" };
int settingsSelection = 0;

int skinSelection = 0;
int shipSelection = 0;
int shopSelection = 0;
int ctrlSelection = 0;
int petSelection = 0;

enum ShopTab { SHOP_TAB_SHIPS, SHOP_TAB_UPGRADES, SHOP_TAB_COUNT };
int shopTab = 0;
int shopIndex = 0;
int shopScroll = 0;

// =============================================================
// KONAMI CODE (declarado ANTES da função que o usa)
// =============================================================
enum KonamiKey { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_CROSS, K_CIRCLE };
const KonamiKey KONAMI_SEQ[] = {
  K_UP, K_UP, K_DOWN, K_DOWN, K_LEFT, K_RIGHT, K_LEFT, K_RIGHT, K_CROSS, K_CIRCLE
};
const int KONAMI_LEN = 10;

int konamiProgress = 0;
bool konamiActive = false;
bool bossRushUnlocked() { return save.campaignWon || konamiActive || achvUnlocked(ACHV_KONAMI); }

// =============================================================
// POPUPS (achievement + debug)
// =============================================================
struct Popup {
  bool active;
  int  idx;
  unsigned long startTime;
  int  timer;
  bool isDebug;
};
Popup popup = {false, 0, 0, 0, false};

// =============================================================
// UTIL
// =============================================================
float randf(float lo, float hi) {
  return lo + (float)random(0, 10000) / 10000.0f * (hi - lo);
}

bool shipUnlocked(int idx) {
  return (save.unlockedShips & (1 << idx)) != 0;
}

bool petUnlocked(int idx) {
  return (save.unlockedPets & (1 << idx)) != 0;
}

bool achvUnlocked(int idx) {
  return (save.achievements & (1 << idx)) != 0;
}

// =============================================================
// ACHIEVEMENTS
// =============================================================
void unlockAchv(AchvType type) {
  if (achvUnlocked((int)type)) return;
  save.achievements |= (1 << (int)type);
  if (ACHIEVEMENTS[(int)type].reward > 0) {
    save.coins += ACHIEVEMENTS[(int)type].reward;
  }
  popup.active = true;
  popup.idx = (int)type;
  popup.startTime = millis();
  popup.timer = 240;
  popup.isDebug = false;
  Serial.printf(">>> ACHV: %s (+%d)\n",
                ACHIEVEMENTS[(int)type].name, ACHIEVEMENTS[(int)type].reward);
  markSaveDirty();
}

void checkAchievements() {
  if (save.totalKills >= 1)    unlockAchv(ACHV_FIRST_KILL);
  if (save.totalKills >= 100)  unlockAchv(ACHV_KILL_100);
  if (save.totalKills >= 500)  unlockAchv(ACHV_KILL_500);
  if (save.totalKills >= 1000) unlockAchv(ACHV_KILL_1000);
  if (save.totalBosses >= 1)   unlockAchv(ACHV_BOSS_1);
  if (save.totalBosses >= 5)   unlockAchv(ACHV_BOSS_5);
  if (save.totalBosses >= 10)  unlockAchv(ACHV_BOSS_10);
  if (game.maxCombo >= 5)      unlockAchv(ACHV_COMBO_5);
  if (game.maxCombo >= 10)     unlockAchv(ACHV_COMBO_10);
  if (save.totalTimeSec >= 300)  unlockAchv(ACHV_SURVIVE_5MIN);
  if (save.totalTimeSec >= 900)  unlockAchv(ACHV_SURVIVE_15MIN);
  if (save.highScore >= 5000)  unlockAchv(ACHV_SCORE_5000);
  if (save.highScore >= 20000) unlockAchv(ACHV_SCORE_20000);
  if ((save.weaponsUsedMask & ALL_WEAPONS_MASK) == ALL_WEAPONS_MASK) {
    unlockAchv(ACHV_ALL_WEAPONS);
  }
  if (save.unlockedPets != 0) {
    unlockAchv(ACHV_PET_OWNER);
    int full = (1 << PET_COUNT) - 1;
    if ((save.unlockedPets & full) == full) unlockAchv(ACHV_PET_COLLECTOR);
  }
  bool allShips = (save.unlockedShips == ((1 << SHIP_COUNT) - 1));
  bool allUpg = true;
  for (int i = 0; i < UPG_COUNT; i++) {
    if (save.upgradeLevels[i] < UPGRADES[i].maxLevel) { allUpg = false; break; }
  }
  if (allShips && allUpg) unlockAchv(ACHV_MAXED_SHIP);
}

// =============================================================
// KONAMI CODE — funções
// =============================================================
void resetKonami() {
  konamiProgress = 0;
}

void feedKonami(int k) {
  if (konamiActive) return;
  if (k == KONAMI_SEQ[konamiProgress]) {
    konamiProgress++;
    if (konamiProgress >= KONAMI_LEN) {
      konamiActive = true;
      save.coins = 999999;
      saveAll();
      unlockAchv(ACHV_KONAMI);
      popup.active = true;
      popup.idx = -1;
      popup.startTime = millis();
      popup.timer = 240;
      popup.isDebug = true;
      Serial.println(">>> KONAMI CODE ATIVADO! 999.999 moedas <<<");
      konamiProgress = 0;
    }
  } else if (k == KONAMI_SEQ[0]) {
    konamiProgress = 1;
  } else {
    konamiProgress = 0;
  }
}

// =============================================================
// NVS
// =============================================================
void loadSave() {
  prefs.begin(NVS_NS, true);
  // A v5 usava namespace "shooter". Migramos APENAS quando a v6 está vazia,
  // preservando a cópia antiga para recuperação e evitando reimportar reset.
  bool v6Initialized = prefs.isKey(NVS_HISCORE) || prefs.isKey(NVS_COINS);
  save.highScore     = prefs.getInt(NVS_HISCORE, 0);
  save.gamesPlayed   = prefs.getInt(NVS_GAMES, 0);
  save.totalScore    = prefs.getInt(NVS_TOTAL, 0);
  save.currentSkin   = prefs.getInt(NVS_SKIN, 0);
  save.unlockedSkins = prefs.getInt(NVS_UNLOCK, 0x0F);
  save.campaignWon   = prefs.getBool(NVS_CAMPWON, false);
  save.controlMode   = prefs.getInt(NVS_CTRLMODE, CTRL_AUTO);
  save.invertY       = prefs.getBool(NVS_INVERTY, true);
  save.coins         = prefs.getInt(NVS_COINS, 0);
  save.currentShip   = prefs.getInt(NVS_SHIP, SHIP_STANDARD);
  save.unlockedShips = prefs.getInt(NVS_SHIPUNL, 0x01);
  save.achievements  = prefs.getInt(NVS_ACHV, 0);
  save.totalKills    = prefs.getInt(NVS_TOTALKILL, 0);
  save.totalBosses   = prefs.getInt(NVS_TOTALBOSS, 0);
  save.totalTimeSec  = prefs.getULong(NVS_TOTALTIME, 0);
  save.weaponsUsedMask = prefs.getUChar(NVS_WEAPONS, 0);
  save.unlockedPets  = prefs.getInt(NVS_PETS, 0);
  for (int i = 0; i < SHIP_COUNT; i++) {
    char key[8]; snprintf(key, sizeof(key), "skin%d", i);
    save.shipSkins[i] = (uint8_t)constrain(prefs.getInt(key, save.currentSkin), 0, SKIN_COUNT - 1);
  }

  if (save.currentSkin < 0 || save.currentSkin >= SKIN_COUNT) save.currentSkin = 0;
  if (save.currentShip < 0 || save.currentShip >= SHIP_COUNT) save.currentShip = SHIP_STANDARD;
  if (save.controlMode < CTRL_DIGITAL || save.controlMode > CTRL_AUTO) save.controlMode = CTRL_AUTO;
  save.unlockedSkins = 0x0F;
  save.unlockedShips |= 0x01;
  save.weaponsUsedMask &= ALL_WEAPONS_MASK;
  save.unlockedPets &= ((1 << PET_COUNT) - 1);

  for (int i = 0; i < UPG_COUNT; i++) {
    char key[8];
    sprintf(key, "upg%d", i);
    save.upgradeLevels[i] = prefs.getInt(key, 0);
    if (save.upgradeLevels[i] < 0) save.upgradeLevels[i] = 0;
    if (save.upgradeLevels[i] > UPGRADES[i].maxLevel) save.upgradeLevels[i] = UPGRADES[i].maxLevel;
  }

  prefs.end();
  if (!v6Initialized) {
    prefs.begin("shooter", true);
    bool legacyExists = prefs.isKey(NVS_HISCORE) || prefs.isKey(NVS_COINS);
    if (legacyExists) {
      save.highScore = prefs.getInt(NVS_HISCORE, 0);
      save.gamesPlayed = prefs.getInt(NVS_GAMES, 0);
      save.totalScore = prefs.getInt(NVS_TOTAL, 0);
      save.currentSkin = prefs.getInt(NVS_SKIN, 0);
      save.campaignWon = prefs.getBool("camp50won", false);
      save.controlMode = prefs.getInt(NVS_CTRLMODE, CTRL_AUTO);
      save.invertY = prefs.getBool(NVS_INVERTY, true);
      save.coins = prefs.getInt(NVS_COINS, 0);
      save.currentShip = prefs.getInt(NVS_SHIP, SHIP_STANDARD);
      save.unlockedShips = prefs.getInt(NVS_SHIPUNL, 1) | 1;
      save.achievements = prefs.getInt(NVS_ACHV, 0) & ((1 << 15) - 1);
      save.totalKills = prefs.getInt(NVS_TOTALKILL, 0);
      save.totalBosses = prefs.getInt(NVS_TOTALBOSS, 0);
      save.totalTimeSec = prefs.getULong(NVS_TOTALTIME, 0);
      for (int i = 0; i < 6; i++) {
        char key[8];
        snprintf(key, sizeof(key), "upg%d", i);
        save.upgradeLevels[i] = constrain(prefs.getInt(key, 0), 0, UPGRADES[i].maxLevel);
      }
      save.currentSkin = constrain(save.currentSkin, 0, SKIN_COUNT - 1);
      save.currentShip = constrain(save.currentShip, 0, SHIP_COUNT - 1);
      save.controlMode = constrain(save.controlMode, (int)CTRL_DIGITAL, (int)CTRL_AUTO);
      for (int i = 0; i < SHIP_COUNT; ++i) save.shipSkins[i] = (uint8_t)save.currentSkin;
    }
    prefs.end();
    if (legacyExists) {
      saveAll(); // grava uma cópia no namespace novo, sem apagar a v5
      Serial.println("Migracao NVS v5 -> v6 realizada.");
    }
  }
  equipShip(save.currentShip);
  Serial.printf("NVS: HI=%d Coins=%d Ship=%d Pets=0x%X\n",
                save.highScore, save.coins, save.currentShip, save.unlockedPets);
}

void saveAll() {
  prefs.begin(NVS_NS, false);
  prefs.putInt(NVS_HISCORE, save.highScore);
  prefs.putInt(NVS_GAMES,   save.gamesPlayed);
  prefs.putInt(NVS_TOTAL,   save.totalScore);
  prefs.putInt(NVS_SKIN,    save.currentSkin);
  for (int i = 0; i < SHIP_COUNT; ++i) {
    char key[8]; snprintf(key, sizeof(key), "skin%d", i);
    prefs.putInt(key, save.shipSkins[i]);
  }
  prefs.putInt(NVS_UNLOCK,  save.unlockedSkins);
  prefs.putBool(NVS_CAMPWON, save.campaignWon);
  prefs.putInt(NVS_CTRLMODE, save.controlMode);
  prefs.putBool(NVS_INVERTY, save.invertY);
  prefs.putInt(NVS_COINS,    save.coins);
  prefs.putInt(NVS_SHIP,     save.currentShip);
  prefs.putInt(NVS_SHIPUNL,  save.unlockedShips);
  prefs.putInt(NVS_ACHV,     save.achievements);
  prefs.putInt(NVS_TOTALKILL, save.totalKills);
  prefs.putInt(NVS_TOTALBOSS, save.totalBosses);
  prefs.putULong(NVS_TOTALTIME, save.totalTimeSec);
  prefs.putUChar(NVS_WEAPONS, save.weaponsUsedMask);
  prefs.putInt(NVS_PETS,     save.unlockedPets);

  for (int i = 0; i < UPG_COUNT; i++) {
    char key[8];
    sprintf(key, "upg%d", i);
    prefs.putInt(key, save.upgradeLevels[i]);
  }
  prefs.end();
  saveDirty = false;
  lastSaveTime = millis();
}

void resetSave() {
  initSaveDefaults();
  konamiActive = false;
  konamiProgress = 0;
  saveAll();
  Serial.println("NVS resetado.");
}

void onGameOver() {
  save.gamesPlayed++;
  save.totalScore += game.score;
  game.newHighScore = game.score > save.highScore;
  if (game.newHighScore) save.highScore = game.score;
  save.totalTimeSec += game.activePlayMs / 1000;
  save.coins += game.coinsEarned;
  checkAchievements();
  saveAll();
}

// =============================================================
// INPUT DABBLE
// =============================================================
void pollDabble() {
  if (millis() - lastDabblePoll < DABBLE_POLL_MS) return;
  lastDabblePoll = millis();

  Dabble.processInput();

  int ax = GamePad.getXaxisData();
  int ay = GamePad.getYaxisData();
  if (save.invertY) ay = -ay;

  if (abs(ax) < 1) ax = 0;
  if (abs(ay) < 1) ay = 0;

  input.axisX = ax;
  input.axisY = ay;
  input.hasAnalog = (abs(ax) > 1 || abs(ay) > 1);

  input.up       = GamePad.isUpPressed();
  input.down     = GamePad.isDownPressed();
  input.left     = GamePad.isLeftPressed();
  input.right    = GamePad.isRightPressed();
  input.dpadActive = input.up || input.down || input.left || input.right;
  input.cross    = GamePad.isCrossPressed();
  input.circle   = GamePad.isCirclePressed();
  input.triangle = GamePad.isTrianglePressed();
  input.square   = GamePad.isSquarePressed();
  input.start    = GamePad.isStartPressed();
  input.select   = GamePad.isSelectPressed();

  bool useAnalog = false;
  switch (save.controlMode) {
    case CTRL_DIGITAL:  useAnalog = false; break;
    case CTRL_JOYSTICK: useAnalog = true;  break;
    case CTRL_AUTO:
    default: useAnalog = input.hasAnalog && !input.dpadActive; break;
  }

  if (useAnalog) {
    const int DZ = 2;
    if (ay < -DZ) input.up = true;
    if (ay >  DZ) input.down = true;
    if (ax < -DZ) input.left = true;
    if (ax >  DZ) input.right = true;
  }
}

// =============================================================
// CUSTO DE UPGRADE
// =============================================================
int upgradeCost(int idx) {
  if (idx < 0 || idx >= UPG_COUNT) return -1;
  const UpgradeInfo& u = UPGRADES[idx];
  int lvl = save.upgradeLevels[idx];
  if (lvl >= u.maxLevel) return -1;
  return u.baseCost + u.costStep * lvl;
}

// =============================================================
// ESTRELAS
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
  currentBgTint = sc.bgTint;

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
      c = canvas.color565(stars[i].bright, stars[i].bright, stars[i].bright);
    }
    int sx = (int)stars[i].x, sy = (int)stars[i].y;
    canvas.drawPixel(sx, sy, c);
    if (stars[i].speed > 1.8f) canvas.drawPixel(sx, sy - 1, c);
  }
}

// =============================================================
// RESET GAME
// =============================================================
void resetGame() {
  const ShipInfo& ship = SHIPS[save.currentShip];

  game.playerX = AREA_X + AREA_W / 2.0f;
  game.playerY = AREA_Y + AREA_H - 25;
  game.playerW = 20;
  game.playerH = 22;

  game.maxLives = 3 + save.upgradeLevels[UPG_LIVES] + ship.startLives;
  if (game.maxLives > 9) game.maxLives = 9;
  game.lives = game.maxLives;

  game.score = 0;
  game.level = 1;
  game.combo = 0;
  game.maxCombo = 0;
  game.lastKillTime = 0;
  game.fireCooldown = 180 - save.upgradeLevels[UPG_FIRERATE] * 15;
  if (game.fireCooldown < 80) game.fireCooldown = 80;
  game.lastFireTime = 0;
  game.enemySpawnInterval = 900;
  game.lastEnemySpawn = millis();
  game.shieldTimer = 0;
  game.maxShieldTimer = 800 + save.upgradeLevels[UPG_SHIELD] * 100;
  game.weapon = ship.startWeapon;
  game.weaponTimer = 0;
  game.nextBossLevel = 5;
  game.bossPending = false;
  game.bossPrepStartMs = 0;
  game.scenario = SC_SPACE;

  game.dashVX = 0;
  game.dashVY = 0;
  game.dashTimer = 0;
  game.dashCooldown = 0;
  game.invulnerable = false;

  game.shakeAmount = 0;
  game.shakeTimer = 0;

  game.enemiesKilled = 0;
  game.bossesKilled = 0;
  game.coinsEarned = 0;
  game.startTime = millis();
  game.levelStartTime = millis();
  game.tookDamageThisLevel = false;
  game.levelAtLastNoLevel = 1;
  game.stateTimer = 0;

  game.revivesLeft = save.upgradeLevels[UPG_REVIVE];
  game.bombsLeft = save.upgradeLevels[UPG_START_BOMB];
  game.bombCooldown = 0;
  game.lastReviveCheck = millis();
  game.hitInvulnerableUntil = 0;
  game.newHighScore = false;
  game.wonThisGame = false;
  game.activePlayMs = 0;
  game.lastGameTickMs = millis();

  boss.active = false;
  bossKillPending = false;
  pendingEnemySpawnCount = 0;

  for (int i = 0; i < MAX_PETS; i++) {
    pets[i].active = false;
    pets[i].lastShot = 0;
    pets[i].lastSpecial = 0;
  }

  int petSlot = 0;
  for (int i = 0; i < PET_COUNT && petSlot < MAX_PETS; i++) {
    if (petUnlocked(i)) {
      pets[petSlot].active = true;
      pets[petSlot].type = (PetType)i;
      pets[petSlot].angle = petSlot * (TWO_PI / MAX_PETS);
      pets[petSlot].lastShot = millis();
      pets[petSlot].lastSpecial = millis();
      petSlot++;
    }
  }

  for (int i = 0; i < MAX_BULLETS; i++)      bullets[i].active = false;
  for (int i = 0; i < MAX_ENEMY_BULLS; i++)  enemyBullets[i].active = false;
  for (int i = 0; i < MAX_ENEMIES; i++)      enemies[i].active = false;
  for (int i = 0; i < MAX_PARTICLES; i++)    particles[i].active = false;
  for (int i = 0; i < MAX_POWERUPS; i++)     powerups[i].active = false;
  for (int i = 0; i < MAX_COINS; i++)        coins[i].active = false;

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
      bullets[i].x = x; bullets[i].y = y;
      bullets[i].vx = vx; bullets[i].vy = vy;

      bool isCrit = (random(0, 100) < save.upgradeLevels[UPG_CRIT] * 10);
      int finalDmg = dmg + save.upgradeLevels[UPG_DAMAGE];
      if (isCrit) finalDmg *= 2;

      bullets[i].dmg = finalDmg;
      bullets[i].crit = isCrit;
      bullets[i].pierce = pierce + save.upgradeLevels[UPG_PIERCE];
      bullets[i].guided = false;
      bullets[i].hitMask = 0;
      bullets[i].hitBoss = false;
      bullets[i].ageFrames = 0;
      bullets[i].color = isCrit ? 0xFFFF : color;
      bullets[i].colorLight = isCrit ? 0xFFE0 : colorLight;
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
      int lvl = game.level;

      if (lvl < 2) {
        enemies[i].type = 0;
      } else if (lvl < 4) {
        if      (t < 50) enemies[i].type = 0;
        else if (t < 80) enemies[i].type = 1;
        else if (t < 95) enemies[i].type = 3;
        else             enemies[i].type = 4;
      } else if (lvl < 8) {
        if      (t < 30) enemies[i].type = 0;
        else if (t < 55) enemies[i].type = 1;
        else if (t < 70) enemies[i].type = 2;
        else if (t < 82) enemies[i].type = 3;
        else if (t < 92) enemies[i].type = 4;
        else if (t < 97) enemies[i].type = 5;
        else             enemies[i].type = 6;
      } else {
        if      (t < 20) enemies[i].type = 0;
        else if (t < 35) enemies[i].type = 1;
        else if (t < 50) enemies[i].type = 2;
        else if (t < 60) enemies[i].type = 3;
        else if (t < 72) enemies[i].type = 4;
        else if (t < 82) enemies[i].type = 5;
        else if (t < 90) enemies[i].type = 6;
        else if (t < 96) enemies[i].type = 7;
        else             enemies[i].type = 8;
      }

      switch (enemies[i].type) {
        case 0: enemies[i].hp = 1; enemies[i].w = 20; enemies[i].h = 18; break;
        case 1: enemies[i].hp = 2; enemies[i].w = 22; enemies[i].h = 20; break;
        case 2: enemies[i].hp = 4; enemies[i].w = 28; enemies[i].h = 24; break;
        case 3: enemies[i].hp = 3; enemies[i].w = 24; enemies[i].h = 20; break;
        case 4: enemies[i].hp = 1; enemies[i].w = 22; enemies[i].h = 22; break;
        case 5: enemies[i].hp = 6; enemies[i].w = 26; enemies[i].h = 26; break;
        case 6: enemies[i].hp = 2; enemies[i].w = 20; enemies[i].h = 22; break;
        case 7: enemies[i].hp = 5; enemies[i].w = 30; enemies[i].h = 28; break;
        case 8: enemies[i].hp = 4; enemies[i].w = 24; enemies[i].h = 22; break;
      }

      enemies[i].maxHp = enemies[i].hp;
      enemies[i].x = randf(AREA_X + 20, AREA_X + AREA_W - 20);
      enemies[i].y = AREA_Y - 20;
      enemies[i].vy = 1.2f + game.level * 0.10f + randf(0, 0.5f);
      enemies[i].vx = 0;
      enemies[i].lastShot = millis();
      enemies[i].lastSpecial = millis();
      enemies[i].specialTimer = 0;
      enemies[i].active = true;
      return;
    }
  }
}

void queueEnemySpawn(float x, float y) {
  if (pendingEnemySpawnCount >= MAX_ENEMIES*2) return;
  pendingEnemySpawns[pendingEnemySpawnCount++] = {x,y};
}
void processPendingEnemySpawns() {
  for(int j=0; j<pendingEnemySpawnCount; ++j) {
    for(int n=0;n<MAX_ENEMIES;++n) {
      if (enemies[n].active) continue;
      enemies[n].type=0;enemies[n].hp=1;enemies[n].maxHp=1;
      enemies[n].w=14;enemies[n].h=14;
      enemies[n].x=pendingEnemySpawns[j].x;
      enemies[n].y=pendingEnemySpawns[j].y;
      enemies[n].vy=2.0f;enemies[n].vx=0;
      enemies[n].lastShot=millis();
      enemies[n].lastSpecial=millis();
      enemies[n].specialTimer=0;
      enemies[n].active=true;
      break;
    }
  }
  pendingEnemySpawnCount=0;
}
void requestBossKill() { if(boss.active) bossKillPending=true; }
void processPendingBossKill() {
  if (!bossKillPending) return;
  bossKillPending=false;
  if (boss.active && boss.hp<=0) {
    diagMark(5, 99);  // boss death + possible campaign end/NVS write
    onBossKilled();
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
      powerups[i].x = x;
      powerups[i].y = y;
      powerups[i].vy = 1.5f;
      int r = random(0, 100);
      if (r < 10)      powerups[i].type = 0;
      else if (r < 18) powerups[i].type = 1;
      else             powerups[i].type = 2 + random(0, 5);
      powerups[i].active = true;
      return;
    }
  }
}

void spawnCoin(float x, float y, int baseValue) {
  for (int i = 0; i < MAX_COINS; i++) {
    if (!coins[i].active) {
      coins[i].x = x;
      coins[i].y = y;
      coins[i].vy = 1.5f;
      int val = baseValue + random(0, 3);
      int bonus = (val * save.upgradeLevels[UPG_COIN_VALUE]) / 4;
      coins[i].value = val + bonus;
      coins[i].life = 600;
      coins[i].active = true;
      return;
    }
  }
}

void spawnCoinBoss(float x, float y) {
  for (int k = -1; k <= 1; k++) {
    for (int i = 0; i < MAX_COINS; i++) {
      if (!coins[i].active) {
        coins[i].x = x + k * 25;
        coins[i].y = y;
        coins[i].vy = 1.5f;
        int val = 30 + random(0, 50);
        int bonus = (val * save.upgradeLevels[UPG_COIN_VALUE]) / 4;
        coins[i].value = val + bonus;
        coins[i].life = 900;
        coins[i].active = true;
        break;
      }
    }
  }
}

// =============================================================
// SHAKE / FLASH
// =============================================================
void shake(int amount) {
  game.shakeAmount = amount;
  game.shakeTimer = 12;
}

void flash(uint16_t color, int frames) {
  (void)color;
  (void)frames;
}

// =============================================================
// BOMBA
// =============================================================
void useBomb() {
  if (game.bombsLeft <= 0 || game.bombCooldown > 0) return;
  game.bombsLeft--;
  game.bombCooldown = BOMB_COOLDOWN_FRAMES;

  // A lista de partículas tem limite fixo: 12 efeitos distribuídos são
  // suficientes para preencher a tela, evitando dezenas de varreduras extras.
  for (int i = 0; i < 12; i++) {
    float px = randf(AREA_X, AREA_X + AREA_W);
    float py = randf(AREA_Y, AREA_Y + AREA_H);
    spawnExplosion(px, py, 1);
  }

  shake(15);

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) continue;
    game.score += 10;
    game.enemiesKilled++;
    save.totalKills++;
    spawnExplosion(enemies[i].x, enemies[i].y, 2);
    if (random(0, 100) < 30) spawnCoin(enemies[i].x, enemies[i].y, 1);
    enemies[i].active = false;
  }

  for (int i = 0; i < MAX_ENEMY_BULLS; i++) enemyBullets[i].active = false;

  if (boss.active) {
    boss.hp -= 30;
    spawnExplosion(boss.x, boss.y, 3);
    if (boss.hp <= 0) onBossKilled();
  }

  Serial.printf("BOMBA! Restam: %d\n", game.bombsLeft);
}

// =============================================================
// DASH
// =============================================================
void tryDash() {
  if (game.dashCooldown > 0) return;
  float mvx = 0, mvy = 0;

  bool useAnalog = false;
  switch (save.controlMode) {
    case CTRL_JOYSTICK: useAnalog = true; break;
    case CTRL_AUTO:     useAnalog = input.hasAnalog && !input.dpadActive; break;
    case CTRL_DIGITAL:  useAnalog = false; break;
  }

  if (useAnalog) {
    mvx = input.axisX / 7.0f;
    mvy = input.axisY / 7.0f;
  } else {
    if (input.up) mvy = -1;
    if (input.down) mvy = 1;
    if (input.left) mvx = -1;
    if (input.right) mvx = 1;
  }

  if (mvx == 0 && mvy == 0) mvy = -1;

  float len = sqrtf(mvx*mvx + mvy*mvy);
  if (len > 0.1f) { mvx /= len; mvy /= len; }

  game.dashVX = mvx * 12.0f;
  game.dashVY = mvy * 12.0f;
  game.dashTimer = 10;
  game.invulnerable = true;

  int cdBase = 90 - save.upgradeLevels[UPG_DASH] * 18;
  if (cdBase < 40) cdBase = 40;
  game.dashCooldown = cdBase;

  spawnParticles(game.playerX, game.playerY, 12, 0x07FF);
}

// =============================================================
// DANO
// =============================================================
void damagePlayer(float x, float y) {
  if (game.state != PLAYING) return;
  if (game.invulnerable || (game.hitInvulnerableUntil != 0 &&
      (long)(millis() - game.hitInvulnerableUntil) < 0)) return;
  if (game.shieldTimer > 0) {
    spawnParticles(x, y, 5, C_POWERUP);
    return;
  }

  if (game.revivesLeft > 0 && game.lives <= 1) {
    game.revivesLeft--;
    game.lives = game.maxLives;
    game.invulnerable = true;
    game.dashTimer = 60;
    Serial.println("REVIVE!");
    return;
  }

  game.lives--;
  game.hitInvulnerableUntil = millis() + 900;
  game.tookDamageThisLevel = true;
  spawnExplosion(x, y, 2);
  shake(6);

  if (game.lives <= 0) {
    onGameOver();
    game.state = GAME_OVER;
  }
}

// =============================================================
// BOSSES
// =============================================================
void spawnBoss(BossType type) {
  if ((int)type < 0 || (int)type >= 10) return;
  for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
  for (int i = 0; i < MAX_ENEMY_BULLS; i++) enemyBullets[i].active = false;
  boss.active = true;
  boss.type = type;

  const BossConfig& cfg = BOSS_CFG[(int)type];
  boss.maxHp = cfg.hp;
  boss.hp = cfg.hp;
  boss.w = cfg.w;
  boss.h = cfg.h;
  boss.x = AREA_X + AREA_W / 2.0f;
  boss.y = AREA_Y - boss.h - 30; // entrada animada
  boss.vx = cfg.speed;
  boss.vy = 0;
  boss.phase = 1;
  boss.angleAccum = 0;
  boss.attackMode = 0;
  boss.lastAttack = millis();
  boss.attackInterval = cfg.baseAttackMs;
  boss.spawnTime = millis();
  boss.lastPhaseSwap = millis();
  boss.shielded = false;
  boss.shieldUntil = 0;
  boss.extraState = 0;
  boss.targetX = boss.x;
  boss.targetY = boss.y;

  shake(10);
  Serial.printf("BOSS tipo=%d HP=%d\n", (int)type, boss.maxHp);
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

void updateBossSentinel() {
  boss.x += boss.vx;
  if (boss.x < AREA_X + boss.w/2) { boss.x = AREA_X + boss.w/2; boss.vx = -boss.vx; }
  if (boss.x > AREA_X + AREA_W - boss.w/2) { boss.x = AREA_X + AREA_W - boss.w/2; boss.vx = -boss.vx; }
  if (millis() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = millis();
    int n = (boss.phase == 1) ? 5 : (boss.phase == 2) ? 7 : 9;
    bossShootAtPlayer(2.8f, n, 0.25f);
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.66f) { boss.phase = 2; boss.attackInterval = BOSS_CFG[0].enragedAttackMs; }
  if (boss.phase == 2 && boss.hp < boss.maxHp * 0.33f) { boss.phase = 3; boss.attackInterval = BOSS_CFG[0].phase3AttackMs; }
}

void updateBossHydra() {
  boss.x += boss.vx;
  if (boss.x < AREA_X + boss.w/2) { boss.x = AREA_X + boss.w/2; boss.vx = -boss.vx; }
  if (boss.x > AREA_X + AREA_W - boss.w/2) { boss.x = AREA_X + AREA_W - boss.w/2; boss.vx = -boss.vx; }
  if (millis() - boss.lastAttack > boss.attackInterval / 3) {
    boss.lastAttack = millis();
    for (int h = -1; h <= 1; h++) {
      float hx = boss.x + h * 25;
      float hy = boss.y - 10;
      float dx = game.playerX - hx;
      float dy = game.playerY - hy;
      float a = atan2f(dy, dx) + randf(-0.15f, 0.15f);
      spawnEnemyBullet(hx, hy, cosf(a) * 3.0f, sinf(a) * 3.0f);
    }
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.66f) { boss.phase = 2; boss.attackInterval = BOSS_CFG[1].enragedAttackMs; }
  if (boss.phase == 2 && boss.hp < boss.maxHp * 0.33f) { boss.phase = 3; boss.attackInterval = BOSS_CFG[1].phase3AttackMs; }
}

void updateBossPrime() {
  boss.angleAccum += 0.05f;
  boss.x += sinf(boss.angleAccum) * 2.5f;
  boss.y += cosf(boss.angleAccum * 0.7f) * 0.8f;
  boss.x = constrain(boss.x, AREA_X + boss.w/2, AREA_X + AREA_W - boss.w/2);
  boss.y = constrain(boss.y, AREA_Y + 30, AREA_Y + 90);
  if (millis() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = millis();
    static int primeMode = 0;
    if (millis() - boss.lastPhaseSwap > 4000) { primeMode = (primeMode + 1) % 2; boss.lastPhaseSwap = millis(); }
    if (primeMode == 0) {
      bossShootRadial((boss.phase == 3) ? 10 : (boss.phase == 2) ? 8 : 6, 2.5f, boss.angleAccum * 2.0f);
    } else {
      boss.x = randf(AREA_X + 30, AREA_X + AREA_W - 30);
      boss.y = randf(AREA_Y + 35, AREA_Y + 80);
      bossShootAtPlayer(3.2f, (boss.phase == 3) ? 11 : (boss.phase == 2) ? 9 : 6, 0.3f);
    }
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.66f) { boss.phase = 2; boss.attackInterval = BOSS_CFG[2].enragedAttackMs; }
  if (boss.phase == 2 && boss.hp < boss.maxHp * 0.33f) { boss.phase = 3; boss.attackInterval = BOSS_CFG[2].phase3AttackMs; }
}

void updateBossDevourer() {
  boss.x += boss.vx * 0.6f;
  if (boss.x < AREA_X + boss.w/2) { boss.x = AREA_X + boss.w/2; boss.vx = -boss.vx; }
  if (boss.x > AREA_X + AREA_W - boss.w/2) { boss.x = AREA_X + AREA_W - boss.w/2; boss.vx = -boss.vx; }
  if (!boss.shielded && millis() - boss.lastPhaseSwap > 8000) {
    boss.shielded = true;
    boss.shieldUntil = millis() + 2500;
    boss.lastPhaseSwap = millis();
  }
  if (boss.shielded && millis() > boss.shieldUntil) boss.shielded = false;
  if (boss.shielded) return;
  if (millis() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = millis();
    boss.angleAccum += 0.3f;
    int arms = (boss.phase == 3) ? 6 : 4;
    for (int i = 0; i < arms; i++) {
      float a = boss.angleAccum + i * TWO_PI / arms;
      spawnEnemyBullet(boss.x, boss.y, cosf(a) * 3.5f, sinf(a) * 3.5f);
    }
    if (boss.phase >= 2) bossShootAtPlayer(2.5f, 5, 0.3f);
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.66f) { boss.phase = 2; boss.attackInterval = BOSS_CFG[3].enragedAttackMs; }
  if (boss.phase == 2 && boss.hp < boss.maxHp * 0.33f) { boss.phase = 3; boss.attackInterval = BOSS_CFG[3].phase3AttackMs; }
}

void updateBossOmega() {
  boss.angleAccum += 0.08f;
  boss.x = AREA_X + AREA_W/2 + sinf(boss.angleAccum) * (AREA_W/2 - 50);
  boss.y = AREA_Y + 50 + cosf(boss.angleAccum * 1.3f) * 15;
  if (millis() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = millis();
    boss.attackMode = (boss.attackMode + 1) % 4;
    int count = (boss.phase == 3) ? 2 : 1;
    for (int k = 0; k < count; k++) {
      switch (boss.attackMode) {
        case 0: bossShootRadial(12, 2.5f, millis()/500.0f + k); break;
        case 1: bossShootAtPlayer(3.0f, 9, 0.25f); break;
        case 2: bossShootRadial(16, 1.8f, 0); break;
        case 3: bossShootAtPlayer(4.0f, 3, 0.5f); break;
      }
    }
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.66f) { boss.phase = 2; boss.attackInterval = BOSS_CFG[4].enragedAttackMs; }
  if (boss.phase == 2 && boss.hp < boss.maxHp * 0.33f) { boss.phase = 3; boss.attackInterval = BOSS_CFG[4].phase3AttackMs; }
}

void updateBossPhantom() {
  boss.angleAccum += 0.04f;
  boss.x = AREA_X + AREA_W/2 + sinf(boss.angleAccum) * (AREA_W/2 - 60);
  boss.y = AREA_Y + 55 + cosf(boss.angleAccum * 0.9f) * 20;
  if (millis() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = millis();
    boss.attackMode = (boss.attackMode + 1) % 3;
    if (boss.attackMode == 0) {
      boss.x = randf(AREA_X + 40, AREA_X + AREA_W - 40);
      bossShootAtPlayer(3.5f, 12, 0.2f);
    } else if (boss.attackMode == 1) {
      bossShootRadial(10, 3.0f, boss.angleAccum * 3.0f);
      bossShootRadial(10, 3.0f, boss.angleAccum * 3.0f + PI);
    } else {
      for (int i = -2; i <= 2; i++) {
        spawnEnemyBullet(boss.x + i * 15, boss.y + 20, i * 0.5f, 3.5f);
      }
    }
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.66f) { boss.phase = 2; boss.attackInterval = BOSS_CFG[5].enragedAttackMs; }
  if (boss.phase == 2 && boss.hp < boss.maxHp * 0.33f) { boss.phase = 3; boss.attackInterval = BOSS_CFG[5].phase3AttackMs; }
}

void updateBossWarlord() {
  boss.x += boss.vx * 0.5f;
  if (boss.x < AREA_X + boss.w/2) { boss.x = AREA_X + boss.w/2; boss.vx = -boss.vx; }
  if (boss.x > AREA_X + AREA_W - boss.w/2) { boss.x = AREA_X + AREA_W - boss.w/2; boss.vx = -boss.vx; }
  boss.y = AREA_Y + 55;
  if (millis() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = millis();
    boss.attackMode = (boss.attackMode + 1) % 3;
    if (boss.attackMode == 0) {
      for (int i = 0; i < (boss.phase >= 2 ? 5 : 3); i++) {
        float dx = game.playerX - boss.x + randf(-30, 30);
        float dy = game.playerY - boss.y + randf(-30, 30);
        float d = sqrtf(dx*dx + dy*dy);
        spawnEnemyBullet(boss.x, boss.y, dx/d*2.8f, dy/d*2.8f);
      }
    } else if (boss.attackMode == 1) {
      for (int i = 0; i < 8; i++) {
        spawnEnemyBullet(boss.x - 40 + i * 10, boss.y + 15, 0, 3.5f);
      }
    } else {
      bossShootRadial((boss.phase == 3) ? 14 : 10, 2.2f, millis()/300.0f);
    }
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.66f) { boss.phase = 2; boss.attackInterval = BOSS_CFG[6].enragedAttackMs; }
  if (boss.phase == 2 && boss.hp < boss.maxHp * 0.33f) { boss.phase = 3; boss.attackInterval = BOSS_CFG[6].phase3AttackMs; }
}

void updateBossCrystal() {
  boss.angleAccum += 0.06f;
  boss.x = AREA_X + AREA_W/2 + sinf(boss.angleAccum) * 60;
  boss.y = AREA_Y + 50;
  if (millis() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = millis();
    boss.attackMode = (boss.attackMode + 1) % 4;
    float baseAngle = boss.angleAccum * 2.0f;
    int prismas = (boss.phase == 3) ? 6 : 4;
    for (int i = 0; i < prismas; i++) {
      float a = baseAngle + i * TWO_PI / prismas;
      for (int k = 0; k < 2; k++) {
        float a2 = a + (k == 0 ? -0.15f : 0.15f);
        spawnEnemyBullet(boss.x, boss.y, cosf(a2) * 3.2f, sinf(a2) * 3.2f);
      }
    }
    if (boss.phase >= 2) bossShootAtPlayer(4.5f, 3, 0.1f);
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.66f) { boss.phase = 2; boss.attackInterval = BOSS_CFG[7].enragedAttackMs; }
  if (boss.phase == 2 && boss.hp < boss.maxHp * 0.33f) { boss.phase = 3; boss.attackInterval = BOSS_CFG[7].phase3AttackMs; }
}

void updateBossTitan() {
  boss.x += boss.vx * 0.3f;
  if (boss.x < AREA_X + boss.w/2) { boss.x = AREA_X + boss.w/2; boss.vx = -boss.vx; }
  if (boss.x > AREA_X + AREA_W - boss.w/2) { boss.x = AREA_X + AREA_W - boss.w/2; boss.vx = -boss.vx; }
  if (millis() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = millis();
    boss.attackMode = (boss.attackMode + 1) % 3;
    if (boss.attackMode == 0) {
      for (int i = 0; i < 12; i++) {
        spawnEnemyBullet(AREA_X + 20 + i * 25, boss.y + 20, 0, 2.5f);
      }
    } else if (boss.attackMode == 1) {
      float dx = game.playerX - boss.x;
      float dy = game.playerY - boss.y;
      float a = atan2f(dy, dx);
      for (int i = -1; i <= 1; i++) {
        spawnEnemyBullet(boss.x, boss.y, cosf(a + i*0.4f) * 4.0f, sinf(a + i*0.4f) * 4.0f);
      }
    } else {
      bossShootRadial((boss.phase == 3) ? 20 : 14, 2.0f, 0);
    }
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.66f) { boss.phase = 2; boss.attackInterval = BOSS_CFG[8].enragedAttackMs; }
  if (boss.phase == 2 && boss.hp < boss.maxHp * 0.33f) { boss.phase = 3; boss.attackInterval = BOSS_CFG[8].phase3AttackMs; }
}

void updateBossVoid() {
  boss.angleAccum += 0.1f;
  if (boss.phase == 1) {
    boss.x = AREA_X + AREA_W/2 + sinf(boss.angleAccum) * 80;
    boss.y = AREA_Y + 50;
  } else if (boss.phase == 2) {
    boss.x += boss.vx;
    if (boss.x < AREA_X + boss.w/2) { boss.x = AREA_X + boss.w/2; boss.vx = -boss.vx; }
    if (boss.x > AREA_X + AREA_W - boss.w/2) { boss.x = AREA_X + AREA_W - boss.w/2; boss.vx = -boss.vx; }
  } else {
    boss.x = AREA_X + AREA_W/2 + sinf(boss.angleAccum * 1.7f) * (AREA_W/2 - 60);
    boss.y = AREA_Y + 50 + cosf(boss.angleAccum * 1.1f) * 25;
  }
  if (millis() - boss.lastAttack > boss.attackInterval) {
    boss.lastAttack = millis();
    if (boss.phase == 1) {
      bossShootRadial(10, 2.5f, boss.angleAccum);
    } else if (boss.phase == 2) {
      bossShootRadial(14, 2.8f, -boss.angleAccum);
      bossShootAtPlayer(3.5f, 3, 0.3f);
    } else {
      bossShootRadial(16, 2.5f, boss.angleAccum * 2);
      bossShootAtPlayer(4.0f, 5, 0.25f);
      for (int i = 0; i < 6; i++) {
        spawnEnemyBullet(AREA_X + 30 + i * 50, AREA_Y + 20, 0, 3.5f);
      }
    }
  }
  if (boss.phase == 1 && boss.hp < boss.maxHp * 0.66f) { boss.phase = 2; boss.attackInterval = BOSS_CFG[9].enragedAttackMs; boss.vx = 1.4f; }
  if (boss.phase == 2 && boss.hp < boss.maxHp * 0.33f) { boss.phase = 3; boss.attackInterval = BOSS_CFG[9].phase3AttackMs; }
}

void updateBoss() {
  if (!boss.active) return;
  switch (boss.type) {
    case BOSS_SENTINEL: updateBossSentinel(); break;
    case BOSS_HYDRA:    updateBossHydra();    break;
    case BOSS_PRIME:    updateBossPrime();    break;
    case BOSS_DEVOURER: updateBossDevourer(); break;
    case BOSS_OMEGA:    updateBossOmega();    break;
    case BOSS_PHANTOM:  updateBossPhantom();  break;
    case BOSS_WARLORD:  updateBossWarlord();  break;
    case BOSS_CRYSTAL:  updateBossCrystal();  break;
    case BOSS_TITAN:    updateBossTitan();    break;
    case BOSS_VOID:     updateBossVoid();     break;
  }
  float dx = boss.x - game.playerX;
  float dy = boss.y - game.playerY;
  if (fabsf(dx) < boss.w/2 && fabsf(dy) < boss.h/2 + 10) {
    damagePlayer(game.playerX, game.playerY);
  }
}

void onBossKilled() {
  spawnExplosion(boss.x, boss.y, 5);
  spawnExplosion(boss.x - 25, boss.y, 3);
  spawnExplosion(boss.x + 25, boss.y, 3);
  spawnExplosion(boss.x, boss.y - 15, 3);

  int reward = 500 + (int)boss.type * 250;
  game.score += reward;

  const bool finalVictory = (game.mode == MODE_CAMPAIGN && game.level >= 50 && boss.type == BOSS_VOID)
                         || (game.mode == MODE_BOSS_RUSH && boss.type == BOSS_VOID && game.bossesKilled == 9);
  if (finalVictory) {
    // A vitória bloqueia a coleta: creditar diretamente as moedas do boss final.
    for (int k = 0; k < 3; ++k) {
      int val = 30 + random(0, 50);
      game.coinsEarned += val + (val * save.upgradeLevels[UPG_COIN_VALUE]) / 4;
    }
  } else {
    spawnPowerUp(boss.x, boss.y);
    spawnPowerUp(boss.x - 30, boss.y);
    spawnPowerUp(boss.x + 30, boss.y);
    spawnCoinBoss(boss.x, boss.y);
  }

  boss.active = false;
  game.bossPending = false;
  game.levelStartTime = millis();
  game.bossesKilled++;
  save.totalBosses++;

  shake(15);

  int nextScen = ((int)game.level / 5) % 5;
  game.scenario = (Scenario)nextScen;
  game.nextBossLevel += 5;

  if (finalVictory) {
    game.wonThisGame = true;
    if (game.mode == MODE_CAMPAIGN) save.campaignWon = true;
    onGameOver();
    game.state = GAME_OVER;
  } else if (game.mode == MODE_BOSS_RUSH) {
    // Exactly one boss is active; next is selected from total bosses beaten.
    game.level = (game.bossesKilled + 1) * 5;
    game.bombsLeft = save.upgradeLevels[UPG_START_BOMB];
    game.bombCooldown = 0;
    spawnBoss((BossType)game.bossesKilled);
    game.state = BOSS_INTRO;
    game.stateTimer = 0;
    game.levelStartTime = millis();
  }
}

// Five different mechanically-detailed silhouettes. The palette is stored per ship.
// Simple primitives keep the artwork light on ESP32-S3 (no external bitmaps).
void drawMechShip(int type, int x, int y, int sc, int skin) {
  if (type < 0 || type >= SHIP_COUNT || sc < 1 || sc > 5) return;
  if (skin < 0 || skin >= SKIN_COUNT) skin = 0;
  const ShipPalette& pal = SHIPS[type].palettes[skin];
  uint16_t body = pal.cMain, edge = pal.cLight, shadow = pal.cDark;
  uint16_t core = (type == SHIP_LASER) ? 0x07FF : ((type == SHIP_HEAVY) ? 0xFD20 : 0xBFFF);
  // Coordinates relative to the ship center. Each hull has its own geometry.
  switch (type) {
    case SHIP_STANDARD: { // Interceptor: split angular wings, triple fuselage plating
      canvas.fillTriangle(x-18*sc,y+7*sc,x-7*sc,y-10*sc,x-4*sc,y+12*sc,shadow);
      canvas.fillTriangle(x+18*sc,y+7*sc,x+7*sc,y-10*sc,x+4*sc,y+12*sc,shadow);
      canvas.fillTriangle(x-17*sc,y+5*sc,x-6*sc,y-7*sc,x-7*sc,y+8*sc,body);
      canvas.fillTriangle(x+17*sc,y+5*sc,x+6*sc,y-7*sc,x+7*sc,y+8*sc,body);
      canvas.fillTriangle(x,y-17*sc,x-9*sc,y+10*sc,x+9*sc,y+10*sc,body);
      canvas.fillTriangle(x,y-17*sc,x-3*sc,y+2*sc,x+3*sc,y+2*sc,edge);
      canvas.drawLine(x-14*sc,y+6*sc,x-6*sc,y+3*sc,edge);
      canvas.drawLine(x+14*sc,y+6*sc,x+6*sc,y+3*sc,edge);
      break;
    }
    case SHIP_SCOUT: { // Fast scout: long canards, narrow needle center
      canvas.fillTriangle(x-18*sc,y-6*sc,x-5*sc,y-3*sc,x-7*sc,y+9*sc,body);
      canvas.fillTriangle(x+18*sc,y-6*sc,x+5*sc,y-3*sc,x+7*sc,y+9*sc,body);
      canvas.fillTriangle(x-14*sc,y+11*sc,x-9*sc,y-5*sc,x-8*sc,y+13*sc,shadow);
      canvas.fillTriangle(x+14*sc,y+11*sc,x+9*sc,y-5*sc,x+8*sc,y+13*sc,shadow);
      canvas.fillTriangle(x,y-21*sc,x-5*sc,y+14*sc,x+5*sc,y+14*sc,edge);
      canvas.fillTriangle(x,y-16*sc,x-2*sc,y+12*sc,x+2*sc,y+12*sc,body);
      canvas.drawLine(x-17*sc,y-6*sc,x-10*sc,y-2*sc,edge);
      canvas.drawLine(x+17*sc,y-6*sc,x+10*sc,y-2*sc,edge);
      break;
    }
    case SHIP_HEAVY: { // Heavy mech: bulky hull, two armored weapon pods
      canvas.fillRoundRect(x-11*sc,y-12*sc,22*sc,27*sc,3*sc,shadow);
      canvas.fillRect(x-9*sc,y-10*sc,18*sc,21*sc,body);
      canvas.fillRect(x-21*sc,y-5*sc,9*sc,20*sc,shadow);
      canvas.fillRect(x+12*sc,y-5*sc,9*sc,20*sc,shadow);
      canvas.fillRect(x-19*sc,y-8*sc,5*sc,17*sc,body);
      canvas.fillRect(x+14*sc,y-8*sc,5*sc,17*sc,body);
      canvas.fillRect(x-17*sc,y-14*sc,3*sc,12*sc,edge);
      canvas.fillRect(x+14*sc,y-14*sc,3*sc,12*sc,edge);
      canvas.drawLine(x-8*sc,y+6*sc,x+8*sc,y+6*sc,edge);
      canvas.fillTriangle(x,y-19*sc,x-8*sc,y-6*sc,x+8*sc,y-6*sc,edge);
      break;
    }
    case SHIP_LASER: { // Energy lance: rail guns + power conduits
      canvas.fillTriangle(x,y-24*sc,x-7*sc,y+13*sc,x+7*sc,y+13*sc,shadow);
      canvas.fillTriangle(x,y-22*sc,x-4*sc,y+11*sc,x+4*sc,y+11*sc,body);
      canvas.fillRect(x-15*sc,y-9*sc,5*sc,21*sc,body);
      canvas.fillRect(x+10*sc,y-9*sc,5*sc,21*sc,body);
      canvas.fillRect(x-14*sc,y-19*sc,3*sc,14*sc,edge);
      canvas.fillRect(x+11*sc,y-19*sc,3*sc,14*sc,edge);
      canvas.drawLine(x-11*sc,y+3*sc,x-5*sc,y-5*sc,core);
      canvas.drawLine(x+11*sc,y+3*sc,x+5*sc,y-5*sc,core);
      canvas.fillRect(x-2*sc,y-9*sc,4*sc,17*sc,core);
      break;
    }
    case SHIP_HUNTER: { // Mechapunk hunter: reverse-swept quad wing, missile pods
      canvas.fillTriangle(x-24*sc,y+3*sc,x-4*sc,y-11*sc,x-9*sc,y+16*sc,shadow);
      canvas.fillTriangle(x+24*sc,y+3*sc,x+4*sc,y-11*sc,x+9*sc,y+16*sc,shadow);
      canvas.fillTriangle(x-21*sc,y+2*sc,x-9*sc,y-7*sc,x-13*sc,y+11*sc,body);
      canvas.fillTriangle(x+21*sc,y+2*sc,x+9*sc,y-7*sc,x+13*sc,y+11*sc,body);
      canvas.fillTriangle(x-15*sc,y-11*sc,x-5*sc,y-5*sc,x-7*sc,y+8*sc,edge);
      canvas.fillTriangle(x+15*sc,y-11*sc,x+5*sc,y-5*sc,x+7*sc,y+8*sc,edge);
      canvas.fillTriangle(x,y-18*sc,x-8*sc,y+13*sc,x+8*sc,y+13*sc,body);
      canvas.fillRect(x-18*sc,y+8*sc,4*sc,7*sc,edge);
      canvas.fillRect(x+14*sc,y+8*sc,4*sc,7*sc,edge);
      canvas.drawLine(x-20*sc,y+2*sc,x-10*sc,y+4*sc,edge);
      canvas.drawLine(x+20*sc,y+2*sc,x+10*sc,y+4*sc,edge);
      break;
    }
  }
  // Shared cockpit, reactive reactor core, rivets and dual engine glow.
  canvas.fillCircle(x,y-3*sc,3*sc,0x0010);
  canvas.fillCircle(x,y-3*sc,2*sc,core);
  canvas.drawPixel(x,y-4*sc,0xFFFF);
  canvas.drawPixel(x-5*sc,y+8*sc,edge);
  canvas.drawPixel(x+5*sc,y+8*sc,edge);
  if ((millis()/100)%2 == 0) {
    canvas.fillTriangle(x-6*sc,y+11*sc,x-2*sc,y+11*sc,x-4*sc,y+18*sc,0xFC00);
    canvas.fillTriangle(x+2*sc,y+11*sc,x+6*sc,y+11*sc,x+4*sc,y+18*sc,0xFFE0);
  }
}

// =============================================================
// DESENHO - NAVE
// =============================================================
void drawPlayer() {
  int x = (int)game.playerX;
  int y = (int)game.playerY;

  if (game.invulnerable && (millis() / 60) % 2 == 0) return;

  const int sk = skinForShip(save.currentShip);

  if (game.dashTimer > 0) {
    for (int i = 1; i <= 3; i++) {
      int tx = x - (int)(game.dashVX * i * 0.6f);
      int ty = y - (int)(game.dashVY * i * 0.6f);
      uint16_t tc = (i == 1) ? 0x07FF : (i == 2) ? 0x0410 : 0x0210;
      canvas.drawCircle(tx, ty, 10 - i * 2, tc);
    }
  }

  drawMechShip(save.currentShip, x, y, 1, sk);

  if (game.shieldTimer > 0) {
    uint16_t sCol = (game.shieldTimer % 10 < 5) ? C_POWERUP : 0x07FF;
    canvas.drawCircle(x, y, 16, sCol);
    canvas.drawCircle(x, y, 17, sCol);
    canvas.drawPixel(x - 12, y - 12, 0xFFFF);
    canvas.drawPixel(x + 12, y + 12, 0xFFFF);
  }

  if (game.dashCooldown <= 0) {
    canvas.drawPixel(x + 14, y - 14, 0x07FF);
    canvas.drawPixel(x - 14, y - 14, 0x07FF);
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
  else if (e.type == 4) {
    uint16_t c = ((millis() / 100) % 2 == 0) ? 0xFC00 : 0xF800;
    canvas.fillCircle(x, y, 11, c);
    canvas.fillCircle(x, y, 8, 0xFFE0);
    canvas.fillCircle(x, y, 5, 0xF800);
    canvas.fillCircle(x, y, 3, 0xFFFF);
    canvas.drawPixel(x, y - 12, 0xFC00);
    canvas.drawPixel(x, y - 14, 0x8000);
  }
  else if (e.type == 5) {
    canvas.fillRoundRect(x - 13, y - 13, 26, 26, 4, 0x4208);
    canvas.fillRoundRect(x - 12, y - 12, 24, 24, 4, C_ENEMY5);
    canvas.fillCircle(x, y, 6, 0x001F);
    canvas.fillCircle(x, y, 4, 0xFFFF);
    canvas.drawCircle(x, y, 14, 0xFFFFFF);
    canvas.drawCircle(x, y, 13, 0x07FF);
    canvas.drawCircle(x, y, 12, 0x07FF);
  }
  else if (e.type == 6) {
    canvas.fillTriangle(x, y + 11, x - 9, y - 6, x + 9, y - 6, 0x4000);
    canvas.fillTriangle(x, y + 9, x - 8, y - 4, x + 8, y - 4, C_ENEMY6);
    canvas.fillCircle(x, y - 2, 3, 0x0000);
    canvas.fillCircle(x, y - 2, 2, 0xF800);
    canvas.drawPixel(x, y - 2, 0xFFFF);
    if ((millis() / 300) % 2 == 0) {
      canvas.drawLine(x, y + 8, (int)game.playerX, (int)game.playerY, 0x4000);
    }
  }
  else if (e.type == 7) {
    canvas.fillCircle(x - 6, y, 8, C_ENEMY7);
    canvas.fillCircle(x + 6, y, 8, C_ENEMY7);
    canvas.fillCircle(x - 6, y, 5, 0x07FF);
    canvas.fillCircle(x + 6, y, 5, 0x07FF);
    canvas.drawPixel(x - 6, y, 0x0000);
    canvas.drawPixel(x + 6, y, 0x0000);
    canvas.drawPixel(x, y - 10, 0xF800);
    canvas.drawPixel(x, y - 8, 0xF800);
    canvas.drawPixel(x, y + 8, 0xF800);
    canvas.drawPixel(x, y + 10, 0xF800);
  }
  else if (e.type == 8) {
    canvas.fillCircle(x, y, 12, 0x0320);
    canvas.fillCircle(x, y, 10, 0x07E0);
    canvas.fillRect(x - 2, y - 7, 4, 14, 0xFFFF);
    canvas.fillRect(x - 7, y - 2, 14, 4, 0xFFFF);
    if (e.specialTimer > 0 && e.specialTimer < 20) {
      canvas.drawCircle(x, y, 18 + e.specialTimer / 2, 0x07E0);
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
    canvas.drawCircle(x, y, boss.w/2 + 8, 0x07FF);
    canvas.drawCircle(x, y, boss.w/2 + 9, 0xFFFF);
  }

  switch (boss.type) {
    case BOSS_SENTINEL: {
      uint16_t c = (boss.phase == 3) ? 0xFFFF : (boss.phase == 2) ? 0xF800 : 0xC000;
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
      uint16_t c = (boss.phase == 3) ? 0xFFFF : (boss.phase == 2) ? 0xF800 : 0xFD20;
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
        int arms = (boss.phase == 3) ? 6 : 4;
        for (int i = 0; i < arms; i++) {
          float a = ang + i * TWO_PI / arms;
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
    case BOSS_PHANTOM: {
      uint16_t base = ((millis() / 100) % 2 == 0) ? 0x8010 : 0x4008;
      for (int i = 3; i >= 1; i--) {
        canvas.drawCircle(x, y, 25 * i, base);
      }
      canvas.fillCircle(x, y, 20, 0xC000);
      canvas.fillCircle(x, y, 15, 0xF81F);
      canvas.fillCircle(x - 8, y - 3, 5, 0xFFFF);
      canvas.fillCircle(x + 8, y - 3, 5, 0xFFFF);
      canvas.fillCircle(x - 8, y - 3, 3, 0x0000);
      canvas.fillCircle(x + 8, y - 3, 3, 0x0000);
      for (int i = 0; i < 8; i++) {
        float a = millis() / 150.0f + i * PI / 4;
        int dx = x + (int)(cosf(a) * 30);
        int dy = y + (int)(sinf(a) * 30);
        canvas.drawPixel(dx, dy, 0xF81F);
      }
      break;
    }
    case BOSS_WARLORD: {
      canvas.fillRoundRect(x - 50, y - 30, 100, 55, 8, 0x4000);
      canvas.fillRoundRect(x - 49, y - 29, 98, 53, 8, 0x8410);
      canvas.fillRect(x - 46, y - 26, 92, 5, 0xC618);
      canvas.fillRect(x - 35, y + 15, 8, 18, 0xFFFF);
      canvas.fillRect(x - 4, y + 15, 8, 22, 0xFFFF);
      canvas.fillRect(x + 27, y + 15, 8, 18, 0xFFFF);
      canvas.fillCircle(x - 20, y - 5, 6, 0x0000);
      canvas.fillCircle(x + 20, y - 5, 6, 0x0000);
      canvas.fillCircle(x - 20, y - 5, 4, 0xF800);
      canvas.fillCircle(x + 20, y - 5, 4, 0xF800);
      canvas.fillCircle(x - 20, y - 5, 2, 0xFFFF);
      canvas.fillCircle(x + 20, y - 5, 2, 0xFFFF);
      if ((millis() / 200) % 2 == 0) {
        canvas.fillRect(x - 25, y - 25, 50, 3, 0xF800);
      }
      break;
    }
    case BOSS_CRYSTAL: {
      uint16_t c = (boss.phase == 3) ? 0xFFFF : (boss.phase == 2) ? 0x07FF : 0x0410;
      canvas.fillTriangle(x, y - 30, x - 30, y + 10, x + 30, y + 10, c);
      canvas.fillTriangle(x, y - 30, x - 15, y - 5, x + 15, y - 5, 0xBFFF);
      canvas.fillTriangle(x, y + 10, x - 30, y + 10, x, y + 30, c);
      canvas.fillTriangle(x, y + 10, x + 30, y + 10, x, y + 30, c);
      int r = 5 + (int)(sinf(millis() / 150.0f) * 3);
      canvas.fillCircle(x, y, r, 0xFFFF);
      canvas.drawLine(x, y - 30, x - 30, y + 10, 0xFFFF);
      canvas.drawLine(x, y - 30, x + 30, y + 10, 0xFFFF);
      canvas.drawLine(x - 30, y + 10, x, y + 30, 0xFFFF);
      canvas.drawLine(x + 30, y + 10, x, y + 30, 0xFFFF);
      break;
    }
    case BOSS_TITAN: {
      canvas.fillRoundRect(x - 60, y - 35, 120, 70, 10, 0x4000);
      canvas.fillRoundRect(x - 58, y - 33, 116, 66, 10, 0x2104);
      for (int i = 0; i < 4; i++) {
        canvas.drawRect(x - 55 + i * 30, y - 30, 25, 25, 0x8410);
      }
      uint16_t c = (boss.phase == 3) ? 0xFFE0 : (boss.phase == 2) ? 0xFC00 : 0xF800;
      canvas.fillCircle(x, y, 18, c);
      canvas.fillCircle(x, y, 12, 0xFFFF);
      canvas.fillCircle(x, y, 6, 0x0000);
      canvas.fillRect(x - 60, y + 20, 10, 20, 0x8410);
      canvas.fillRect(x + 50, y + 20, 10, 20, 0x8410);
      break;
    }
    case BOSS_VOID: {
      if (boss.phase == 1) {
        for (int i = 4; i >= 1; i--) {
          canvas.drawCircle(x, y, 10 * i, 0x4008);
          canvas.drawCircle(x, y, 10 * i - 1, 0x8010);
        }
        canvas.fillCircle(x, y, 18, 0xC000);
        canvas.fillCircle(x, y, 12, 0xF81F);
        canvas.fillCircle(x, y, 6, 0xFFFF);
        canvas.fillCircle(x, y, 3, 0x0000);
      } else if (boss.phase == 2) {
        float ang = millis() / 200.0f;
        for (int i = 0; i < 8; i++) {
          float a = ang + i * PI / 4;
          int tx = x + (int)(cosf(a) * 40);
          int ty = y + (int)(sinf(a) * 40);
          canvas.drawLine(x, y, tx, ty, 0xC000);
          canvas.fillCircle(tx, ty, 5, 0xF81F);
        }
        canvas.fillCircle(x, y, 22, 0x8010);
        canvas.fillCircle(x, y, 16, 0xC000);
        canvas.fillCircle(x, y, 8, 0xFFFF);
      } else {
        for (int i = 0; i < 12; i++) {
          float a = millis() / 100.0f + i * PI / 6;
          int tx = x + (int)(cosf(a) * 45);
          int ty = y + (int)(sinf(a) * 45);
          canvas.drawLine(x, y, tx, ty, 0xF800);
          canvas.drawLine(x, y, tx - 5, ty, 0xFC00);
          canvas.fillCircle(tx, ty, 4, 0xFFE0);
        }
        canvas.fillCircle(x, y, 25, 0x8010);
        canvas.fillCircle(x, y, 18, 0xC000);
        canvas.fillCircle(x, y, 12, 0xF800);
        canvas.fillCircle(x, y, 6, 0xFFFF);
      }
      break;
    }
  }
}

// =============================================================
// DESENHO - PETS
// =============================================================
void drawPets() {
  for (int i = 0; i < MAX_PETS; i++) {
    if (!pets[i].active) continue;
    int px = (int)pets[i].x;
    int py = (int)pets[i].y;
    const PetInfo& p = PETS[(int)pets[i].type];

    canvas.fillCircle(px, py, 6, p.color);
    canvas.drawCircle(px, py, 6, 0xFFFF);
    canvas.fillCircle(px, py, 3, p.colorLight);
    canvas.drawPixel(px, py - 2, 0xFFFF);

    if (pets[i].type == PET_ORB) canvas.drawCircle(px, py, 8, p.color);
    else if (pets[i].type == PET_HEALER) {
      canvas.drawPixel(px, py - 4, 0xFFFF);
      canvas.drawPixel(px - 1, py - 4, 0xFFFF);
      canvas.drawPixel(px + 1, py - 4, 0xFFFF);
    }
    else if (pets[i].type == PET_COLLECTOR) {
      canvas.drawPixel(px - 2, py + 2, 0xFFE0);
      canvas.drawPixel(px + 2, py + 2, 0xFFE0);
    }
    else if (pets[i].type == PET_SHIELDER) {
      canvas.drawCircle(px, py, 9, 0x07FF);
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
    canvas.printf("CAMP %d/50", game.level);
  } else if (game.mode == MODE_BOSS_RUSH) {
    canvas.printf("RUSH %d/10", game.bossesKilled + (boss.active ? 1 : 0));
  } else {
    canvas.printf("ENDLESS %d", game.level);
  }

  canvas.setCursor(SCREEN_W - 60, 6);
  canvas.setTextColor(C_COIN, C_BG);
  canvas.printf("$%d", save.coins + game.coinsEarned);

  for (int i = 0; i < game.lives && i < 9; i++) {
    int hx = SCREEN_W - 14 - i * 12;
    canvas.fillTriangle(hx, 16, hx - 4, 24, hx + 4, 24, C_PLAYER);
  }

  canvas.setTextColor(C_UI, C_BG);
  if (game.shieldTimer > 0) {
    canvas.fillRect(6, 18, 50 * game.shieldTimer / game.maxShieldTimer, 3, C_POWERUP);
    canvas.drawRect(5, 17, 52, 5, C_POWERUP);
  }

  if (game.weapon != WPN_DEFAULT) {
    const WeaponInfo& wi = WEAPONS[(int)game.weapon];
    canvas.setTextColor(wi.color, C_BG);
    canvas.setCursor(6, 24);
    canvas.printf("%s", wi.name);
    if (game.weaponTimer > 0) {
      canvas.fillRect(6, 32, 50 * game.weaponTimer / 900, 2, wi.color);
    }
  }

  if (game.dashCooldown > 0) {
    canvas.drawRect(60, 24, 20, 4, C_DIM);
    int pct = 20 - (20 * game.dashCooldown / 90);
    if (pct < 0) pct = 0;
    canvas.fillRect(60, 24, pct, 4, 0x07FF);
  } else {
    canvas.drawRect(60, 24, 20, 4, 0x07FF);
    canvas.fillRect(60, 24, 20, 4, 0x07FF);
  }

  if (game.revivesLeft > 0) {
    canvas.setTextColor(0x07FF, C_BG);
    canvas.setCursor(90, 24);
    canvas.printf("RV%d", game.revivesLeft);
  }

  canvas.setTextColor(game.bombsLeft > 0 ? C_ENEMY9 : C_DIM, C_BG);
  canvas.setCursor(115, 24);
  canvas.printf("B%d", game.bombsLeft);
  const int bx = 135, bw = 32;
  canvas.drawRect(bx, 24, bw, 4, C_DIM);
  const int cooldownPct = game.bombsLeft == 0 ? 0 :
      (game.bombCooldown > 0 ? bw - bw * game.bombCooldown / BOMB_COOLDOWN_FRAMES : bw);
  if (cooldownPct > 0) canvas.fillRect(bx, 24, cooldownPct, 4, C_ENEMY9);

  if (game.combo > 1 && millis() - game.lastKillTime < 1500) {
    int cx = SCREEN_W / 2 - 22;
    int cy = 88;
    canvas.fillRect(cx, cy, 44, 20, currentBgTint);
    canvas.drawRect(cx, cy, 44, 20, C_BULLET);
    canvas.setTextColor(C_BULLET, currentBgTint);
    canvas.setTextSize(2);
    canvas.setCursor(cx + 6, cy + 3);
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
    uint16_t hpCol = (pct > 0.66f) ? 0x07E0 : (pct > 0.33f) ? 0xFFE0 : 0xF800;
    canvas.fillRect(barX, barY, fill, 8, hpCol);
    canvas.drawRect(barX, barY, barW, 8, 0xFFFF);

    canvas.setTextColor(0xFFFF, C_HP_BG);
    canvas.setCursor(barX + 2, barY - 1);
    canvas.printf("BOSS %d/3", boss.phase);

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
  const ShipInfo& ship = SHIPS[save.currentShip];
  float SPEED = 3.0f * ship.speed * (1.0f + save.upgradeLevels[UPG_SPEED] * 0.08f);
  float dx = 0, dy = 0;

  if (game.dashTimer > 0) {
    dx = game.dashVX;
    dy = game.dashVY;
    game.dashTimer--;
    if (game.dashTimer == 0) game.invulnerable = false;
  } else {
    bool useAnalog = false;
    switch (save.controlMode) {
      case CTRL_DIGITAL:  useAnalog = false; break;
      case CTRL_JOYSTICK: useAnalog = true;  break;
      case CTRL_AUTO:
      default: useAnalog = input.hasAnalog && !input.dpadActive; break;
    }

    if (useAnalog) {
      float nx = input.axisX / 7.0f;
      float ny = input.axisY / 7.0f;
      float curveX = (nx >= 0 ? 1 : -1) * powf(fabsf(nx), 1.4f);
      float curveY = (ny >= 0 ? 1 : -1) * powf(fabsf(ny), 1.4f);
      dx = curveX * SPEED * 1.3f;
      dy = curveY * SPEED * 1.3f;
    } else {
      if (input.up)    dy -= SPEED;
      if (input.down)  dy += SPEED;
      if (input.left)  dx -= SPEED;
      if (input.right) dx += SPEED;
    }
  }

  if (game.dashCooldown > 0) game.dashCooldown--;

  game.playerX += dx;
  game.playerY += dy;

  if (game.playerX < AREA_X + 12) game.playerX = AREA_X + 12;
  if (game.playerX > AREA_X + AREA_W - 12) game.playerX = AREA_X + AREA_W - 12;
  if (game.playerY < AREA_Y + 12) game.playerY = AREA_Y + 12;
  if (game.playerY > AREA_Y + AREA_H - 12) game.playerY = AREA_Y + AREA_H - 12;
}

// =============================================================
// PETS
// =============================================================
void updatePets() {
  for (int i = 0; i < MAX_PETS; i++) {
    if (!pets[i].active) continue;

    pets[i].angle += 0.03f;
    float radius = 30.0f;
    pets[i].x = game.playerX + cosf(pets[i].angle) * radius;
    pets[i].y = game.playerY + sinf(pets[i].angle) * radius;

    switch (pets[i].type) {
      case PET_BLASTER: {
        if (millis() - pets[i].lastShot > 500) {
          pets[i].lastShot = millis();
          for (int b = 0; b < MAX_BULLETS; b++) {
            if (!bullets[b].active) {
              bullets[b].x = pets[i].x;
              bullets[b].y = pets[i].y;
              bullets[b].vx = 0;
              bullets[b].vy = -7;
              bullets[b].dmg = 1;
              bullets[b].pierce = 0;
              bullets[b].guided = false;
              bullets[b].crit = false;
              bullets[b].hitMask = 0;
              bullets[b].hitBoss = false;
              bullets[b].ageFrames = 0;
              bullets[b].color = 0x07FF;
              bullets[b].colorLight = 0xBFFF;
              bullets[b].active = true;
              break;
            }
          }
        }
        break;
      }
      case PET_ORB: break;
      case PET_HEALER: {
        if (millis() - pets[i].lastSpecial > 30000) {
          pets[i].lastSpecial = millis();
          if (game.lives < game.maxLives) {
            game.lives++;
            spawnParticles(pets[i].x, pets[i].y, 8, 0x07E0);
          }
        }
        break;
      }
      case PET_COLLECTOR: {
        for (int c = 0; c < MAX_COINS; c++) {
          if (!coins[c].active) continue;
          float dx = pets[i].x - coins[c].x;
          float dy = pets[i].y - coins[c].y;
          float d = sqrtf(dx*dx + dy*dy);
          if (d < 80 && d > 5) {
            coins[c].x += (dx / d) * 2.0f;
            coins[c].y += (dy / d) * 2.0f;
          }
        }
        break;
      }
      case PET_SHIELDER: {
        if (millis() - pets[i].lastSpecial > 20000) {
          pets[i].lastSpecial = millis();
          game.shieldTimer = game.maxShieldTimer / 2;
          spawnParticles(pets[i].x, pets[i].y, 10, 0x8010);
        }
        break;
      }
      default: break;
    }
  }
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
        bullets[idx1].dmg = w.dmg + save.upgradeLevels[UPG_DAMAGE];
        bullets[idx1].pierce = 0;
        bullets[idx1].guided = true;
        bullets[idx1].crit = false;
        bullets[idx1].hitMask = 0;
        bullets[idx1].hitBoss = false;
        bullets[idx1].ageFrames = 0;
        bullets[idx1].color = w.color;
        bullets[idx1].colorLight = w.colorLight;
        bullets[idx1].active = true;
      }
      if (idx2 >= 0) {
        bullets[idx2].x = game.playerX + 8;
        bullets[idx2].y = game.playerY - 10;
        bullets[idx2].vx = 0.5f;
        bullets[idx2].vy = -6;
        bullets[idx2].dmg = w.dmg + save.upgradeLevels[UPG_DAMAGE];
        bullets[idx2].pierce = 0;
        bullets[idx2].guided = true;
        bullets[idx2].crit = false;
        bullets[idx2].hitMask = 0;
        bullets[idx2].hitBoss = false;
        bullets[idx2].ageFrames = 0;
        bullets[idx2].color = w.color;
        bullets[idx2].colorLight = w.colorLight;
        bullets[idx2].active = true;
      }
      break;
    }
  }

  uint8_t oldMask = save.weaponsUsedMask;
  save.weaponsUsedMask |= (uint8_t)(1u << (int)game.weapon);
  if (save.weaponsUsedMask != oldMask) {
    if ((save.weaponsUsedMask & ALL_WEAPONS_MASK) == ALL_WEAPONS_MASK) unlockAchv(ACHV_ALL_WEAPONS);
    markSaveDirty();
  }
}

void tryFire() {
  bool fire = input.cross || input.circle;
  int cd = WEAPONS[(int)game.weapon].cooldown - save.upgradeLevels[UPG_FIRERATE] * 15;
  if (cd < 80) cd = 80;
  if (fire && millis() - game.lastFireTime > (unsigned long)cd) {
    fireWeapon();
    game.lastFireTime = millis();
  }
}

void updateBullets() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) continue;
    if (++bullets[i].ageFrames > 240) { bullets[i].active = false; continue; }

    if (bullets[i].guided) {
      float bestD = 99999;
      int bestIdx = -1;
      for (int e = 0; e < MAX_ENEMIES; e++) {
        if (!enemies[e].active) continue;
        float dx = enemies[e].x - bullets[i].x;
        float dy = enemies[e].y - bullets[i].y;
        float d = dx*dx + dy*dy;
        if (d < bestD) { bestD = d; bestIdx = e; }
      }
      if (boss.active) {
        float dx = boss.x - bullets[i].x;
        float dy = boss.y - bullets[i].y;
        float d = dx*dx + dy*dy;
        if (d < bestD) { bestD = d; bestIdx = 999; }
      }
      if (bestIdx >= 0) {
        float tx, ty;
        if (bestIdx == 999) { tx = boss.x; ty = boss.y; }
        else { tx = enemies[bestIdx].x; ty = enemies[bestIdx].y; }
        float dx = tx - bullets[i].x;
        float dy = ty - bullets[i].y;
        float len = sqrtf(dx*dx + dy*dy);
        if (len > 1) {
          float tVX = (dx / len) * 6.0f;
          float tVY = (dy / len) * 6.0f;
          bullets[i].vx = bullets[i].vx * 0.8f + tVX * 0.2f;
          bullets[i].vy = bullets[i].vy * 0.8f + tVY * 0.2f;
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
      if (!game.invulnerable) {
        damagePlayer(enemyBullets[i].x, enemyBullets[i].y);
      }
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
      if (millis() - enemies[i].lastShot > 1400 && random(0, 100) < 8) {
        float dx = game.playerX - enemies[i].x;
        float dy = game.playerY - enemies[i].y;
        float d = sqrtf(dx*dx + dy*dy);
        if (d > 5) {
          float sp = 3.0f;
          spawnEnemyBullet(enemies[i].x, enemies[i].y + 8, dx/d*sp, dy/d*sp);
        }
        enemies[i].lastShot = millis();
      }
    }

    if (enemies[i].type == 4) {
      float dx = game.playerX - enemies[i].x;
      float dy = game.playerY - enemies[i].y;
      float d = sqrtf(dx*dx + dy*dy);
      if (d > 5) {
        enemies[i].x += (dx / d) * 2.5f;
        enemies[i].y += (dy / d) * 2.5f;
      }
    }

    if (enemies[i].type == 6) {
      if (enemies[i].y > AREA_Y + 50) enemies[i].vy = 0.1f;
      if (millis() - enemies[i].lastShot > 2200) {
        float dx = game.playerX - enemies[i].x;
        float dy = game.playerY - enemies[i].y;
        float d = sqrtf(dx*dx + dy*dy);
        if (d > 5) {
          spawnEnemyBullet(enemies[i].x, enemies[i].y + 10, dx/d*4.5f, dy/d*4.5f);
        }
        enemies[i].lastShot = millis();
      }
    }

    if (enemies[i].type == 8) {
      if (enemies[i].y > AREA_Y + 30) enemies[i].vy = 0.3f;
      if (enemies[i].specialTimer > 0) enemies[i].specialTimer--;

      if (millis() - enemies[i].lastSpecial > 2500) {
        enemies[i].lastSpecial = millis();
        enemies[i].specialTimer = 30;
        int healed = 0;
        for (int k = 0; k < MAX_ENEMIES && healed < 3; k++) {
          if (k == i || !enemies[k].active) continue;
          float dx = enemies[k].x - enemies[i].x;
          float dy = enemies[k].y - enemies[i].y;
          if (dx*dx + dy*dy < 70*70) {
            if (enemies[k].hp < enemies[k].maxHp) {
              enemies[k].hp = min(enemies[k].hp + 1, enemies[k].maxHp);
              spawnParticles(enemies[k].x, enemies[k].y, 3, 0x07E0);
              healed++;
            }
          }
        }
      }
    }

    float dxp = enemies[i].x - game.playerX;
    float dyp = enemies[i].y - game.playerY;
    if (fabsf(dxp) < (enemies[i].w/2 + game.playerW/2 - 4) &&
        fabsf(dyp) < (enemies[i].h/2 + game.playerH/2 - 4)) {
      if (!game.invulnerable) {
        damagePlayer(game.playerX, game.playerY);
      }
      enemies[i].active = false;
      spawnExplosion(enemies[i].x, enemies[i].y, 1);
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
    if (game.state != PLAYING) return;
    if (!bullets[b].active) continue;

    if (boss.active && !bullets[b].hitBoss) {
      if (fabsf(bullets[b].x - boss.x) < boss.w/2 &&
          fabsf(bullets[b].y - boss.y) < boss.h/2) {

        if (boss.type == BOSS_DEVOURER && boss.shielded) {
          spawnParticles(bullets[b].x, bullets[b].y, 3, 0x07FF);
          bullets[b].active = false;
          continue;
        }

        bullets[b].hitBoss = true;
        boss.hp -= bullets[b].dmg;
        spawnParticles(bullets[b].x, bullets[b].y, 3, bullets[b].color);

        if (bullets[b].pierce > 0) bullets[b].pierce--;
        else bullets[b].active = false;

        if (boss.hp <= 0) requestBossKill();
        continue;
      }
    }

    for (int e = 0; e < MAX_ENEMIES; e++) {
      if (!enemies[e].active || (bullets[b].hitMask & (1UL << e))) continue;
      float dx = bullets[b].x - enemies[e].x;
      float dy = bullets[b].y - enemies[e].y;
      if (fabsf(dx) < enemies[e].w/2 && fabsf(dy) < enemies[e].h/2) {
        bullets[b].hitMask |= (1UL << e);

        if (enemies[e].type == 5) {
          if (bullets[b].vy < 0 && bullets[b].y > enemies[e].y) {
            enemies[e].hp -= bullets[b].dmg;
          } else {
            spawnParticles(bullets[b].x, bullets[b].y, 2, 0x07FF);
          }
        } else {
          enemies[e].hp -= bullets[b].dmg;
        }
        spawnParticles(bullets[b].x, bullets[b].y, 3, bullets[b].color);

        if (bullets[b].pierce > 0) bullets[b].pierce--;
        else bullets[b].active = false;

        if (enemies[e].hp <= 0) {
          int pts = (enemies[e].type == 0) ? 10 :
                    (enemies[e].type == 1) ? 25 :
                    (enemies[e].type == 2) ? 60 :
                    (enemies[e].type == 3) ? 80 :
                    (enemies[e].type == 4) ? 40 :
                    (enemies[e].type == 5) ? 100 :
                    (enemies[e].type == 6) ? 70 :
                    (enemies[e].type == 7) ? 90 : 120;

          unsigned long now = millis();
          if (now - game.lastKillTime < 1500) {
            if (game.combo < 15) game.combo++;
            if (game.combo > game.maxCombo) game.maxCombo = game.combo;
          } else {
            game.combo = 1;
          }
          game.lastKillTime = now;

          int mult = (game.combo >= 10) ? 4 : (game.combo >= 5) ? 3 : (game.combo >= 3) ? 2 : 1;
          game.score += pts * mult;
          game.enemiesKilled++;
          save.totalKills++;

          spawnExplosion(enemies[e].x, enemies[e].y,
                         (enemies[e].type == 2 || enemies[e].type == 5) ? 2 : 1);

          int coinChance = 30;
          if (enemies[e].type == 2 || enemies[e].type == 5) coinChance = 60;
          if (random(0, 100) < coinChance) {
            spawnCoin(enemies[e].x, enemies[e].y, 1);
          }
          if (random(0, 100) < 14) spawnPowerUp(enemies[e].x, enemies[e].y);

          if (enemies[e].type == 7) {
            queueEnemySpawn(enemies[e].x-10, enemies[e].y);
            queueEnemySpawn(enemies[e].x+10, enemies[e].y);
          }

          enemies[e].active = false;
        }
        if (!bullets[b].active) break;
      }
    }
  }

  for (int p = 0; p < MAX_PETS; p++) {
    if (!pets[p].active || pets[p].type != PET_ORB) continue;

    for (int e = 0; e < MAX_ENEMIES; e++) {
      if (!enemies[e].active) continue;
      float dx = pets[p].x - enemies[e].x;
      float dy = pets[p].y - enemies[e].y;
      if (fabsf(dx) < enemies[e].w/2 + 6 && fabsf(dy) < enemies[e].h/2 + 6) {
        if (millis() % 100 < 20) {
          enemies[e].hp -= 1;
          spawnParticles(enemies[e].x, enemies[e].y, 2, 0xF81F);

          if (enemies[e].hp <= 0) {
            game.score += 15;
            game.enemiesKilled++;
            save.totalKills++;
            spawnExplosion(enemies[e].x, enemies[e].y, 1);
            if (random(0, 100) < 30) spawnCoin(enemies[e].x, enemies[e].y, 1);
            enemies[e].active = false;
          }
        }
      }
    }

    if (boss.active) {
      float dx = pets[p].x - boss.x;
      float dy = pets[p].y - boss.y;
      if (fabsf(dx) < boss.w/2 + 6 && fabsf(dy) < boss.h/2 + 6) {
        if (millis() % 150 < 20) {
          boss.hp -= 1;
          spawnParticles(pets[p].x, pets[p].y, 3, 0xF81F);
          if (boss.hp <= 0) requestBossKill();
        }
      }
    }
  }  // Resolve all mutations after processing every projectile/pet.
  processPendingBossKill();
  if (game.state == PLAYING) processPendingEnemySpawns();
  else pendingEnemySpawnCount = 0;
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
        game.shieldTimer = game.maxShieldTimer;
      } else if (powerups[i].type == 1) {
        if (game.lives < game.maxLives) game.lives++;
      } else {
        game.weapon = (WeaponType)(powerups[i].type - 1);
        game.weaponTimer = 900;
      }
      spawnParticles(powerups[i].x, powerups[i].y, 10, C_POWERUP);
      powerups[i].active = false;
      continue;
    }
    if (powerups[i].y > AREA_Y + AREA_H + 10) powerups[i].active = false;
  }
}

// =============================================================
// COINS
// =============================================================
void updateCoins() {
  int magnetRange = 60 + save.upgradeLevels[UPG_COIN_MAGNET] * 20;
  for (int i = 0; i < MAX_COINS; i++) {
    if (!coins[i].active) continue;
    coins[i].y += coins[i].vy;
    coins[i].vy *= 0.98f;
    coins[i].life--;

    if (coins[i].life <= 0) {
      coins[i].active = false;
      continue;
    }

    float dx = coins[i].x - game.playerX;
    float dy = coins[i].y - game.playerY;
    float d = sqrtf(dx*dx + dy*dy);
    if (d < magnetRange && d > 5) {
      coins[i].x -= (dx / d) * 2.5f;
      coins[i].y -= (dy / d) * 2.5f;
    }
    if (d < 12) {
      game.coinsEarned += coins[i].value;
      spawnParticles(coins[i].x, coins[i].y, 5, C_COIN);
      coins[i].active = false;
    }

    if (coins[i].y > AREA_Y + AREA_H + 10) coins[i].active = false;
  }
}

// =============================================================
// TIMERS
// =============================================================
void updateTimers() {
  if (game.shieldTimer > 0) game.shieldTimer--;
  if (game.weaponTimer > 0 && --game.weaponTimer == 0)
    game.weapon = SHIPS[save.currentShip].startWeapon;
  if (game.bombCooldown > 0) game.bombCooldown--;

  // Boss Rush has no time-based levels or regular enemy spawns.
  if (game.mode == MODE_BOSS_RUSH) return;
  const unsigned long now = millis();
  const int maxLevel = (game.mode == MODE_CAMPAIGN) ? 50 : 999;
  if (!boss.active && !game.bossPending && game.level < maxLevel &&
      now - game.levelStartTime >= LEVEL_DURATION_MS) {
    if (!game.tookDamageThisLevel && game.level >= 1)
      unlockAchv(ACHV_NO_HIT_LEVEL);
    const int nextLevel = game.level + 1;
    game.tookDamageThisLevel = false;
    if (nextLevel == game.nextBossLevel) {
      // Never spawn more enemies while waiting for the boss.
      game.bossPending = true;
      game.bossPrepStartMs = now;
      for (int i = 0; i < MAX_ENEMY_BULLS; ++i) enemyBullets[i].active = false;
    } else {
      game.level = nextLevel;
      game.levelStartTime = now;
      if (game.enemySpawnInterval > 420) game.enemySpawnInterval -= 12;
      if (game.enemySpawnInterval < 420) game.enemySpawnInterval = 420;
      if (save.upgradeLevels[UPG_START_BOMB] > 0)
        game.bombsLeft = save.upgradeLevels[UPG_START_BOMB];
      game.bombCooldown = 0;
    }
  }

  if (game.bossPending && !boss.active) {
    bool anyEnemy = false;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
      if (enemies[i].active) { anyEnemy = true; break; }
    }
    // Snipers/healers can remain stationary. Never wait forever for cleanup.
    if (!anyEnemy || now - game.bossPrepStartMs >= BOSS_CLEANUP_GRACE_MS) {
      if (anyEnemy) {
        for (int i = 0; i < MAX_ENEMIES; ++i) enemies[i].active = false;
      }
      for (int i = 0; i < MAX_ENEMY_BULLS; ++i) enemyBullets[i].active = false;
      const int idx = (game.nextBossLevel / 5 - 1) % 10;
      game.level = game.nextBossLevel;
      game.bossPending = false;
      game.stateTimer = 0;
      spawnBoss((BossType)idx);
      game.state = BOSS_INTRO;
      game.levelStartTime = now;
      Serial.printf("ENTRADA BOSS %d apos fase %d\n", idx, game.level);
    }
  }
  if (!boss.active && !game.bossPending && game.state == PLAYING &&
      now - game.lastEnemySpawn > game.enemySpawnInterval) {
    spawnEnemy();
    game.lastEnemySpawn = now;
  }
}

// =============================================================
// PIXEL ART DA TRANSIÇÃO
// =============================================================
void drawPixelArtShipFullscreen(int shipType, int frame) {
  canvas.fillSprite(0x0000);
  for (int i = 0; i < 60; i++) {
    int sx = random(0, SCREEN_W);
    int sy = random(0, SCREEN_H);
    uint8_t b = random(80, 255);
    canvas.drawPixel(sx, sy, canvas.color565(b, b, b));
  }

  int cx = SCREEN_W / 2;
  int cy = SCREEN_H / 2 - 10;
  int offset = (frame % 30) - 15;
  cy += offset / 3;

  if (shipType < 0 || shipType >= SHIP_COUNT) shipType = 0;
  const int sk = skinForShip(shipType);

  drawMechShip(shipType, cx, cy, 3, sk);

  for (int i = 0; i < 12; i++) {
    int sx = random(0, SCREEN_W);
    int sy = (random(0, 30) + frame * 15) % SCREEN_H;
    canvas.drawPixel(sx, sy, 0xFFFF);
    canvas.drawPixel(sx, sy + 1, 0xBFFF);
  }
}

// Cutscene v6.5.6: boss enters from above, gameplay input/time frozen.
void updateBossIntro() {
  if (!boss.active) { game.state = PLAYING; game.stateTimer = 0; return; }
  const int t = game.stateTimer > BOSS_INTRO_FRAMES ? BOSS_INTRO_FRAMES : game.stateTimer;
  const float startY = AREA_Y - boss.h - 30.0f;
  const float targetY = AREA_Y + 50.0f;
  const float p = (float)t / (float)BOSS_INTRO_FRAMES;
  const float easing = 1.0f - powf(1.0f - p, 3.0f);
  boss.y = startY + (targetY - startY) * easing;
  if ((t % 4) == 0) {
    spawnParticles(boss.x + randf(-boss.w * .35f, boss.w * .35f),
                   boss.y - boss.h * .35f, 2, 0xFFFF);
  }
  ++game.stateTimer;
  if (game.stateTimer >= BOSS_INTRO_FRAMES) {
    boss.y = targetY;
    boss.lastAttack = millis();
    boss.lastPhaseSwap = millis();
    game.levelStartTime = millis();
    game.lastGameTickMs = millis();
    game.stateTimer = 0;
    game.state = PLAYING;
  }
}

void drawBossIntro() {
  canvas.fillRect(AREA_X, AREA_Y, AREA_W, AREA_H, C_BG);
  for (int i = 0; i < 18; ++i) {
    const int sx = AREA_X + ((i * 37 + game.stateTimer * 3) % AREA_W);
    const int sy = AREA_Y + ((i * 19 + game.stateTimer * 5) % AREA_H);
    canvas.drawLine(sx, sy, sx, sy + 5, C_DIM);
  }
  drawBoss();
  drawPets();
  drawPlayer();
  canvas.setTextColor(C_ENEMY9, C_BG);
  canvas.setTextSize(1);
  canvas.setCursor(105, 65);
  canvas.print("BOSS INCOMING");
  canvas.setCursor(128, 77);
  canvas.printf("FASE %d", game.level);
}

void drawTransition() {
  int t = (int)game.stateTimer;

  if (t < 20) {
    canvas.fillSprite(0x0000);
    drawPixelArtShipFullscreen(save.currentShip, t * 3);
  } else if (t < 70) {
    drawPixelArtShipFullscreen(save.currentShip, t);
    canvas.setTextColor(0xFFFF, 0x0000);
    canvas.setTextSize(1);
    canvas.setCursor(SCREEN_W / 2 - 40, SCREEN_H - 25);
    if (game.level == 1) canvas.print("PREPARE-SE");
    else if (game.level >= game.nextBossLevel - 5) canvas.print("WARNING: BOSS!");
    else canvas.print("NOVA AREA");
  } else if (t < 90) {
    canvas.fillSprite(0x0000);
  } else {
    game.state = PLAYING;
    game.stateTimer = 0;
  }
  game.stateTimer++;
}

// =============================================================
// RENDER
// =============================================================
void render() {
  int shakeX = 0, shakeY = 0;
  if (game.shakeTimer > 0) {
    shakeX = (random(-game.shakeAmount, game.shakeAmount + 1));
    shakeY = (random(-game.shakeAmount, game.shakeAmount + 1));
    game.shakeTimer--;
    if (game.shakeTimer == 0) game.shakeAmount = 0;
  }

  canvas.fillSprite(C_BG);
  drawStars();

  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
    int px = (int)particles[i].x, py = (int)particles[i].y;
    if (px < 0 || px >= SCREEN_W || py < 0 || py >= SCREEN_H) continue;
    int rad = (particles[i].life > 20) ? 2 : 1;
    canvas.fillCircle(px, py, rad, particles[i].color);
  }

  for (int i = 0; i < MAX_COINS; i++) {
    if (!coins[i].active) continue;
    int px = (int)coins[i].x, py = (int)coins[i].y;
    if (coins[i].life < 120 && (millis() / 100) % 2 == 0) continue;

    canvas.fillCircle(px, py, 5, C_COIN);
    canvas.drawCircle(px, py, 5, 0x8000);
    canvas.drawCircle(px, py, 4, C_COIN_LIGHT);
    canvas.drawPixel(px, py - 2, 0xFFFF);
  }

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

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) drawEnemy(enemies[i]);
  }

  drawBoss();

  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) continue;
    int bx = (int)bullets[i].x, by = (int)bullets[i].y;

    if (bullets[i].pierce > 0 && !bullets[i].guided) {
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

    if (bullets[i].crit) {
      canvas.drawPixel(bx - 2, by, 0xFFFF);
      canvas.drawPixel(bx + 2, by, 0xFFFF);
    }
  }

  for (int i = 0; i < MAX_ENEMY_BULLS; i++) {
    if (!enemyBullets[i].active) continue;
    int bx = (int)enemyBullets[i].x, by = (int)enemyBullets[i].y;
    canvas.fillCircle(bx, by, 3, 0xF800);
    canvas.fillCircle(bx, by, 2, 0xFFE0);
    canvas.drawPixel(bx, by, 0xFFFF);
  }

  if (game.state == PLAYING || game.state == PAUSED) {
    drawPets();
    drawPlayer();
  }

  drawUI();
  if (game.state == BOSS_INTRO) drawBossIntro();

  if (popup.active) {
    int t = popup.timer;
    int y = 40;
    if (t > 220) y = 40 - (240 - t) * 2;
    else if (t < 20) y = 40 + (20 - t) * 2;

    uint16_t borderCol = popup.isDebug ? C_DEBUG : C_COIN;
    uint16_t textCol = popup.isDebug ? C_DEBUG : C_COIN;

    canvas.fillRect(20, y, SCREEN_W - 40, 32, 0x0000);
    canvas.drawRect(20, y, SCREEN_W - 40, 32, borderCol);
    canvas.drawRect(21, y + 1, SCREEN_W - 42, 30, borderCol);

    if (popup.isDebug) {
      canvas.setTextColor(textCol, 0x0000);
      canvas.setTextSize(1);
      canvas.setCursor(28, y + 6);
      canvas.print("MODO DEBUG");
      canvas.setTextColor(0xFFFF, 0x0000);
      canvas.setCursor(28, y + 18);
      canvas.print("999.999 moedas adicionadas!");
    } else {
      canvas.setTextColor(textCol, 0x0000);
      canvas.setTextSize(1);
      canvas.setCursor(28, y + 6);
      canvas.print("ACHIEVEMENT!");
      canvas.setTextColor(0xFFFF, 0x0000);
      canvas.setCursor(28, y + 18);
      canvas.print(ACHIEVEMENTS[popup.idx].name);
      canvas.setTextColor(textCol, 0x0000);
      canvas.setCursor(SCREEN_W - 65, y + 18);
      canvas.printf("+%d$", ACHIEVEMENTS[popup.idx].reward);
    }

    popup.timer--;
    if (popup.timer <= 0) popup.active = false;
  }

  // ===== MENU =====
  if (game.state == MENU) {
    for (int i = 0; i < 30; i++) {
      int sx = (i * 11 + millis() / 20) % SCREEN_W;
      int sy = (i * 37) % SCREEN_H;
      canvas.drawPixel(sx, sy, 0x8410);
    }

    int cx = 255, cy = 130;

    drawMechShip(save.currentShip, cx, cy, 2, skinForShip(save.currentShip));

    canvas.setTextSize(3);
    canvas.setTextColor(0x0010, C_BG);
    canvas.setCursor(22, 30); canvas.print("SPACE");
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setCursor(20, 27); canvas.print("SPACE");

    canvas.setTextSize(2);
    canvas.setTextColor(0x0010, C_BG);
    canvas.setCursor(22, 60); canvas.print("SHOOTER");
    canvas.setTextColor(C_BULLET, C_BG);
    canvas.setCursor(20, 57); canvas.print("SHOOTER");

    canvas.setTextSize(1);
    canvas.setTextColor(C_COIN, C_BG);
    canvas.setCursor(20, 85);
    canvas.printf("$ %d", save.coins);

    for (int i = 0; i < MENU_ITEMS; i++) {
      int y = 107 + i * 18;
      bool sel = (i == menuSelection);

      if (sel) {
        if ((millis() / 300) % 2 == 0) {
          canvas.setTextColor(C_BULLET, C_BG);
          canvas.setCursor(18, y); canvas.print(">");
        }
        canvas.fillRect(26, y - 2, 140, 12, C_HIGHLIGHT);
        canvas.drawRect(26, y - 2, 140, 12, C_BULLET);
        canvas.setTextColor(C_BULLET, C_HIGHLIGHT);
      } else {
        canvas.setTextColor(0xBDF7, C_BG);
      }
      canvas.setCursor(34, y); canvas.print(menuLabels[i]);

      if (i == 1) {
        canvas.setCursor(115, y);
        canvas.setTextColor(0x8410, sel ? C_HIGHLIGHT : C_BG);
        canvas.printf("%s", SHIPS[save.currentShip].name);
      }
    }

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setTextSize(1);
    canvas.setCursor(SCREEN_W - 93, SCREEN_H - 12);
    canvas.print(GAME_VERSION);

    canvas.setCursor(60, SCREEN_H - 12); canvas.print("UP/DOWN  CROSS=OK");
  }

  // ===== MODE SELECT =====
  if (game.state == MODE_SELECT) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(70, 50); canvas.print("MODO DE JOGO");

    for (int i = 0; i < MODE_ITEMS; i++) {
      int y = 89 + i * 36;
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
      if (i == 2 && !bossRushUnlocked()) canvas.setTextColor(C_DIM, C_BG);
      canvas.setCursor(85, y); canvas.print(modeLabels[i]);
      if (i == 2 && !bossRushUnlocked()) {
        canvas.setTextSize(1);
        canvas.setCursor(90, y + 18); canvas.print("BLOQUEADO");
      }
    }

    canvas.setTextSize(1);
    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(18, 207);
    canvas.print(bossRushUnlocked() ? "RUSH: 10 BOSSES SEGUIDOS" : "RUSH: vença CAMPANHA ou use DEBUG");
  }

  // ===== SKIN SELECT =====
  if (game.state == SKIN_SELECT) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(30, 15); canvas.print("SKINS POR NAVE");
    canvas.setTextSize(1); canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(40, 38); canvas.print(SHIPS[shipSelection].name);

    const ShipInfo& ship = SHIPS[shipSelection];

    for (int i = 0; i < SKIN_COUNT; i++) {
      int y = 55 + i * 38;
      bool sel = (i == skinSelection);
      const ShipPalette& pal = ship.palettes[i];

      int px = 40, py = y + 12;
      drawMechShip(shipSelection, px, py, 1, i);

      if (sel) {
        if ((millis() / 300) % 2 == 0) {
          canvas.setTextColor(C_BULLET, C_BG);
          canvas.setCursor(58, y);
          canvas.setTextSize(2);
          canvas.print(">");
        }
        canvas.drawRect(64, y - 4, 220, 26, C_BULLET);
      }

      canvas.setTextSize(2);
      canvas.setTextColor(0xFFFF, C_BG);
      canvas.setCursor(75, y);
      canvas.print(shipSkinNames[shipSelection][i]);

      canvas.setTextSize(1);
      if (i == skinForShip(shipSelection)) {
        canvas.setTextColor(0x07E0, C_BG);
        canvas.setCursor(75, y + 18);
        canvas.print("<< ATIVA >>");
      }
    }

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(60, 215); canvas.print("CROSS=OK  SELECT=voltar");
  }

  // ===== SHIP SELECT =====
  if (game.state == SHIP_SELECT) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(65, 8); canvas.print("NAVES");

    int startIdx = shipSelection - 2;
    if (startIdx < 0) startIdx = 0;
    if (startIdx > SHIP_COUNT - 5) startIdx = SHIP_COUNT - 5;
    if (startIdx < 0) startIdx = 0;

    for (int j = 0; j < 5; j++) {
      int i = startIdx + j;
      if (i >= SHIP_COUNT) break;

      int y = 35 + j * 36;
      bool sel = (i == shipSelection);
      bool unl = shipUnlocked(i);
      const ShipInfo& sh = SHIPS[i];
      drawMechShip(i, 41, y + 11, 1, skinForShip(i));

      if (sel) canvas.drawRect(60, y - 3, 138, 32, unl ? C_BULLET : 0x4000);

      canvas.setTextSize(1);
      canvas.setTextColor(unl ? 0xFFFF : 0x4208, C_BG);
      canvas.setCursor(70, y);
      canvas.printf("%s", sh.name);

      canvas.setTextColor(0x8410, C_BG);
      canvas.setCursor(70, y + 12);
      canvas.printf("%s", WEAPONS[(int)sh.startWeapon].name);

      if (i == save.currentShip) {
        canvas.setTextColor(0x07E0, C_BG);
        canvas.setCursor(151, y + 12);
        canvas.print("ATIVA");
      }
      if (!unl) {
        canvas.setTextColor(C_COIN, C_BG);
        canvas.setCursor(151, y + 12);
        canvas.printf("$%d", sh.cost);
      }
    }

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(10, 215); canvas.print("CROSS=NAVE RIGHT=SKINS SELECT=MENU");
    // Preview panel uses the selected ship's saved palette, not the global legacy palette.
    const int sk = skinForShip(shipSelection);
    const ShipPalette& cp = SHIPS[shipSelection].palettes[sk];
    canvas.drawRect(201, 28, 111, 171, cp.cMain);
    drawMechShip(shipSelection, 256, 88, 2, sk);
    canvas.setTextColor(cp.cLight, C_BG);
    canvas.setTextSize(1);
    canvas.setCursor(209, 131); canvas.print("SKIN");
    canvas.setCursor(209, 145); canvas.print(shipSkinNames[shipSelection][sk]);
    canvas.setCursor(209, 166);
    canvas.print(shipUnlocked(shipSelection) ? "RIGHT: MUDAR" : "BLOQUEADA");
  }

  // ===== SHOP =====
  if (game.state == SHOP) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(120, 5); canvas.print("LOJA");
    canvas.setTextSize(1);
    canvas.setTextColor(C_COIN, C_BG);
    canvas.setCursor(SCREEN_W - 60, 10);
    canvas.printf("$ %d", save.coins);

    for (int i = 0; i < SHOP_TAB_COUNT; i++) {
      int x = 20 + i * 140;
      bool sel = (i == shopTab);
      if (sel) {
        canvas.fillRect(x, 22, 130, 14, C_HIGHLIGHT);
        canvas.drawRect(x, 22, 130, 14, C_BULLET);
      }
      canvas.setTextColor(sel ? C_BULLET : 0xDEDB, C_BG);
      canvas.setCursor(x + 8, 26);
      if (i == 0) canvas.print("NAVES");
      else canvas.print("UPGRADES");
    }

    if (shopTab == 0) {
      for (int i = 0; i < SHIP_COUNT; i++) {
        int y = 42 + i * 30;
        bool sel = (i == shopIndex);
        bool unl = shipUnlocked(i);
        const ShipInfo& sh = SHIPS[i];
        drawMechShip(i, 30, y + 12, 1, skinForShip(i));

        if (sel) {
          canvas.fillRect(48, y - 2, SCREEN_W - 55, 26, C_HIGHLIGHT);
          canvas.drawRect(48, y - 2, SCREEN_W - 55, 26, C_BULLET);
        }
        canvas.setTextColor(0xFFFF, sel ? C_HIGHLIGHT : C_BG);
        canvas.setCursor(55, y);
        canvas.print(sh.name);

        canvas.setTextColor(0x8410, sel ? C_HIGHLIGHT : C_BG);
        canvas.setCursor(55, y + 11);
        canvas.printf("%s | Arma: %s", sh.desc, WEAPONS[(int)sh.startWeapon].name);

        canvas.setCursor(SCREEN_W - 65, y + 5);
        if (unl) {
          canvas.setTextColor(0x07E0, sel ? C_HIGHLIGHT : C_BG);
          canvas.print("OK");
        } else {
          canvas.setTextColor(C_COIN, sel ? C_HIGHLIGHT : C_BG);
          canvas.printf("$%d", sh.cost);
        }
      }
    } else {
      int startIdx = shopScroll;
      if (startIdx < 0) startIdx = 0;
      if (startIdx > UPG_COUNT - 9) startIdx = UPG_COUNT - 9;
      if (startIdx < 0) startIdx = 0;

      for (int j = 0; j < 9; j++) {
        int i = startIdx + j;
        if (i >= UPG_COUNT) break;

        int y = 42 + j * 20;
        bool sel = (i == shopIndex);
        const UpgradeInfo& u = UPGRADES[i];
        int lvl = save.upgradeLevels[i];
        bool maxed = (lvl >= u.maxLevel);

        if (sel) {
          canvas.fillRect(8, y - 2, SCREEN_W - 16, 18, C_HIGHLIGHT);
          canvas.drawRect(8, y - 2, SCREEN_W - 16, 18, C_BULLET);
        }

        canvas.setTextColor(0xFFFF, sel ? C_HIGHLIGHT : C_BG);
        canvas.setCursor(14, y);
        canvas.print(u.name);

        canvas.setTextColor(0x8410, sel ? C_HIGHLIGHT : C_BG);
        canvas.setCursor(14, y + 9);
        canvas.print(u.desc);

        canvas.setTextColor(C_BULLET, sel ? C_HIGHLIGHT : C_BG);
        canvas.setCursor(180, y + 4);
        for (int k = 0; k < u.maxLevel && k < 5; k++) {
          uint16_t c = (k < lvl) ? 0x07E0 : 0x4208;
          canvas.fillRect(180 + k * 8, y + 4, 6, 6, c);
        }

        canvas.setCursor(SCREEN_W - 55, y + 4);
        if (maxed) {
          canvas.setTextColor(0x07E0, sel ? C_HIGHLIGHT : C_BG);
          canvas.print("MAX");
        } else {
          int cost = upgradeCost(i);
          canvas.setTextColor(C_COIN, sel ? C_HIGHLIGHT : C_BG);
          canvas.printf("$%d", cost);
        }
      }
    }

    canvas.setTextColor(0x4208, C_BG);
    canvas.setCursor(20, 232);
    canvas.print("? ? ? ? ? ? ? ? ? ?");

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(20, 222); canvas.print("L/R tabs  UP/DOWN  CROSS=OK");
  }

  // ===== PET SHOP =====
  if (game.state == PET_SHOP) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(120, 5); canvas.print("PETS");
    canvas.setTextSize(1);
    canvas.setTextColor(C_COIN, C_BG);
    canvas.setCursor(SCREEN_W - 60, 10);
    canvas.printf("$ %d", save.coins);

    for (int i = 0; i < PET_COUNT; i++) {
      int y = 30 + i * 36;
      bool sel = (i == petSelection);
      bool unl = petUnlocked(i);
      const PetInfo& pi = PETS[i];

      int px = 25, py = y + 15;
      canvas.fillCircle(px, py, 8, pi.color);
      canvas.drawCircle(px, py, 8, 0xFFFF);
      canvas.fillCircle(px, py, 4, pi.colorLight);
      canvas.drawPixel(px, py - 2, 0xFFFF);

      if (sel) {
        canvas.fillRect(45, y - 2, SCREEN_W - 52, 32, C_HIGHLIGHT);
        canvas.drawRect(45, y - 2, SCREEN_W - 52, 32, C_BULLET);
      }

      canvas.setTextColor(0xFFFF, sel ? C_HIGHLIGHT : C_BG);
      canvas.setCursor(52, y + 2);
      canvas.print(pi.name);

      canvas.setTextColor(0x8410, sel ? C_HIGHLIGHT : C_BG);
      canvas.setCursor(52, y + 14);
      canvas.print(pi.desc);

      canvas.setCursor(SCREEN_W - 65, y + 8);
      if (unl) {
        canvas.setTextColor(0x07E0, sel ? C_HIGHLIGHT : C_BG);
        canvas.print("OK");
      } else {
        canvas.setTextColor(C_COIN, sel ? C_HIGHLIGHT : C_BG);
        canvas.printf("$%d", pi.cost);
      }
    }

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(50, 222);
    canvas.print("CROSS=comprar  SELECT=voltar");
  }

  // ===== ACHIEVEMENTS =====
  if (game.state == ACHV_SCREEN) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(60, 5); canvas.print("CONQUISTAS");

    int unlocked = 0;
    for (int i = 0; i < ACHV_COUNT; i++) if (achvUnlocked(i)) unlocked++;
    canvas.setTextSize(1);
    canvas.setTextColor(C_COIN, C_BG);
    canvas.setCursor(SCREEN_W - 70, 12);
    canvas.printf("%d/%d", unlocked, ACHV_COUNT);

    int startIdx = shopScroll;
    if (startIdx < 0) startIdx = 0;
    for (int j = 0; j < 12; j++) {
      int i = startIdx + j;
      if (i >= ACHV_COUNT) break;

      int y = 25 + j * 16;
      bool unl = achvUnlocked(i);

      canvas.setTextColor(unl ? C_COIN : 0x4208, C_BG);
      canvas.setCursor(8, y);
      canvas.print(unl ? "*" : "-");

      canvas.setTextColor(unl ? 0xFFFF : 0x4208, C_BG);
      canvas.setCursor(18, y);
      canvas.print(ACHIEVEMENTS[i].name);

      canvas.setTextColor(unl ? C_COIN : 0x4208, C_BG);
      canvas.setCursor(SCREEN_W - 55, y);
      canvas.printf("+%d$", ACHIEVEMENTS[i].reward);
    }

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(60, 222); canvas.print("UP/DOWN scroll  SELECT=voltar");
  }

  // ===== CTRL SELECT =====
  if (game.state == CTRL_SELECT) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(60, 15); canvas.print("MODO CONTROLE");

    for (int i = 0; i < 4; i++) {
      int y = 50 + i * 32;
      bool sel = (i == ctrlSelection);

      const char* label = "";
      bool isInvert = false;
      if (i == 0) label = "DIGITAL";
      else if (i == 1) label = "JOYSTICK";
      else if (i == 2) label = "AUTO";
      else { label = "INVERT Y"; isInvert = true; }

      if (sel) {
        if ((millis() / 300) % 2 == 0) {
          canvas.setTextColor(C_BULLET, C_BG);
          canvas.setCursor(58, y);
          canvas.setTextSize(2);
          canvas.print(">");
        }
        canvas.fillRect(70, y - 4, 200, 24, C_HIGHLIGHT);
        canvas.drawRect(70, y - 4, 200, 24, C_BULLET);
        canvas.setTextColor(C_BULLET, C_HIGHLIGHT);
      } else {
        canvas.setTextColor(0xBDF7, C_BG);
      }

      canvas.setTextSize(2);
      canvas.setCursor(85, y);
      canvas.print(label);

      canvas.setTextSize(1);
      if (isInvert) {
        if (sel) canvas.setTextColor(C_BULLET, C_HIGHLIGHT);
        else canvas.setTextColor(0x8410, C_BG);
        canvas.setCursor(220, y + 4);
        canvas.print(save.invertY ? "[SIM]" : "[NAO]");
      } else {
        if (i == save.controlMode) {
          canvas.setTextColor(0x07E0, sel ? C_HIGHLIGHT : C_BG);
          canvas.setCursor(220, y + 4);
          canvas.print("<OK>");
        }
      }
    }

    canvas.setTextSize(1);
    canvas.setTextColor(0x8410, C_BG);
    canvas.setCursor(20, 190);
    if (ctrlSelection == 0) canvas.print("Usa o D-pad (modo Gamepad)");
    else if (ctrlSelection == 1) canvas.print("Usa o analogico (Joystick)");
    else if (ctrlSelection == 2) canvas.print("Detecta automaticamente");
    else canvas.print("Inverte eixo Y se estiver ao contrario");

    canvas.setTextColor(0x7BEF, C_BG);
    canvas.setCursor(20, 205);
    canvas.printf("axis=(%d,%d)  hasAnalog=%d", input.axisX, input.axisY, input.hasAnalog);

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(60, 222); canvas.print("CROSS=OK  SELECT=voltar");
  }

  // ===== CONFIGURACOES =====
  if (game.state == SETTINGS) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(2); canvas.setCursor(60, 22); canvas.print("CONFIGURACOES");
    for (int i = 0; i < SETTINGS_ITEMS; i++) {
      int y = 85 + i * 39;
      bool sel = (i == settingsSelection);
      if (sel) {
        canvas.fillRect(35, y - 5, 250, 25, C_HIGHLIGHT);
        canvas.drawRect(35, y - 5, 250, 25, C_BULLET);
      }
      canvas.setTextColor(sel ? C_BULLET : C_UI, sel ? C_HIGHLIGHT : C_BG);
      canvas.setTextSize(2); canvas.setCursor(49, y); canvas.print(settingsLabels[i]);
    }
    canvas.setTextSize(1); canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(48, 219); canvas.print("CROSS=ABRIR  SELECT=MENU");
  }

  // ===== STATS =====
  if (game.state == STATS) {
    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(80, 5); canvas.print("ESTATISTICAS");

    canvas.setTextSize(1);
    canvas.setTextColor(C_UI, C_BG);

    unsigned long totalMin = save.totalTimeSec / 60;
    unsigned long totalHr = totalMin / 60;
    totalMin = totalMin % 60;

    int y = 28;
    const int line = 11;

    canvas.setCursor(10, y); canvas.print("HIGH SCORE:        "); canvas.printf("%d", save.highScore); y += line;
    canvas.setCursor(10, y); canvas.print("PARTIDAS:          "); canvas.printf("%d", save.gamesPlayed); y += line;
    canvas.setCursor(10, y); canvas.print("PONTOS TOTAIS:     "); canvas.printf("%d", save.totalScore); y += line;

    int avg = (save.gamesPlayed > 0) ? (save.totalScore / save.gamesPlayed) : 0;
    canvas.setCursor(10, y); canvas.print("MEDIA POR PARTIDA: "); canvas.printf("%d", avg); y += line;

    canvas.setCursor(10, y); canvas.print("INIMIGOS MORTOS:   "); canvas.printf("%d", save.totalKills); y += line;
    canvas.setCursor(10, y); canvas.print("BOSSES DERROTADOS: "); canvas.printf("%d", save.totalBosses); y += line;

    canvas.setCursor(10, y); canvas.print("TEMPO TOTAL:       ");
    canvas.printf("%luh %lum", totalHr, totalMin); y += line;

    canvas.setCursor(10, y); canvas.print("MOEDAS:            ");
    canvas.setTextColor(C_COIN, C_BG);
    canvas.printf("$%d", save.coins);
    canvas.setTextColor(C_UI, C_BG); y += line;

    int unlockedAchv = 0;
    for (int i = 0; i < ACHV_COUNT; i++) if (achvUnlocked(i)) unlockedAchv++;
    canvas.setCursor(10, y); canvas.print("CONQUISTAS:        "); canvas.printf("%d/%d", unlockedAchv, ACHV_COUNT); y += line;

    canvas.setCursor(10, y); canvas.print("NAVES:             ");
    int shipCount = 0;
    for (int i = 0; i < SHIP_COUNT; i++) if (shipUnlocked(i)) shipCount++;
    canvas.printf("%d/%d", shipCount, SHIP_COUNT); y += line;

    canvas.setCursor(10, y); canvas.print("PETS:              ");
    int petCount = 0;
    for (int i = 0; i < PET_COUNT; i++) if (petUnlocked(i)) petCount++;
    canvas.printf("%d/%d", petCount, PET_COUNT); y += line;

    if (save.campaignWon) {
      canvas.setTextColor(0x07E0, C_BG);
      canvas.setCursor(10, y);
      canvas.print("CAMPANHA VENCIDA!");
    }

    canvas.setTextColor(C_DIM, C_BG);
    canvas.setCursor(70, 222); canvas.print("SELECT/CROSS p/ voltar");
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
    canvas.fillRect(30, 60, SCREEN_W - 60, 130, C_BG);
    canvas.drawRect(30, 60, SCREEN_W - 60, 130, C_ENEMY1);
    canvas.drawRect(32, 62, SCREEN_W - 64, 126, C_ENEMY1);

    bool venceu = game.wonThisGame;

    canvas.setTextColor(venceu ? 0x07E0 : C_ENEMY1, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(venceu ? 75 : 85, 70);
    canvas.print(venceu ? "VITORIA!" : "GAME OVER");

    canvas.setTextColor(C_UI, C_BG);
    canvas.setTextSize(1);
    canvas.setCursor(75, 100);
    canvas.printf("SCORE: %d", game.score);
    canvas.setCursor(75, 113);
    canvas.printf("NIVEL: %d", game.level);
    canvas.setCursor(75, 126);
    canvas.setTextColor(C_COIN, C_BG);
    canvas.printf("MOEDAS: +%d", game.coinsEarned);

    if (game.newHighScore) {
      canvas.setTextColor(C_BULLET, C_BG);
      canvas.setCursor(75, 145);
      canvas.print("*** NOVO RECORDE ***");
    } else {
      canvas.setTextColor(C_DIM, C_BG);
      canvas.setCursor(75, 145);
      canvas.printf("HI: %d", save.highScore);
    }

    canvas.setTextColor(C_UI, C_BG);
    canvas.setCursor(80, 165);
    canvas.print("START p/ menu");
  }

  // ===== RESET CONFIRM =====
  if (game.state == RESET_CONFIRM) {
    canvas.fillRect(24, 60, SCREEN_W - 48, 116, C_BG);
    canvas.drawRect(24, 60, SCREEN_W - 48, 116, C_ENEMY1);
    canvas.setTextSize(2);
    canvas.setTextColor(C_ENEMY1, C_BG);
    canvas.setCursor(52, 73); canvas.print("APAGAR DADOS?");
    canvas.setTextSize(1);
    canvas.setTextColor(C_UI, C_BG);
    canvas.setCursor(37, 113); canvas.print("Recordes, moedas, pets e naves");
    canvas.setCursor(37, 129); canvas.print("serao apagados. Irreversivel!");
    canvas.setCursor(45, 151); canvas.print("CROSS=CONFIRMAR SELECT=SAIR");
  }

  // ===== TRANSICAO =====
  if (game.state == TRANSITION) {
    drawTransition();
  }

  canvas.pushSprite(shakeX, shakeY);
}

// =============================================================
// SETUP
// =============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.printf(" SPACE SHOOTER %s\n", GAME_VERSION);
  Serial.println(" ESP32-S3 + ILI9341 + DABBLE");
  Serial.println("================================");
  printBootResetReason();

  Serial.printf("PSRAM livre: %d bytes\n", ESP.getFreePsram());
  Serial.printf("SRAM livre:  %d bytes\n", ESP.getFreeHeap());
  Serial.printf("CPU freq:    %d MHz\n", getCpuFrequencyMhz());

  initSaveDefaults();
  loadSave();
  checkAchievements();

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
  Serial.println("BLE OK.");
  Serial.println("Pronto!");
}

// =============================================================
// LOOP
// =============================================================
unsigned long lastFrame = 0;
const int FRAME_MS = 16;

unsigned long pauseStartMs = 0;
bool resetConfirmReleased = false;
bool lastStart = false;
bool lastSelect = false;
bool lastTriangle = false;
bool lastSquare = false;
bool lastDebugCombo = false;

bool lastKonamiUp = false, lastKonamiDown = false;
bool lastKonamiLeft = false, lastKonamiRight = false;
bool lastKonamiCross = false, lastKonamiCircle = false;

// New convenience access from v6.5.6; original SHOP Konami stays available.
void activatePauseDebug() {
  if (konamiActive) return;
  konamiActive = true;
  konamiProgress = 0;
  save.coins = 999999;
  markSaveDirty();
  unlockAchv(ACHV_KONAMI);
  popup.active = true;
  popup.idx = -1;
  popup.startTime = millis();
  popup.timer = 240;
  popup.isDebug = true;
  // Only called in PAUSED, never writes to flash during active gameplay.
  saveAll();
  Serial.println("DEBUG: 999999 moedas durante pausa");
}

void feedKonamiInputs() {
  if (game.state != SHOP) {
    if (konamiProgress > 0) konamiProgress = 0;
    return;
  }
  if (konamiActive) return;

  if (input.up && !lastKonamiUp)       feedKonami(K_UP);
  if (input.down && !lastKonamiDown)   feedKonami(K_DOWN);
  if (input.left && !lastKonamiLeft)   feedKonami(K_LEFT);
  if (input.right && !lastKonamiRight) feedKonami(K_RIGHT);
  if (input.cross && !lastKonamiCross) feedKonami(K_CROSS);
  if (input.circle && !lastKonamiCircle) feedKonami(K_CIRCLE);

  lastKonamiUp = input.up;
  lastKonamiDown = input.down;
  lastKonamiLeft = input.left;
  lastKonamiRight = input.right;
  lastKonamiCross = input.cross;
  lastKonamiCircle = input.circle;
}

void loop() {
#if defined(ESP32)
  const uint32_t loopBLEstart = micros();
#endif
  diagMark(1);
  pollDabble();
#if defined(ESP32)
  const uint32_t bleElapsed = micros() - loopBLEstart;
  if (bleElapsed > diagMaxBLEus) diagMaxBLEus = bleElapsed;
#endif

  // Evita gravação da flash em momentos críticos de gameplay/SPI/BLE.
  if (saveDirty && game.state != PLAYING && game.state != TRANSITION &&
      game.state != BOSS_INTRO &&
      millis() - lastSaveTime > SAVE_MIN_INTERVAL) {
    diagMark(9);
    saveAll();
  }

  unsigned long now = millis();
  if (now - lastFrame < FRAME_MS) { yield(); return; }
  lastFrame = now;
#if defined(ESP32)
  const uint32_t logicStart = micros();
  ++diagFrameCounter;
#endif
  diagMark(2);
  feedKonamiInputs();

  // X + SELECT in PAUSED: consume SELECT so it never exits accidentally.
  const bool debugCombo = game.state == PAUSED && input.cross && input.select;
  if (debugCombo && !lastDebugCombo) {
    activatePauseDebug();
    lastSelect = true;
  }
  lastDebugCombo = debugCombo;

  if (input.start && !lastStart) {
    if (game.state == PLAYING) {
      game.state = PAUSED;
      pauseStartMs = millis();
    } else if (game.state == PAUSED) {
      unsigned long pauseDelta = millis() - pauseStartMs;
      game.levelStartTime += pauseDelta;
      game.lastEnemySpawn += pauseDelta;
      game.lastFireTime += pauseDelta;
      game.lastKillTime += pauseDelta;
      if (game.hitInvulnerableUntil) game.hitInvulnerableUntil += pauseDelta;
      if (boss.active) {
        boss.lastAttack += pauseDelta;
        boss.spawnTime += pauseDelta;
        boss.lastPhaseSwap += pauseDelta;
        if (boss.shielded) boss.shieldUntil += pauseDelta;
      }
      for (int i = 0; i < MAX_ENEMIES; i++) if (enemies[i].active) {
        enemies[i].lastShot += pauseDelta;
        enemies[i].lastSpecial += pauseDelta;
      }
      for (int i = 0; i < MAX_PETS; i++) if (pets[i].active) {
        pets[i].lastShot += pauseDelta;
        pets[i].lastSpecial += pauseDelta;
      }
      game.state = PLAYING;
      game.lastGameTickMs = millis();
    }
  }
  lastStart = input.start;

  if (input.select && !lastSelect) {
    if (game.state == SKIN_SELECT) {
      game.state = SHIP_SELECT;
    } else if (game.state == CTRL_SELECT || game.state == STATS || game.state == RESET_CONFIRM) {
      game.state = SETTINGS;
    } else if (game.state == GAME_OVER || game.state == PAUSED ||
               game.state == MODE_SELECT || game.state == SETTINGS ||
               game.state == SHOP || game.state == SHIP_SELECT ||
               game.state == PET_SHOP || game.state == ACHV_SCREEN) {
      game.state = MENU;
      saveAll();
    }
  }
  lastSelect = input.select;

  if (input.triangle && !lastTriangle) {
    if (game.state == PLAYING) tryDash();
  }
  lastTriangle = input.triangle;

  if (input.square && !lastSquare) {
    if (game.state == PLAYING) useBomb();
  }
  lastSquare = input.square;

  // ===== MENU =====
  if (game.state == MENU) {
    static bool lU = false, lD = false, lC = false;
    if (input.up   && !lU) { menuSelection--; if (menuSelection < 0) menuSelection = MENU_ITEMS - 1; }
    if (input.down && !lD) { menuSelection++; if (menuSelection >= MENU_ITEMS) menuSelection = 0; }
    if ((input.cross || input.start) && !lC) {
      switch (menuSelection) {
        case 0: modeSelection = 0; game.state = MODE_SELECT; break;
        case 1: shipSelection = save.currentShip; game.state = SHIP_SELECT; break;
        case 2: shopTab = 0; shopIndex = 0; shopScroll = 0; game.state = SHOP; break;
        case 3: petSelection = 0; game.state = PET_SHOP; break;
        case 4: shopScroll = 0; game.state = ACHV_SCREEN; break;
        case 5: settingsSelection = 0; game.state = SETTINGS; break;
      }
    }
    lU = input.up; lD = input.down; lC = input.cross || input.start;
    updateStars();
  }

  // ===== RESET CONFIRM =====
  else if (game.state == RESET_CONFIRM) {
    // Exige soltar o botao antes do segundo CROSS/START.
    if (!input.cross && !input.start) resetConfirmReleased = true;
    if (resetConfirmReleased && (input.cross || input.start)) {
      resetSave();
      resetConfirmReleased = false;
      game.state = SETTINGS;
    }
  }

  // ===== MODE SELECT =====
  else if (game.state == MODE_SELECT) {
    static bool lU = false, lD = false, lC = false;
    if (input.up   && !lU) { modeSelection--; if (modeSelection < 0) modeSelection = MODE_ITEMS - 1; }
    if (input.down && !lD) { modeSelection++; if (modeSelection >= MODE_ITEMS) modeSelection = 0; }
    if ((input.cross || input.start) && !lC) {
      if (modeSelection != 2 || bossRushUnlocked()) {
        game.mode = (GameMode)modeSelection;
        resetGame();
        game.stateTimer = 0;
        if (game.mode == MODE_BOSS_RUSH) {
          game.level = 5;
          spawnBoss(BOSS_SENTINEL);
          game.state = BOSS_INTRO;
        } else game.state = TRANSITION;
      }
    }
    lU = input.up; lD = input.down; lC = input.cross || input.start;
  }

  // ===== SKIN SELECT =====
  else if (game.state == SKIN_SELECT) {
    static bool lU = false, lD = false, lC = false;
    if (input.up   && !lU) { skinSelection--; if (skinSelection < 0) skinSelection = SKIN_COUNT - 1; }
    if (input.down && !lD) { skinSelection++; if (skinSelection >= SKIN_COUNT) skinSelection = 0; }
    if ((input.cross || input.start) && !lC) {
      if (shipUnlocked(shipSelection)) {
        equipSkin(shipSelection, skinSelection);
        saveAll();
      }
      game.state = SHIP_SELECT;
    }
    lU = input.up; lD = input.down; lC = input.cross || input.start;
  }

  // ===== SHIP SELECT =====
  else if (game.state == SHIP_SELECT) {
    static bool lU = false, lD = false, lC = false, lR = false;
    if (input.up   && !lU) { shipSelection--; if (shipSelection < 0) shipSelection = SHIP_COUNT - 1; }
    if (input.down && !lD) { shipSelection++; if (shipSelection >= SHIP_COUNT) shipSelection = 0; }
    if (input.right && !lR && shipUnlocked(shipSelection)) {
      skinSelection = skinForShip(shipSelection);
      game.state = SKIN_SELECT;
    }
    if ((input.cross || input.start) && !lC && game.state == SHIP_SELECT) {
      if (shipUnlocked(shipSelection)) {
        equipShip(shipSelection);
        saveAll();
        game.state = MENU;
      } else {
        shopTab = 0;
        shopIndex = shipSelection;
        game.state = SHOP;
      }
    }
    lU = input.up; lD = input.down; lC = input.cross || input.start; lR = input.right;
  }

  // ===== SHOP =====
  else if (game.state == SHOP) {
    static bool lU = false, lD = false, lL = false, lR = false, lC = false;
    int maxItems = (shopTab == 0) ? SHIP_COUNT : UPG_COUNT;

    if (input.up   && !lU) {
      shopIndex--;
      if (shopIndex < 0) shopIndex = maxItems - 1;
      if (shopTab == 1 && shopIndex < shopScroll) shopScroll = shopIndex;
    }
    if (input.down && !lD) {
      shopIndex++;
      if (shopIndex >= maxItems) shopIndex = 0;
      if (shopTab == 1 && shopIndex >= shopScroll + 9) shopScroll = shopIndex - 8;
    }
    if (input.left && !lL) {
      shopTab--;
      if (shopTab < 0) shopTab = SHOP_TAB_COUNT - 1;
      shopIndex = 0; shopScroll = 0;
    }
    if (input.right && !lR) {
      shopTab++;
      if (shopTab >= SHOP_TAB_COUNT) shopTab = 0;
      shopIndex = 0; shopScroll = 0;
    }
    if ((input.cross || input.start) && !lC) {
      if (shopTab == 0) {
        if (!shipUnlocked(shopIndex) && save.coins >= SHIPS[shopIndex].cost) {
          save.coins -= SHIPS[shopIndex].cost;
          save.unlockedShips |= (1 << shopIndex);
          equipShip(shopIndex);
          saveAll();
          checkAchievements();
        }
      } else {
        int cost = upgradeCost(shopIndex);
        if (cost > 0 && save.coins >= cost) {
          save.coins -= cost;
          save.upgradeLevels[shopIndex]++;
          saveAll();
          checkAchievements();
        }
      }
    }
    lU = input.up; lD = input.down; lC = input.cross || input.start;
    lL = input.left; lR = input.right;
  }

  // ===== PET SHOP =====
  else if (game.state == PET_SHOP) {
    static bool lU = false, lD = false, lC = false;
    if (input.up   && !lU) { petSelection--; if (petSelection < 0) petSelection = PET_COUNT - 1; }
    if (input.down && !lD) { petSelection++; if (petSelection >= PET_COUNT) petSelection = 0; }
    if ((input.cross || input.start) && !lC) {
      if (!petUnlocked(petSelection) && save.coins >= PETS[petSelection].cost) {
        save.coins -= PETS[petSelection].cost;
        save.unlockedPets |= (1 << petSelection);
        saveAll();
        checkAchievements();
      }
    }
    lU = input.up; lD = input.down; lC = input.cross || input.start;
  }

  // ===== ACHIEVEMENTS =====
  else if (game.state == ACHV_SCREEN) {
    static bool lU = false, lD = false;
    if (input.up   && !lU) { shopScroll--; if (shopScroll < 0) shopScroll = 0; }
    if (input.down && !lD) { shopScroll++; if (shopScroll > ACHV_COUNT - 12) shopScroll = ACHV_COUNT - 12; if (shopScroll < 0) shopScroll = 0; }
    lU = input.up; lD = input.down;
  }

  // ===== SETTINGS =====
  else if (game.state == SETTINGS) {
    static bool lU = false, lD = false, lC = false;
    if (input.up && !lU) settingsSelection = (settingsSelection + SETTINGS_ITEMS - 1) % SETTINGS_ITEMS;
    if (input.down && !lD) settingsSelection = (settingsSelection + 1) % SETTINGS_ITEMS;
    if ((input.cross || input.start) && !lC) {
      if (settingsSelection == 0) { ctrlSelection = 0; game.state = CTRL_SELECT; }
      else if (settingsSelection == 1) game.state = STATS;
      else { resetConfirmReleased = false; game.state = RESET_CONFIRM; }
    }
    lU = input.up; lD = input.down; lC = input.cross || input.start;
  }

  // ===== CTRL =====
  else if (game.state == CTRL_SELECT) {
    static bool lU = false, lD = false, lC = false;
    if (input.up   && !lU) { ctrlSelection--; if (ctrlSelection < 0) ctrlSelection = 3; }
    if (input.down && !lD) { ctrlSelection++; if (ctrlSelection > 3) ctrlSelection = 0; }
    if ((input.cross || input.start) && !lC) {
      if (ctrlSelection == 0)      save.controlMode = CTRL_DIGITAL;
      else if (ctrlSelection == 1) save.controlMode = CTRL_JOYSTICK;
      else if (ctrlSelection == 2) save.controlMode = CTRL_AUTO;
      else if (ctrlSelection == 3) save.invertY = !save.invertY;
      saveAll();
    }
    lU = input.up; lD = input.down; lC = input.cross || input.start;
  }

  // ===== PLAYING =====
  else if (game.state == PLAYING) {
    unsigned long elapsedTick = millis() - game.lastGameTickMs;
    if (elapsedTick <= 100) game.activePlayMs += elapsedTick;
    game.lastGameTickMs = millis();
    updateStars();
    diagMark(3,1); updatePlayer();
    diagMark(3,2); updatePets();
    diagMark(3,3); tryFire();
    diagMark(4,1); updateBullets();
    diagMark(4,2); updateEnemyBullets();
    if (game.state == PLAYING) { diagMark(4,3); updateEnemies(); }
    if (game.state == PLAYING) { diagMark(5); updateBoss(); }
    if (game.state == PLAYING) { diagMark(6); checkBulletHits(); }
    updateParticles();
    if (game.state == PLAYING) {
      diagMark(7,1); updatePowerUps();
      diagMark(7,2); updateCoins();
      diagMark(7,3); updateTimers();
    }
  }

  else if (game.state == PAUSED) { game.lastGameTickMs = millis(); }
  else if (game.state == BOSS_INTRO) {
    diagMark(10);
    updateStars();
    updateParticles();
    updateBossIntro();
  }

  else if (game.state == GAME_OVER) {
    updateStars();
    updateParticles();

    static bool lRestart = false;
    if (input.start && !lRestart) {
      game.state = MENU;
    }
    lRestart = input.start;
  }

  else if (game.state == TRANSITION) { /* feito no render */ }

#if defined(ESP32)
  const uint32_t logicElapsed = micros() - logicStart;
  if (logicElapsed > diagMaxGameus) diagMaxGameus = logicElapsed;
  if (logicElapsed > 16000) ++diagSlowFrameCount;
  const uint32_t renderStart = micros();
#endif
  diagMark(8);
  render();
#if defined(ESP32)
  const uint32_t renderElapsed = micros() - renderStart;
  if (renderElapsed > diagMaxRenderus) diagMaxRenderus = renderElapsed;
#endif
  diagMark(0);
  diagHealth();
  yield();
}