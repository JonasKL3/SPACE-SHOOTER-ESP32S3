/*
   =====================================================
   SPACE SHOOTER - ESP32-S3 + ILI9341 + DABBLE BLE
   =====================================================
   Display : ILI9341 2.8" (320x240, rotacao 2)
   Controle: Dabble GamePad via BLE

   UP/DOWN/LEFT/RIGHT -> mover nave
   CROSS / CIRCLE     -> atirar
   START              -> iniciar / pausar / reiniciar
   SELECT             -> voltar ao menu (game over)
*/

#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <DabbleESP32.h>
#include <math.h>

// ==================================================
// DISPLAY
// ==================================================
TFT_eSPI tft = TFT_eSPI(320, 240);
TFT_eSprite canvas = TFT_eSprite(&tft);

// ==================================================
// TELA
// ==================================================
const int SCREEN_W = 320;
const int SCREEN_H = 240;

// Área de jogo (deixa HUD em cima e embaixo livre)
const int AREA_X = 5;
const int AREA_Y = 30;
const int AREA_W = 310;
const int AREA_H = 205;

// ==================================================
// LIMITES DE ENTIDADES
// ==================================================
#define MAX_BULLETS    16
#define MAX_ENEMIES    20
#define MAX_PARTICLES  40
#define MAX_STARS      28
#define MAX_POWERUPS    4

// ==================================================
// CORES
// ==================================================
#define C_BG        0x0000
#define C_STAR      0xCE79
#define C_PLAYER    0x07E0
#define C_PLAYER2   0x03E0
#define C_BULLET    0xFFE0
#define C_ENEMY1    0xF800
#define C_ENEMY2    0xF81F
#define C_ENEMY3    0xFD20
#define C_EXPLOSION 0xFC00
#define C_POWERUP   0x07FF
#define C_UI        0xFFFF
#define C_AREA_BORDER 0x4208

// ==================================================
// ESTRUTURAS
// ==================================================
struct Bullet {
  float x, y;
  float vy;
  bool active;
};

struct Enemy {
  float x, y;
  float vy;
  int hp;
  int type;
  bool active;
  int w, h;
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
  int type;  // 0 = tiro triplo, 1 = escudo
  bool active;
};

// ==================================================
// ESTADO DO JOGO
// ==================================================
enum GameState { MENU, PLAYING, PAUSED, GAME_OVER };

struct Game {
  GameState state;
  float playerX, playerY;
  int playerW, playerH;
  int lives;
  int score;
  int highScore;
  int level;
  unsigned long lastFireTime;
  unsigned long fireCooldown;
  unsigned long lastEnemySpawn;
  unsigned long enemySpawnInterval;
  int tripleShotTimer;
  int shieldTimer;
  bool btnFirePrev;
} game;

Bullet bullets[MAX_BULLETS];
Enemy enemies[MAX_ENEMIES];
Particle particles[MAX_PARTICLES];
Star stars[MAX_STARS];
PowerUp powerups[MAX_POWERUPS];

// ==================================================
// UTILIDADES
// ==================================================
float randf(float lo, float hi) {
  return lo + (float)random(0, 10000) / 10000.0f * (hi - lo);
}

void initStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(AREA_X, AREA_X + AREA_W);
    stars[i].y = random(AREA_Y, AREA_Y + AREA_H);
    stars[i].speed = randf(0.4f, 2.6f);
    stars[i].bright = random(90, 255);
  }
}

void resetGame() {
  game.playerX = AREA_X + AREA_W / 2.0f;
  game.playerY = AREA_Y + AREA_H - 25;
  game.playerW = 20;
  game.playerH = 22;
  game.lives = 3;
  game.score = 0;
  game.level = 1;
  game.fireCooldown = 180;
  game.lastFireTime = 0;
  game.enemySpawnInterval = 900;
  game.lastEnemySpawn = millis();
  game.tripleShotTimer = 0;
  game.shieldTimer = 0;

  for (int i = 0; i < MAX_BULLETS; i++)  bullets[i].active = false;
  for (int i = 0; i < MAX_ENEMIES; i++)  enemies[i].active = false;
  for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
  for (int i = 0; i < MAX_POWERUPS; i++) powerups[i].active = false;
}

