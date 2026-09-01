// ============================================================================
//  E5 — modes + arming + failsafe + a real autonomous run.  (Builds on E4.)
// ============================================================================
#include <Arduino.h>
#include <Bluepad32.h>

constexpr int PIN_DRIVE_A = 27, PIN_DRIVE_B = 13, PIN_STEER = 25, PIN_LED = 33;
constexpr int PIN_MODE_SWITCH = 26;   // LOW = AUTO, HIGH = RC
constexpr int PIN_ARM_SWITCH  = 32;   // LOW = ARMED, HIGH = disarmed

constexpr int MOT_FREQ = 5000, MOT_BITS = 10, MOT_MAX = (1 << MOT_BITS) - 1;
constexpr int SRV_FREQ = 50, SRV_BITS = 16, SRV_PERIOD_US = 20000;
constexpr int STEER_CENTER_US = 1500, STEER_RANGE_US = 400;

constexpr float DEADZONE      = 0.06f;
constexpr float THROTTLE_SLEW = 2.0f;
constexpr float DUTY_CAP      = 0.40f;         // raise to 1.0 with the real 12 V motor
constexpr unsigned long FAILSAFE_MS = 500;     // stop if no fresh command within this

struct Command { float throttle=0, steer=0; bool armed=false; unsigned long stamp=0; };

// --- paste your PaddleDrive, Steering, onConnect/onDisconnect, deadzone(),
//     and readController() from E4 here, unchanged ---


PaddleDrive drive; Steering steer;
ControllerPtr pad = nullptr;

// ---- input: autonomous (fill in below) ----
enum Mode { MODE_RC, MODE_AUTO };

Mode readMode() {
  // TODO 1: return MODE_AUTO if the mode switch reads LOW, else MODE_RC.
}

// knock `armed` false if the arm switch is off OR the command is stale
void applyFailsafe(Command& c) {
  // TODO 2: force c.armed = false if EITHER the newest command is older
  // than FAILSAFE_MS, OR the arm switch reads HIGH (open = disarmed).
}

// ---- autonomous run parameters ----
constexpr float         AUTO_CRUISE = 0.55f;    // forward throttle in AUTO (0..1)
constexpr unsigned long AUTO_RUN_MS = 15000;    // straight run length, then stop

class AutoPlanner {
public:
  void arm()    { t0 = millis(); running = true; }     // called on entering AUTO
  void disarm() { running = false; }                   // called on leaving AUTO

  void poll(Command& c) {
    // TODO 3: compute elapsed = millis() - t0. If `running` AND elapsed <
    // AUTO_RUN_MS: set c.throttle = AUTO_CRUISE, c.steer = 0, c.armed = true.
    // Otherwise: c.throttle = 0, c.armed = false. Either way, c.stamp = millis().
  }
private:
  unsigned long t0 = 0; bool running = false;
};
AutoPlanner planner;

Command cmd; float applied = 0; unsigned long lastLoop = 0; Mode lastMode = MODE_RC;

void setup() {
  pinMode(PIN_MODE_SWITCH, INPUT_PULLUP);
  pinMode(PIN_ARM_SWITCH,  INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  drive.begin(); steer.begin();
  BP32.setup(&onConnect, &onDisconnect);
  BP32.forgetBluetoothKeys(); BP32.enableVirtualDevice(false);
  lastLoop = millis();
  Console.println("\n[E5] modes + arm + failsafe + autonomous. Boat on a stand!");
}

void loop() {
  unsigned long now = millis(); float dt = (now - lastLoop) / 1000.f; lastLoop = now;
  BP32.update();

  Mode mode = readMode();
  if (mode != lastMode) {                          // edge: entering/leaving AUTO
    if (mode == MODE_AUTO) planner.arm(); else planner.disarm();
    applied = 0; lastMode = mode;
  }

  if (mode == MODE_AUTO) planner.poll(cmd);
  else if (!readController(cmd)) cmd.armed = false;       // RC with no fresh data -> neutral

  applyFailsafe(cmd);                                     // arm switch + link-loss

  float target = cmd.armed ? cmd.throttle * DUTY_CAP : 0.f;
  applied += constrain(target - applied, -THROTTLE_SLEW * dt, THROTTLE_SLEW * dt);
  drive.set(applied);
  steer.set(cmd.armed ? cmd.steer : 0.f);

  // LED: solid=armed, slow blink=RC idle, fast blink=AUTO idle
  int blink = (mode == MODE_AUTO) ? (now / 150 % 2) : (now / 400 % 2);
  digitalWrite(PIN_LED, cmd.armed ? HIGH : blink);
  delay(10);
}
