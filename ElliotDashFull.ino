#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <vector>

#ifdef ARDUINO_ARCH_ESP32
  #include <esp_random.h>
#endif

// Pin Initiation
#define TFT_CS   10
#define TFT_DC    9
#define TFT_RST  -1

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// FPS

unsigned long lastFrame = millis();

// Button Pins
#define PIN_JUMP     2   // Space
#define PIN_RESTART  4   // 'R' (used as "back to start" on Game Over)
#define PIN_MENU     5   // 'B' (menu / back)

// Screen/Game Setup
static constexpr int   SCR_W = 320;
static constexpr int   SCR_H = 240;
static constexpr float GROUND_Y = 200.f;
static constexpr float floatingO_Ground_Y = 150.f;

static constexpr float PLAYER_W = 18.f;
static constexpr float PLAYER_H = 18.f;

static constexpr float GRAVITY      = 2000.f;
static constexpr float JUMP_IMPULSE = -600.f;
static constexpr float MAX_FALL     = 2000.f;

static constexpr float COYOTE        = 0.08f;
static constexpr float JUMP_BUFFER   = 0.10f;
static constexpr float HOLD_MAX      = 0.18f;
static constexpr float HOLD_GRAVITY  = 0.43f;
static constexpr float RELEASE_CUT   = 0.45f;

// GameState
enum class GameState { Start, Playing, GameOver };
GameState state = GameState::Start;

struct Buttons {
  bool jumpHeld    = false;
  bool jumpEdge    = false;
  bool restartEdge = false;
  bool menuEdge    = false;
};

struct Input {
  bool _pj=false,_pr=false,_pm=false;
  uint32_t _tLast=0;

  void begin() {
    pinMode(PIN_JUMP,    INPUT_PULLUP);
    pinMode(PIN_RESTART, INPUT_PULLUP);
    pinMode(PIN_MENU,    INPUT_PULLUP);
    _pj = digitalRead(PIN_JUMP)==LOW;
    _pr = digitalRead(PIN_RESTART)==LOW;
    _pm = digitalRead(PIN_MENU)==LOW;
    _tLast = millis();
  }

  Buttons poll() {
    Buttons b;
    uint32_t now = millis();
    if (now - _tLast < 8) return b;
    _tLast = now;

    bool j = digitalRead(PIN_JUMP)==LOW;
    bool r = digitalRead(PIN_RESTART)==LOW;
    bool m = digitalRead(PIN_MENU)==LOW;

    b.jumpHeld    = j;
    b.jumpEdge    = (j && !_pj);
    b.restartEdge = (r && !_pr);
    b.menuEdge    = (m && !_pm);

    _pj = j; _pr = r; _pm = m;
    return b;
  }
} input;

class Player {
public:
  void reset() {
    _x = 40.f;
    _y = GROUND_Y - PLAYER_H;
    _vy = 0.f;
    _coyote = 0.f;
    _buffer = 0.f;
    _hold   = 0.f;
    _grounded = true;
  }

  void update(float dt, bool jumpHeld, bool jumpEdge) {
    if (_coyote > 0) _coyote -= dt;
    if (_buffer > 0) _buffer -= dt;

    if (_grounded) _coyote = COYOTE;

    if (jumpEdge) _buffer = JUMP_BUFFER;

    if (_buffer > 0 && _coyote > 0) {
      _vy = JUMP_IMPULSE;
      _grounded = false;
      _buffer = 0;
      _coyote = 0;
      _hold   = 0;
    }

    if (_vy < 0 && jumpHeld && _hold < HOLD_MAX) {
      _vy += (GRAVITY * HOLD_GRAVITY) * dt;
      _hold += dt;
    } else {
      _vy += GRAVITY * dt;
    }
    if (_vy > MAX_FALL) _vy = MAX_FALL;

    // move player
    _y += _vy * dt;

    // clamp to ground
    float floor = GROUND_Y - PLAYER_H;
    if (_y > floor) {
      _y = floor;
      _vy = 0.f;
      _grounded = true;
    } else {
      _grounded = false;
    }
  }

