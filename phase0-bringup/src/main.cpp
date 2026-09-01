// ============================================================================
//  Phase 0 — board bring-up.
//  Proves: (1) we can flash the ESP32, (2) GPIO output works (LED blinks),
//          (3) serial works (counter prints). No motors, no Bluetooth yet.
// ============================================================================
#include <Arduino.h>

// GPIO 33 is a servo header on the Maker-ESP32-Pro and our future STATUS LED.
// (Chosen to avoid the strapping pins 0/2/5/12/15 — see this folder's README.)
constexpr int PIN_STATUS_LED = 33;

unsigned long counter = 0;

void setup() {
  Serial.begin(115200);              // open the USB serial link at 115200 baud
  delay(200);                        // give the monitor a moment to attach
  Serial.println("\n[Phase 0] DaVinci paddle boat — board is alive.");

  pinMode(PIN_STATUS_LED, OUTPUT);   // this GPIO will DRIVE (output), not read
}

void loop() {
  // --- blink: HIGH = LED on, LOW = LED off ---
  digitalWrite(PIN_STATUS_LED, HIGH);
  Serial.printf("tick %lu  (LED on)\n", counter++);
  delay(500);

  digitalWrite(PIN_STATUS_LED, LOW);
  delay(500);
}
