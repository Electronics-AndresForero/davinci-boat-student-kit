// ============================================================================
//  E1 — pair an Xbox controller and stream its values.
//  Prints via Console (NOT Serial — Bluepad32 owns the serial console).
//  No motors/servo yet: this only proves the Bluetooth link + shows raw ranges.
// ============================================================================
#include <Arduino.h>
#include <Bluepad32.h>

// We only ever use ONE controller on this boat. Keep a single pointer to it.
ControllerPtr myController = nullptr;

// --- Called by Bluepad32 when a controller connects ---
void onConnected(ControllerPtr ctl) {
  // TODO 1: if we don't already have a controller, remember this one in
  // `myController`, and print its model name + vendor/product ID with Console.
  // If we already have one, print that we're ignoring this new connection.
}

// --- Called when it disconnects (out of range, powered off) ---
void onDisconnected(ControllerPtr ctl) {
  // TODO 2: if `ctl` is the controller we're tracking, forget it
  // (set myController back to nullptr) and print that it disconnected.
}

// --- Deadzone helper: raw stick counts -> zero if near center ---
static int deadzone(int v, int threshold) {
  // TODO 3: return 0 if the magnitude of v is below threshold, else return v.
}

void setup() {
  Console.printf("\n[E1] Bluepad32 firmware: %s\n", BP32.firmwareVersion());

  // Register our two callbacks.
  BP32.setup(&onConnected, &onDisconnected);

  // Start clean during bring-up: forget any controller we paired before.
  // (Comment this out later so the boat re-pairs instantly without re-pressing sync.)
  BP32.forgetBluetoothKeys();

  // We don't emulate a mouse/keyboard — gamepads only.
  BP32.enableVirtualDevice(false);

  Console.println("Ready. Put the Xbox pad in pairing mode (hold the Sync button).");
}

void loop() {
  // Pump the Bluetooth stack. Returns true if fresh controller data arrived.
  bool haveData = BP32.update();

  if (haveData && myController && myController->isConnected()
      && myController->isGamepad()) {
    ControllerPtr c = myController;

    // TODO 4: read the left stick X (c->axisX()), right trigger (c->throttle()),
    // left trigger (c->brake()), and the A button (c->a()). Apply deadzone(...)
    // to the stick reading. Print all four with Console.printf, one line per tick.
  }

  delay(50);   // ~20 Hz print rate — plenty for eyeballing values
}