  void draw(Adafruit_ILI9341& d) const {
    d.fillRect((int)_x, (int)_y, (int)PLAYER_W, (int)PLAYER_H, ILI9341_YELLOW);
  }

  struct AABB { int x,y,w,h; };
  AABB bounds() const { return {(int)_x, (int)_y, (int)PLAYER_W, (int)PLAYER_H}; }

private:
  float _x=40.f, _y=GROUND_Y-PLAYER_H, _vy=0.f;
  float _coyote=0.f, _buffer=0.f, _hold=0.f;
  bool  _grounded=true;
} player;

// Obstacles
struct Ob { float x,y,w,h, vy; bool bouncing; };

class Obstacles {
public:
  void reset() {
    _obs.clear();
    _spawnAcc = 0.f;
    _spawnInterval = 1.8f;
  }

  void update(float dt, float speed) {

    _spawnAcc += dt;
    if (_spawnAcc >= _spawnInterval) {
      _spawnAcc = 0.f;
      if (_spawnInterval > 0.6f) _spawnInterval -= 0.2f;

      int kind = random(0,4);
      Ob o{};
      switch (kind) {
        case 0: // low wide
          o.w = 28 + random(0, 40); o.h = 16 + random(0, 8);
          o.y = GROUND_Y - o.h; o.bouncing = false; o.vy = 0;
          break;
        case 1: // medium
          o.w = 22 + random(0, 24); o.h = 28 + random(0, 20);
          o.y = GROUND_Y - o.h; o.bouncing = false; o.vy = 0;
          break;
        case 2: // tall thin
          o.w = 14 + random(0, 12); o.h = 50 + random(0, 30);
          o.y = GROUND_Y - o.h; o.bouncing = false; o.vy = 0;
          break;
        case 3: // floating (simple bouncing block between two limits)
          o.w = 20 + random(0, 24); o.h = 16 + random(0, 8);
          o.y = (GROUND_Y - 50 - random(0, 40));
          o.bouncing = true;
          o.vy = (random(0,2)==0) ? -180.f : 180.f;
          break;
      }
      o.x = SCR_W + o.w;
      _obs.push_back(o);
    }

    for (auto &o : _obs) {
      o.x -= speed * dt;
      if (o.bouncing) {
        o.y += o.vy * dt;
        float top = 100.f;
        float bottom = GROUND_Y - o.h - 4.f;
        if (o.y < top)   { o.y = top;   o.vy = fabs(o.vy); }
        if (o.y > bottom){ o.y = bottom; o.vy = -fabs(o.vy); }
      }
    }
    while (!_obs.empty() && _obs.front().x + _obs.front().w < 0) {
      _obs.erase(_obs.begin());
    }
  }

  bool collide(const Player::AABB& p) const {
    for (const auto &o : _obs) {
      if (aabb(p.x,p.y,p.w,p.h, (int)o.x,(int)o.y,(int)o.w,(int)o.h)) return true;
    }
    return false;
  }

  void draw(Adafruit_ILI9341& d) const {
    for (const auto &o : _obs) {
      d.fillRect((int)o.x, (int)o.y, (int)o.w, (int)o.h, ILI9341_RED);
    }
  }

  void clearAll(){ _obs.clear(); }

  int count() const { return (int)_obs.size(); }
  const Ob& get(int i) const { return _obs[i]; }

private:
  static bool aabb(int ax,int ay,int aw,int ah, int bx,int by,int bw,int bh){
    return (ax < bx + bw) && (ax + aw > bx) && (ay < by + bh) && (ay + ah > by);
  }

  std::vector<Ob> _obs;
  float _spawnAcc = 0.f;
  float _spawnInterval = 1.8f;
} obstacles;

unsigned long prevMs = 0;
unsigned long scoreStartMs = 0;
float blockSpeed = 125.f;
unsigned long lastScoreSec = 0;

Player::AABB prevPlayer = {0,0,0,0};
bool          havePrevPlayer = false;

std::vector<Ob> prevObs;

