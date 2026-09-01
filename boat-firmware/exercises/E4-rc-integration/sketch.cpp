// ============================================================================
//  E4 — RC integration: Xbox pad -> Command -> motor + rudder.
//  Motor on M0 (27/13) @ 5 kHz ; rudder servo on 25 @ 50 Hz ; controller via BLE.
// ============================================================================
#include <Arduino.h>
#include <Bluepad32.h>

// ---------- pins ----------
constexpr int PIN_DRIVE_A = 27, PIN_DRIVE_B = 13;   // motor port M0
constexpr int PIN_STEER   = 25;                     // servo header
constexpr int PIN_LED     = 33;                     // status LED

// ---------- motor PWM (5 kHz — what the SS6625E can follow!) ----------
constexpr int MOT_FREQ = 5000, MOT_BITS = 10, MOT_MAX = (1 << MOT_BITS) - 1;

// ---------- servo PWM (50 Hz) ----------
constexpr int SRV_FREQ = 50, SRV_BITS = 16, SRV_PERIOD_US = 20000;
constexpr int STEER_CENTER_US = 1500, STEER_RANGE_US = 400;   // trim from E3

// ---------- tunables ----------
constexpr float DEADZONE      = 0.06f;   // use YOUR E1 value
constexpr float THROTTLE_SLEW = 2.0f;    // max throttle change per second
// SAFETY: the bench TT motor AND the real JGA25-370 drive motor are both 6 V-rated;
// our 2S 18650 pack is 7.4 V (up to 8.4 V full charge). 0.60 keeps us comfortably
// under 6 V. This cap is PERMANENT -- do not raise it toward 1.0 on the real motor,
// that would overvolt it.
constexpr float DUTY_CAP      = 0.60f;

// ---------- the normalized command (the bus between input and actuators) ----------
struct Command {
  float throttle = 0;    // -1..+1
  float steer    = 0;    // -1..+1
  bool  armed    = false;
  unsigned long stamp = 0;
};

// --- paste your E2 PaddleDrive and E3 Steering classes here, unchanged ---


PaddleDrive drive;
Steering    steer;

// ---------- input: Xbox controller ----------
ControllerPtr pad = nullptr;
void onConnect(ControllerPtr c)    { if (!pad) pad = c; }
void onDisconnect(ControllerPtr c) { if (pad == c) pad = nullptr; }

float deadzone(float v) { return fabs(v) < DEADZONE ? 0.f : v; }

// Fill `cmd` from the controller. Returns false if we have no live pad.
bool readController(Command& cmd) {
  if (!pad || !pad->isConnected() || !pad->isGamepad()) return false;
  // TODO 1: fill cmd.throttle (RT/1023 - LT/1023, deadzoned) and
  // cmd.steer (axisX/512, deadzoned). Set cmd.armed = true and
  // cmd.stamp = millis(). Return true.
}

Command cmd;
float applied = 0.0f;                 // slew-limited throttle actually sent
unsigned long lastLoop = 0;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  drive.begin();
  steer.begin();
  BP32.setup(&onConnect, &onDisconnect);
  BP32.forgetBluetoothKeys();         // clean pairing during bring-up
  BP32.enableVirtualDevice(false);
  lastLoop = millis();
  Console.println("\n[E4] RC integration. Board on a stand — wheels off the ground!");
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastLoop) / 1000.0f; lastLoop = now;

  BP32.update();
  if (!readController(cmd)) {          // no fresh pad data -> hold neutral
    cmd.armed = false;
  }

  // TODO 2: compute `target` (cmd.throttle * DUTY_CAP if armed, else 0),
  // ramp `applied` toward `target` by at most THROTTLE_SLEW * dt per loop
  // (reuse the constrain-based approach idea from E2), then call
  // drive.set(applied) and steer.set(cmd.armed ? cmd.steer : 0.0f).

  digitalWrite(PIN_LED, cmd.armed ? HIGH : (now / 300 % 2));  // solid=armed, blink=idle
  delay(10);   // ~100 Hz control loop
}