// ==================================================
// SPAWN
// ==================================================
void spawnBullet(float x, float y) {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].x = x;
      bullets[i].y = y;
      bullets[i].vy = -8.0f;
      bullets[i].active = true;
      return;
    }
  }
}

void spawnEnemy() {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) {
      int t = random(0, 100);
      if (t < 70 || game.level < 2) {
        enemies[i].type = 0;
        enemies[i].hp = 1;
        enemies[i].w = 20; enemies[i].h = 18;
      } else if (t < 92) {
        enemies[i].type = 1;
        enemies[i].hp = 2;
        enemies[i].w = 22; enemies[i].h = 20;
      } else {
        enemies[i].type = 2;
        enemies[i].hp = 4;
        enemies[i].w = 28; enemies[i].h = 24;
      }
      enemies[i].x = randf(AREA_X + 20, AREA_X + AREA_W - 20);
      enemies[i].y = AREA_Y - 20;
      enemies[i].vy = 1.2f + game.level * 0.15f + randf(0, 0.5f);
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
      particles[i].x = x;
      particles[i].y = y;
      particles[i].vx = cosf(ang) * sp;
      particles[i].vy = sinf(ang) * sp;
      particles[i].life = random(15, 30);
      particles[i].color = color;
      particles[i].active = true;
      spawned++;
    }
  }
}

void spawnPowerUp(float x, float y) {
  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (!powerups[i].active) {
      powerups[i].x = x;
      powerups[i].y = y;
      powerups[i].vy = 1.5f;
      powerups[i].type = random(0, 2);
      powerups[i].active = true;
      return;
    }
  }
}

// ==================================================
// DESENHO DE ENTIDADES
// ==================================================
void drawPlayer() {
  int x = (int)game.playerX;
  int y = (int)game.playerY;
  uint16_t col = (game.shieldTimer > 0) ? C_POWERUP : C_PLAYER;

  canvas.fillTriangle(x, y - 12, x - 10, y + 8, x + 10, y + 8, col);
  canvas.fillTriangle(x, y - 6, x - 6, y + 6, x + 6, y + 6, C_PLAYER2);
  canvas.fillCircle(x, y - 2, 2, TFT_WHITE);

  // Chama do motor (alterna)
  if ((millis() / 60) % 2 == 0) {
    canvas.fillTriangle(x - 4, y + 8, x + 4, y + 8, x, y + 14, C_EXPLOSION);
  } else {
    canvas.fillTriangle(x - 3, y + 8, x + 3, y + 8, x, y + 12, C_BULLET);
  }

  // Aro de escudo
  if (game.shieldTimer > 0) {
    canvas.drawCircle(x, y, 16, C_POWERUP);
    canvas.drawCircle(x, y, 17, C_POWERUP);
  }
}

void drawEnemy(Enemy &e) {
  int x = (int)e.x, y = (int)e.y;
  uint16_t c = (e.type == 0) ? C_ENEMY1 : (e.type == 1) ? C_ENEMY2 : C_ENEMY3;

  if (e.type == 0) {
    canvas.fillTriangle(x, y + 8, x - 10, y - 8, x + 10, y - 8, c);
    canvas.fillRect(x - 3, y - 4, 6, 6, TFT_WHITE);
  } else if (e.type == 1) {
    canvas.fillRect(x - 10, y - 8, 20, 16, c);
    canvas.fillTriangle(x - 10, y + 8, x - 14, y + 4, x - 10, y + 4, c);
    canvas.fillTriangle(x + 10, y + 8, x + 14, y + 4, x + 10, y + 4, c);
    canvas.fillRect(x - 3, y - 3, 6, 6, TFT_WHITE);
  } else {
    canvas.fillRoundRect(x - 14, y - 12, 28, 24, 4, c);
    canvas.fillRect(x - 8, y - 6, 16, 6, TFT_WHITE);
    canvas.drawRect(x - 14, y - 12, 28, 24, TFT_RED);
  }
}