static void drawBackgroundOnce() {
  tft.fillRect(0, 0, SCR_W, (int)GROUND_Y, ILI9341_NAVY);
  tft.fillRect(0, (int)GROUND_Y, SCR_W, SCR_H-(int)GROUND_Y, ILI9341_DARKGREEN);
}

static void eraseRect(int x, int y, int w, int h) {
  int gy = (int)GROUND_Y;

  int skyTop = y;
  int skyBot = min(y + h, gy);
  if (skyTop < skyBot) {
    tft.fillRect(x, skyTop, w, skyBot - skyTop, ILI9341_NAVY);
  }

  int grdTop = max(y, gy);
  int grdBot = y + h;
  if (grdTop < grdBot) {
    tft.fillRect(x, grdTop, w, grdBot - grdTop, ILI9341_DARKGREEN);
  }
}

static void resetGame() {
  tft.fillScreen(ILI9341_BLACK);
  drawBackgroundOnce();

  player.reset();
  obstacles.reset();
  blockSpeed = 125.f;
  scoreStartMs = millis();

  havePrevPlayer = false;
  prevObs.clear();

  tft.fillRect(0,0,120,20, ILI9341_NAVY);
  tft.setCursor(6,6);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_CYAN, ILI9341_NAVY);
  tft.print("Score: 0");
  lastScoreSec = 0;
}

static void drawStart() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(46, 60);
  tft.println("Elliot Dash");
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_RED);
  tft.setCursor(46, 110);
  tft.println("Press JUMP to start");
}

static void drawGameOver() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(3);
  tft.setCursor(60,70);
  tft.println("GAME OVER");
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(40, 118);
  tft.println("R: Restart");
  tft.setCursor(40, 144);
  tft.println("B: Start Screen");
}

void setup() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

#ifdef ARDUINO_ARCH_ESP32
  randomSeed(esp_random());
#else
  randomSeed(analogRead(A0));
#endif

  input.begin();
  drawStart();
}

void loop() {

 // FPS implementation for smoother graphics
 while(millis() - lastFrame < 20);
 lastFrame = millis();

  const unsigned long now = millis();
  const float dt = (now - prevMs) / 1000.0f;
  prevMs = now;

  const Buttons btn = input.poll();

  if (state == GameState::Start) {
    if (btn.jumpEdge) { resetGame(); state = GameState::Playing; }
    delay(10);
    return;
  }

  if (state == GameState::Playing) {

    if (havePrevPlayer) {
      eraseRect(prevPlayer.x, prevPlayer.y, prevPlayer.w, prevPlayer.h);
    }
    for (const auto& po : prevObs) {
      eraseRect((int)po.x, (int)po.y, (int)po.w, (int)po.h);
    }

    player.update(dt, btn.jumpHeld, btn.jumpEdge);
    obstacles.update(dt, blockSpeed);

    static float acc = 0.f; acc += dt;
    if (acc >= 2.f) { acc = 0.f; if (blockSpeed < 300.f) blockSpeed += 50.f; }

    if (obstacles.collide(player.bounds())) {
      state = GameState::GameOver;
      drawGameOver();
      return;
    }

    obstacles.draw(tft);
    player.draw(tft);

    prevPlayer = player.bounds();
    havePrevPlayer = true;

    prevObs.clear();
    prevObs.reserve(obstacles.count());
    for (int i = 0; i < obstacles.count(); ++i) {
      prevObs.push_back(obstacles.get(i));
    }

    unsigned long sec = (millis() - scoreStartMs)/1000UL;
    if (sec != lastScoreSec) {
      lastScoreSec = sec;
      tft.fillRect(0,0,120,20, ILI9341_NAVY);
      tft.setCursor(6,6);
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_CYAN, ILI9341_NAVY);
      char buf[32];
      snprintf(buf, sizeof(buf), "Score: %lu", sec);
      tft.print(buf);
    }

    return;
  }

  if (state == GameState::GameOver) {
    if (btn.jumpEdge) {
      resetGame();
      state = GameState::Playing;
    } else if (btn.menuEdge || btn.restartEdge) {
      state = GameState::Start;
      drawStart();
    }
    delay(10);
    return;
  }
}
