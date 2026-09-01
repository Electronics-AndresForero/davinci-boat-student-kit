// ============================================================================
//  E2 — drive one motor via the onboard SS6625E using LEDC (core 3.x API).
//  No controller yet: an automated sequence ramps forward, then reverse, on loop.
// ============================================================================
#include <Arduino.h>
#include <Bluepad32.h>          // we're in the Bluepad32 project; we use its Console

// ---- Pin map: drive motor on port M0 ----
constexpr int PIN_DRIVE_A = 27;   // PWM this pin -> forward
constexpr int PIN_DRIVE_B = 13;   // PWM this pin -> reverse

// ---- PWM setup ----
constexpr int   PWM_FREQ = 5000;  // 5 kHz: what the SS6625E can actually follow
                                   // (20 kHz can make the driver output nothing!)
constexpr int   PWM_BITS = 10;    // 10-bit resolution -> duty 0..1023
constexpr int   PWM_MAX  = (1 << PWM_BITS) - 1;   // 1023

// SAFETY: TT motor is 3-6 V. Our 2S 18650 pack is 7.4 V nominal (up to 8.4 V full
// charge) -- at 0.60 duty that's ~4.4-5.0 V, comfortably under the motor's 6 V max.
// Raise toward 1.0 only once you're on the real 12 V motor (a 12 V-rated motor is
// perfectly fine run under its rated voltage from this same 7.4 V pack).
constexpr float TEST_DUTY_CAP = 0.60f;

// ---- The drive: throttle in [-1, 1]; sign = direction, magnitude = speed ----
class PaddleDrive {
public:
  void begin() {
    // 3.x LEDC: attach each pin with (freq, resolution). NO ledcSetup/channel.
    ledcAttach(PIN_DRIVE_A, PWM_FREQ, PWM_BITS);
    ledcAttach(PIN_DRIVE_B, PWM_FREQ, PWM_BITS);
    stop();
  }

  void set(float t) {
    // TODO: t is -1..+1 (sign = direction, magnitude = speed).
    // 1. constrain t to [-1, 1].
    // 2. compute a duty 0..PWM_MAX from the MAGNITUDE of t.
    // 3. if t >= 0: PWM on PIN_DRIVE_A, PIN_DRIVE_B held at 0 (forward).
    //    if t <  0: PWM on PIN_DRIVE_B, PIN_DRIVE_A held at 0 (reverse).
  }

  void stop() {                   // coast: both low
    ledcWrite(PIN_DRIVE_A, 0);
    ledcWrite(PIN_DRIVE_B, 0);
  }
};

PaddleDrive drive;

// Smoothly move `applied` toward `target` by at most `step` (the ramp).
float approach(float applied, float target, float step) {
  return applied + constrain(target - applied, -step, step);
}

void setup() {
  Console.println("\n[E2] Motor bring-up. Keep the motor loose on the bench!");
  drive.begin();
}

// Automated demo: forward ramp -> hold -> down -> reverse ramp -> hold -> down.
void loop() {
  static float applied = 0.0f;
  static float target  = 0.0f;
  static int   phase   = 0;
  static unsigned long phaseStart = 0;

  const float STEP = 0.02f;                 // ramp rate per loop tick (~ gentle)
  const unsigned long HOLD_MS = 2000;

  // choose a target for the current phase
  switch (phase) {
    case 0: target =  TEST_DUTY_CAP; break; // ramp up forward
    case 1: target =  TEST_DUTY_CAP; break; // hold forward
    case 2: target =  0.0f;          break; // ramp down
    case 3: target = -TEST_DUTY_CAP; break; // ramp up reverse
    case 4: target = -TEST_DUTY_CAP; break; // hold reverse
    case 5: target =  0.0f;          break; // ramp down
  }

  applied = approach(applied, target, STEP);
  drive.set(applied);

  // advance phases: ramps end when applied reaches target; holds are timed
  bool reached = fabs(applied - target) < 0.001f;
  if ((phase == 1 || phase == 4)) {
    if (millis() - phaseStart > HOLD_MS) { phase = (phase + 1) % 6; phaseStart = millis(); }
  } else if (reached) {
    phase = (phase + 1) % 6; phaseStart = millis();
    Console.printf("phase %d  applied=%.2f\n", phase, applied);
  }

  delay(20);   // ~50 Hz update -> STEP 0.02 gives a ~1 s full ramp
}