// ==================================================
// HUD
// ==================================================
void drawUI() {
  canvas.setTextColor(C_UI, C_BG);
  canvas.setTextSize(1);

  canvas.setCursor(6, 6);
  canvas.printf("SCORE %d", game.score);

  canvas.setCursor(SCREEN_W / 2 - 20, 6);
  canvas.printf("LV %d", game.level);

  // Vidas
  for (int i = 0; i < game.lives; i++) {
    int hx = SCREEN_W - 14 - i * 14;
    canvas.fillTriangle(hx, 8, hx - 5, 18, hx + 5, 18, C_PLAYER);
  }

  // Barras de power-up
  if (game.tripleShotTimer > 0) {
    canvas.fillRect(6, 18, 50 * game.tripleShotTimer / 600, 3, C_BULLET);
  }
  if (game.shieldTimer > 0) {
    canvas.fillRect(6, 23, 50 * game.shieldTimer / 800, 3, C_POWERUP);
  }

  // Bordas da área de jogo
  canvas.drawRect(AREA_X - 1, AREA_Y - 1, AREA_W + 2, AREA_H + 2, C_AREA_BORDER);
}

// ==================================================
// UPDATE: ESTRELAS
// ==================================================
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
    canvas.drawPixel((int)stars[i].x, (int)stars[i].y, c);
    if (stars[i].speed > 1.8f) {
      canvas.drawPixel((int)stars[i].x, (int)stars[i].y - 1, c);
    }
  }
}

// ==================================================
// UPDATE: JOGADOR
// ==================================================
void updatePlayer() {
  const float SPEED = 6.0f; //velocidade mudar aqui
  float dx = 0, dy = 0;

  if (GamePad.isUpPressed())    dy -= SPEED;
  if (GamePad.isDownPressed())  dy += SPEED;
  if (GamePad.isLeftPressed())  dx -= SPEED;
  if (GamePad.isRightPressed()) dx += SPEED;

  game.playerX += dx;
  game.playerY += dy;

  // Limites da área de jogo
  if (game.playerX < AREA_X + 12) game.playerX = AREA_X + 12;
  if (game.playerX > AREA_X + AREA_W - 12) game.playerX = AREA_X + AREA_W - 12;
  if (game.playerY < AREA_Y + 12) game.playerY = AREA_Y + 12;
  if (game.playerY > AREA_Y + AREA_H - 12) game.playerY = AREA_Y + AREA_H - 12;
}

// ==================================================
// UPDATE: TIRO
// ==================================================
void tryFire() {
  bool fire = GamePad.isCrossPressed() || GamePad.isCirclePressed();

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
    bullets[i].y += bullets[i].vy;
    if (bullets[i].y < AREA_Y - 5) bullets[i].active = false;
  }
}

// ==================================================
// UPDATE: INIMIGOS
// ==================================================
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

    // Colisão com jogador
    float dx = enemies[i].x - game.playerX;
    float dy = enemies[i].y - game.playerY;
    if (fabsf(dx) < (enemies[i].w / 2 + game.playerW / 2 - 4) &&
        fabsf(dy) < (enemies[i].h / 2 + game.playerH / 2 - 4)) {
      if (game.shieldTimer <= 0) {
        game.lives--;
        spawnParticles(game.playerX, game.playerY, 20, C_EXPLOSION);
        if (game.lives <= 0) {
          if (game.score > game.highScore) game.highScore = game.score;
          game.state = GAME_OVER;
        }
      } else {
        spawnParticles(enemies[i].x, enemies[i].y, 10, C_POWERUP);
      }
      enemies[i].active = false;
      continue;
    }

    if (enemies[i].y > AREA_Y + AREA_H + 30) {
      enemies[i].active = false;
      if (game.score >= 5) game.score -= 5; else game.score = 0;
    }
  }
}

// ==================================================
// UPDATE: COLISÃO BALA x INIMIGO
// ==================================================
void checkBulletHits() {
  for (int b = 0; b < MAX_BULLETS; b++) {
    if (!bullets[b].active) continue;
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
                    (enemies[e].type == 1) ? 25 : 60;
          game.score += pts;
          spawnParticles(enemies[e].x, enemies[e].y, 12, C_EXPLOSION);

          if (random(0, 100) < 12) spawnPowerUp(enemies[e].x, enemies[e].y);
          enemies[e].active = false;
        }
        break;
      }
    }
  }
}

