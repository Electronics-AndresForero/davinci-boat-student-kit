// ============================================================================
//  Phase 4 — RC integration: Nintendo Switch Pro pad -> Command -> motor + rudder.
//  Motor on M0 (27/13) @ 5 kHz ; rudder servo on 25 @ 50 Hz ; controller via Bluetooth.
//  Left stick X = steering, right stick Y (up = forward) = throttle. ZL/ZR unused.
//  Needs a classic-BT ESP32 (esp32dev): the Switch Pro is not a BLE device.
// ============================================================================
#include <Arduino.h>
#include <Bluepad32.h>

// ---------- pins ----------
constexpr int PIN_DRIVE_A   = 27, PIN_DRIVE_B = 13; // motor port M0
constexpr int PIN_STEER     = 25;                   // servo header
constexpr int PIN_LED       = 33;                   // status LED
constexpr int PIN_ARM_SWITCH = 32;   // SPDT (COM + one throw), other side to GND;
                                      // INPUT_PULLUP: closed=LOW=armed, open=HIGH=disarmed

// ---------- motor PWM (5 kHz — what the SS6625E can follow!) ----------
constexpr int MOT_FREQ = 5000, MOT_BITS = 10, MOT_MAX = (1 << MOT_BITS) - 1;

// ---------- servo PWM (50 Hz) ----------
constexpr int SRV_FREQ = 50, SRV_BITS = 16, SRV_PERIOD_US = 20000;
constexpr int STEER_CENTER_US = 1500, STEER_RANGE_US = 750;   // trim from Phase 3

// ---------- tunables ----------
constexpr float DEADZONE      = 0.06f;   // use YOUR Phase 1 value
constexpr float THROTTLE_SLEW = 4.0f;    // max throttle change per second (0->full in ~0.25s)
// SAFETY: the bench TT motor AND the real JGA25-370 drive motor are both 6 V-rated;
// our 2S 18650 pack is 7.4 V (up to 8.4 V full charge). 1.0 would mean full pack
// voltage continuously -- up to ~40% overvoltage, worst-case right at the stall/
// breakaway moment when current is already highest. 0.90 was chosen as a deliberate
// compromise (2026-09-25): still below the pack's 8.4 V full-charge ceiling, while the
// old paddle wheels needed most of the trigger's range to break stiction at all.
// Revisit this once the paddles are rebuilt and re-tested -- see duty_sweep tool.
constexpr float DUTY_CAP      = 0.90f;

// ---------- the normalized command (the bus between input and actuators) ----------
struct Command {
  float throttle = 0;    // -1..+1
  float steer    = 0;    // -1..+1
  bool  armed    = false;
  unsigned long stamp = 0;
};

// ---------- actuators (each just reads a number, knows nothing about inputs) ----------
class PaddleDrive {
public:
  void begin() { ledcAttach(PIN_DRIVE_A, MOT_FREQ, MOT_BITS);
                 ledcAttach(PIN_DRIVE_B, MOT_FREQ, MOT_BITS); stop(); }
  void set(float t) {
    t = constrain(t, -1.f, 1.f);
    uint32_t duty = (uint32_t)(fabs(t) * MOT_MAX);
    ledcWrite(PIN_DRIVE_A, t >= 0 ? duty : 0);
    ledcWrite(PIN_DRIVE_B, t <  0 ? duty : 0);
  }
  void stop() { ledcWrite(PIN_DRIVE_A, 0); ledcWrite(PIN_DRIVE_B, 0); }
};

class Steering {
public:
  void begin() { ledcAttach(PIN_STEER, SRV_FREQ, SRV_BITS); set(0); }
  void set(float s) {
    s = constrain(s, -1.f, 1.f);
    int us = STEER_CENTER_US + (int)(s * STEER_RANGE_US);
    ledcWrite(PIN_STEER, (uint32_t)((uint64_t)us * (1UL << SRV_BITS) / SRV_PERIOD_US));
  }
};

PaddleDrive drive;
Steering    steer;

// ---------- input: Nintendo Switch controller ----------
ControllerPtr pad = nullptr;
void onConnect(ControllerPtr c)    { if (!pad) pad = c; }
void onDisconnect(ControllerPtr c) { if (pad == c) pad = nullptr; }

float deadzone(float v) { return fabs(v) < DEADZONE ? 0.f : v; }

bool armSwitchClosed() { return digitalRead(PIN_ARM_SWITCH) == LOW; }

// Fill `cmd` from the controller. Returns false if we have no live pad.
bool readController(Command& cmd) {
  if (!pad || !pad->isConnected() || !pad->isGamepad()) return false;
  // ZL/ZR are digital buttons on the Switch (throttle()/brake() stay 0), so
  // throttle comes from the right stick. Bluepad32: stick up = NEGATIVE Y.
  cmd.throttle = deadzone(-pad->axisRY() / 512.0f);   // right stick, up = forward
  cmd.steer    = deadzone(pad->axisX()   / 512.0f);   // left stick, right = +
  cmd.armed    = true;
  cmd.stamp    = millis();
  return true;
}

Command cmd;
float applied = 0.0f;                 // slew-limited throttle actually sent
unsigned long lastLoop = 0;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_ARM_SWITCH, INPUT_PULLUP);
  drive.begin();
  steer.begin();
  BP32.setup(&onConnect, &onDisconnect);
  // BP32.forgetBluetoothKeys();      // E1 bring-up only -- see E1 README.
  // Leave this COMMENTED OUT from E4 on. It wipes the ESP32's half of the
  // bond on every boot while the pad keeps its half, so the pad's
  // auto-reconnect is refused and it never connects unless you hold Sync
  // (small button on top of the Switch Pro until the LEDs sweep back and forth).
  BP32.enableVirtualDevice(false);
  lastLoop = millis();
  Console.println("\n[Phase 4] RC integration. Board on a stand — wheels off the ground!");
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastLoop) / 1000.0f; lastLoop = now;

  BP32.update();
  if (!readController(cmd)) {          // no fresh pad data -> hold neutral
    cmd.armed = false;
  }
  if (!armSwitchClosed()) cmd.armed = false;   // hardware kill switch -- overrides everything

  // throttle ramp (slew limit); steering passes straight through
  float target  = cmd.armed ? cmd.throttle * DUTY_CAP : 0.0f;
  float maxStep = THROTTLE_SLEW * dt;
  applied += constrain(target - applied, -maxStep, maxStep);

  drive.set(applied);
  steer.set(cmd.armed ? cmd.steer : 0.0f);

  digitalWrite(PIN_LED, cmd.armed ? HIGH : (now / 300 % 2));  // solid=armed, blink=idle
  delay(10);   // ~100 Hz control loop
}
