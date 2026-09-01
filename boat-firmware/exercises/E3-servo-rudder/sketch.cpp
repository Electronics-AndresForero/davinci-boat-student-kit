// ============================================================================
//  E3 — MG90S rudder servo via LEDC (50 Hz), no servo library.
//  Auto-sweeps port -> center -> starboard so we can see + trim the travel.
// ============================================================================
#include <Arduino.h>
#include <Bluepad32.h>          // for Console (we're in the Bluepad32 project)

// ---- Pin + servo timing ----
constexpr int PIN_STEER       = 25;      // servo header
constexpr int SERVO_FREQ      = 50;      // 50 Hz -> 20 ms frame
constexpr int SERVO_BITS      = 16;      // duty 0..65535 ; 50*2^16 = 3.3 MHz (legal)
constexpr int SERVO_PERIOD_US = 20000;   // one frame in microseconds

// Trim these to YOUR servo/rudder (start conservative):
constexpr int STEER_CENTER_US = 1500;    // straight ahead
constexpr int STEER_RANGE_US  = 400;     // +/- travel from center

class Steering {
public:
  void begin() {
    ledcAttach(PIN_STEER, SERVO_FREQ, SERVO_BITS);   // 3.x API, pin-based
    center();
  }

  // s in [-1, 1] : -1 = full port, 0 = center, +1 = full starboard
  void set(float s) {
    // TODO 1: clamp s to [-1, 1], then convert it to a pulse width in
    // microseconds: STEER_CENTER_US + (s scaled by STEER_RANGE_US).
    // Pass that microsecond value to writeMicroseconds().
  }

  void center() { writeMicroseconds(STEER_CENTER_US); }

private:
  // Convert a servo pulse width (us) to a 16-bit LEDC duty and output it.
  void writeMicroseconds(int us) {
    // TODO 2: convert `us` (a pulse width) into a 16-bit LEDC duty value
    // using the formula in this exercise's README, and ledcWrite it
    // to PIN_STEER. Watch for integer overflow — cast to a wide type
    // (uint64_t) before multiplying us by 65536.
  }
};

Steering steer;

void setup() {
  Console.println("\n[E3] Servo rudder. Horn NOT on the rudder yet!");
  steer.begin();
  delay(500);              // let it reach center before we start moving
}

// Sweep -1 -> +1 -> -1 slowly so the motion is easy to watch and trim.
void loop() {
  static float s = 0.0f;
  static float dir = +1.0f;

  s += dir * 0.02f;                       // step
  if (s >=  1.0f) { s =  1.0f; dir = -1.0f; }
  if (s <= -1.0f) { s = -1.0f; dir = +1.0f; }

  steer.set(s);
  Console.printf("steer=%+.2f  (%d us)\n", s, STEER_CENTER_US + (int)(s * STEER_RANGE_US));
  delay(40);                              // ~ smooth sweep
}