// ==================================================
// UPDATE: PARTÍCULAS
// ==================================================
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

// ==================================================
// UPDATE: POWER-UPS
// ==================================================
void updatePowerUps() {
  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (!powerups[i].active) continue;
    powerups[i].y += powerups[i].vy;

    float dx = powerups[i].x - game.playerX;
    float dy = powerups[i].y - game.playerY;
    if (fabsf(dx) < 14 && fabsf(dy) < 16) {
      if (powerups[i].type == 0) game.tripleShotTimer = 600;
      else                       game.shieldTimer    = 800;
      powerups[i].active = false;
      continue;
    }

    if (powerups[i].y > AREA_Y + AREA_H + 10) powerups[i].active = false;
  }
}

// ==================================================
// UPDATE: TIMERS E DIFICULDADE
// ==================================================
void updateTimers() {
  if (game.tripleShotTimer > 0) game.tripleShotTimer--;
  if (game.shieldTimer    > 0) game.shieldTimer--;

  int newLevel = 1 + game.score / 300;
  if (newLevel > game.level) {
    game.level = newLevel;
    if (game.enemySpawnInterval > 350) game.enemySpawnInterval -= 100;
    if (game.fireCooldown > 100)       game.fireCooldown       -= 15;
  }

  if (millis() - game.lastEnemySpawn > game.enemySpawnInterval) {
    spawnEnemy();
    game.lastEnemySpawn = millis();
  }
}

// ==================================================
// RENDER
// ==================================================
void render() {
  canvas.fillSprite(C_BG);

  drawStars();

  // Partículas
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
    canvas.fillCircle((int)particles[i].x, (int)particles[i].y, 2, particles[i].color);
  }

  // Power-ups
  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (!powerups[i].active) continue;
    uint16_t c = (powerups[i].type == 0) ? C_BULLET : C_POWERUP;
    int px = (int)powerups[i].x;
    int py = (int)powerups[i].y;
    canvas.fillCircle(px, py, 7, c);
    canvas.drawCircle(px, py, 7, TFT_WHITE);
    canvas.setTextColor(TFT_BLACK, c);
    canvas.setTextSize(1);
    canvas.setCursor(px - 3, py - 3);
    canvas.print(powerups[i].type == 0 ? "T" : "S");
  }

  // Inimigos
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) drawEnemy(enemies[i]);
  }

  // Balas
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) continue;
    canvas.fillRect((int)bullets[i].x - 1, (int)bullets[i].y - 4, 2, 8, C_BULLET);
    canvas.drawPixel((int)bullets[i].x, (int)bullets[i].y + 4, TFT_WHITE);
  }

  // Jogador
  if (game.state == PLAYING || game.state == PAUSED) drawPlayer();

  drawUI();

  // ---- MENU ----
  if (game.state == MENU) {
    canvas.fillRect(40, 70, SCREEN_W - 80, 100, C_BG);
    canvas.drawRect(40, 70, SCREEN_W - 80, 100, C_PLAYER);
    canvas.drawRect(42, 72, SCREEN_W - 84, 96, C_PLAYER);

    canvas.setTextColor(C_PLAYER, C_BG);
    canvas.setTextSize(3);
    canvas.setCursor(90, 82);
    canvas.print("SPACE");

    canvas.setTextColor(C_BULLET, C_BG);
    canvas.setCursor(80, 112);
    canvas.print("SHOOTER");

    canvas.setTextColor(C_UI, C_BG);
    canvas.setTextSize(1);
    canvas.setCursor(85, 145);
    canvas.print("START para jogar");

    if (game.highScore > 0) {
      canvas.setCursor(115, 158);
      canvas.printf("HI %d", game.highScore);
    }
  }

  // ---- PAUSA ----
  if (game.state == PAUSED) {
    canvas.fillRect(90, 95, 140, 50, C_BG);
    canvas.drawRect(90, 95, 140, 50, TFT_YELLOW);
    canvas.drawRect(92, 97, 136, 46, TFT_YELLOW);
    canvas.setTextColor(TFT_YELLOW, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(125, 113);
    canvas.print("PAUSA");
  }

  // ---- GAME OVER ----
  if (game.state == GAME_OVER) {
    canvas.fillRect(40, 80, SCREEN_W - 80, 80, C_BG);
    canvas.drawRect(40, 80, SCREEN_W - 80, 80, C_ENEMY1);
    canvas.drawRect(42, 82, SCREEN_W - 84, 76, C_ENEMY1);

    canvas.setTextColor(C_ENEMY1, C_BG);
    canvas.setTextSize(2);
    canvas.setCursor(85, 90);
    canvas.print("GAME OVER");

    canvas.setTextColor(C_UI, C_BG);
    canvas.setTextSize(1);
    canvas.setCursor(95, 120);
    canvas.printf("SCORE: %d", game.score);
    canvas.setCursor(85, 135);
    canvas.print("START p/ reiniciar");
  }

  canvas.pushSprite(0, 0);
}

