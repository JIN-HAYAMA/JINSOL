// ==== M5 ATOM S3 ====
// ポーリング方式（割り込みなし）
// G5: 反転入力（INPUT_PULLUP）→ G38(H=FALLING / L=RISING)で方向選択、G6&G7に7msパルス
// G2: 非反転入力（INPUT_PULLDOWN）→ G38に従いG6パルス
// G1: 非反転入力（INPUT_PULLDOWN）→ G38に従いG7パルス
// G39→G8: 反転ミラー出力
// G38: INPUT_PULLUP（方向選択）

#define PIN_IN_EDGE   5
#define PIN_OUT1      6
#define PIN_OUT2      7
#define PIN_IN_MIR    39
#define PIN_OUT_MIR   8
#define PIN_EDGE_SEL  38
#define PIN_LVL_TO_G6 2
#define PIN_LVL_TO_G7 1

const uint32_t PULSE_MS    = 7;   // ★パルス幅7ms
const uint32_t DEBOUNCE_MS = 20;  // デバウンス時間

bool     pulse1Active = false, pulse2Active = false;
uint32_t pulse1Start  = 0, pulse2Start = 0;

int  g5_rawPrev = HIGH, g5_stableInv = LOW;
int  g2_rawPrev = LOW,  g2_stable = LOW;
int  g1_rawPrev = LOW,  g1_stable = LOW;
uint32_t g5_bounceMs = 0, g2_bounceMs = 0, g1_bounceMs = 0;

void triggerG6(uint32_t now) { digitalWrite(PIN_OUT1, HIGH); pulse1Active = true; pulse1Start = now; }
void triggerG7(uint32_t now) { digitalWrite(PIN_OUT2, HIGH); pulse2Active = true; pulse2Start = now; }

void setup() {
  pinMode(PIN_IN_EDGE,   INPUT_PULLUP);   // G5
  pinMode(PIN_LVL_TO_G6, INPUT_PULLDOWN); // G2
  pinMode(PIN_LVL_TO_G7, INPUT_PULLDOWN); // G1
  pinMode(PIN_EDGE_SEL,  INPUT_PULLUP);   // G38
  pinMode(PIN_IN_MIR,    INPUT_PULLUP);   // G39
  pinMode(PIN_OUT_MIR,   OUTPUT);
  pinMode(PIN_OUT1,      OUTPUT);
  pinMode(PIN_OUT2,      OUTPUT);

  digitalWrite(PIN_OUT1, LOW);
  digitalWrite(PIN_OUT2, LOW);
  digitalWrite(PIN_OUT_MIR, LOW);

  uint32_t now = millis();
  g5_rawPrev = digitalRead(PIN_IN_EDGE);
  g5_stableInv = !g5_rawPrev;
  g2_rawPrev = digitalRead(PIN_LVL_TO_G6);
  g2_stable = g2_rawPrev;
  g1_rawPrev = digitalRead(PIN_LVL_TO_G7);
  g1_stable = g1_rawPrev;
  g5_bounceMs = g2_bounceMs = g1_bounceMs = now;
}

void loop() {
  uint32_t now = millis();

  // G39→G8 反転ミラー
  digitalWrite(PIN_OUT_MIR, !digitalRead(PIN_IN_MIR));

  // G38で方向決定（H=FALLING, L=RISING）
  bool g38raw = digitalRead(PIN_EDGE_SEL);
  bool wantRising = !g38raw;

  // ==== G5（反転）====
  {
    int raw = digitalRead(PIN_IN_EDGE);
    if (raw != g5_rawPrev) { g5_rawPrev = raw; g5_bounceMs = now; }
    if ((now - g5_bounceMs) >= DEBOUNCE_MS) {
      int invNow = !raw;
      if (invNow != g5_stableInv) {
        int prev = g5_stableInv; g5_stableInv = invNow;
        bool rising  = (prev==LOW && invNow==HIGH);
        bool falling = (prev==HIGH && invNow==LOW);
        if ((wantRising && rising) || (!wantRising && falling)) {
          triggerG6(now); triggerG7(now);
        }
      }
    }
  }

  // ==== G2（非反転, プルダウン）====
  {
    int raw = digitalRead(PIN_LVL_TO_G6);
    if (raw != g2_rawPrev) { g2_rawPrev = raw; g2_bounceMs = now; }
    if ((now - g2_bounceMs) >= DEBOUNCE_MS) {
      if (raw != g2_stable) {
        int prev = g2_stable; g2_stable = raw;
        bool rising  = (prev==LOW && raw==HIGH);
        bool falling = (prev==HIGH && raw==LOW);
        if ((wantRising && rising) || (!wantRising && falling)) {
          triggerG6(now);
        }
      }
    }
  }

  // ==== G1（非反転, プルダウン）====
  {
    int raw = digitalRead(PIN_LVL_TO_G7);
    if (raw != g1_rawPrev) { g1_rawPrev = raw; g1_bounceMs = now; }
    if ((now - g1_bounceMs) >= DEBOUNCE_MS) {
      if (raw != g1_stable) {
        int prev = g1_stable; g1_stable = raw;
        bool rising  = (prev==LOW && raw==HIGH);
        bool falling = (prev==HIGH && raw==LOW);
        if ((wantRising && rising) || (!wantRising && falling)) {
          triggerG7(now);
        }
      }
    }
  }

  // ==== パルス終了 ====
  if (pulse1Active && (now - pulse1Start >= PULSE_MS)) {
    digitalWrite(PIN_OUT1, LOW);
    pulse1Active = false;
  }
  if (pulse2Active && (now - pulse2Start >= PULSE_MS)) {
    digitalWrite(PIN_OUT2, LOW);
    pulse2Active = false;
  }
}