// ==================================================
// SETUP
// ==================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println(" SPACE SHOOTER - ESP32-S3");
  Serial.println(" ILI9341 + DABBLE BLE");
  Serial.println("================================");

  // ---- TFT ----
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);

  // ---- SPRITE (buffer, evita flicker) ----
  canvas.setColorDepth(8);
  if (!canvas.createSprite(SCREEN_W, SCREEN_H)) {
    Serial.println("ERRO: falha ao criar sprite!");
    while (1) delay(100);
  }
  canvas.fillSprite(C_BG);

  randomSeed(esp_random());

  initStars();
  resetGame();
  game.highScore = 0;
  game.state = MENU;

  // ---- DABBLE ----
  Serial.println("Iniciando BLE...");
  Dabble.begin("ESP32-S3-GAMEPAD");
  Serial.println("BLE OK. Nome: ESP32-S3-GAMEPAD");
  Serial.println("Aguardando conexao no app Dabble...");

  Serial.println();
  Serial.println("Controles:");
  Serial.println("  UP/DOWN/LEFT/RIGHT -> mover");
  Serial.println("  CROSS / CIRCLE     -> atirar");
  Serial.println("  START              -> iniciar/pausar/reiniciar");
  Serial.println("  SELECT             -> menu (no Game Over)");
  Serial.println();
  Serial.println("Pronto!");
}

// ==================================================
// LOOP
// ==================================================
unsigned long lastFrame = 0;
const int FRAME_MS = 16;  // ~60 FPS

bool lastStart = false;
bool lastSelect = false;

void loop() {
  // Processa dados do Dabble
  Dabble.processInput();

  unsigned long now = millis();
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;

  // =================================================
  // BOTÃO START (borda de subida)
  // =================================================
  bool startNow = GamePad.isStartPressed();
  if (startNow && !lastStart) {
    if (game.state == MENU) {
      resetGame();
      game.state = PLAYING;
      Serial.println("START -> jogando");
    }
    else if (game.state == PLAYING) {
      game.state = PAUSED;
      Serial.println("START -> pausado");
    }
    else if (game.state == PAUSED) {
      game.state = PLAYING;
      Serial.println("START -> continuando");
    }
    else if (game.state == GAME_OVER) {
      resetGame();
      game.state = PLAYING;
      Serial.println("START -> reiniciando");
    }
  }
  lastStart = startNow;

  // =================================================
  // BOTÃO SELECT (volta ao menu no Game Over)
  // =================================================
  bool selectNow = GamePad.isSelectPressed();
  if (selectNow && !lastSelect) {
    if (game.state == GAME_OVER) {
      game.state = MENU;
      Serial.println("SELECT -> menu");
    }
  }
  lastSelect = selectNow;

  // =================================================
  // UPDATE POR ESTADO
  // =================================================
  if (game.state == MENU) {
    updateStars();
  }
  else if (game.state == PLAYING) {
    updateStars();
    updatePlayer();
    tryFire();
    updateBullets();
    updateEnemies();
    checkBulletHits();
    updateParticles();
    updatePowerUps();
    updateTimers();
  }
  else if (game.state == PAUSED) {
    // congela tudo
  }
  else if (game.state == GAME_OVER) {
    updateStars();
    updateParticles();
  }

  render();
}