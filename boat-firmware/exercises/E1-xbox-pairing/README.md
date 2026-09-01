# E1 — Pair the Xbox controller

**Goal:** get your Xbox 1914 controller to connect to the ESP32 over Bluetooth and
stream its stick/trigger values into the console. No motors, no servo — a pure
software win, and it de-risks the scariest part of the whole project early.

**You'll end with:** the controller's Xbox light going solid (paired), and a live
readout like `L: -3  RT: 812  LT: 0  A: 0` changing as you move the sticks and squeeze
the triggers.

---

## Concepts you need first

### Bluetooth Classic (BR/EDR) vs BLE

"Bluetooth" is really **two different radios** sharing a name:

- **BR/EDR ("Bluetooth Classic")** — the original, higher-throughput link used by
  headsets, audio, and *older* game controllers.
- **BLE (Bluetooth Low Energy)** — a newer, lean protocol built for battery devices
  that send small bursts. *Newer* Xbox controllers use this.

Your **ESP32-WROOM-32E supports BOTH.** The smaller ESP32 variants (S3/C3/C6/H2) do
**BLE only**. Xbox controllers can be on either radio depending on their firmware, so
the plain ESP32 is the one chip that pairs with *any* of them.

| Your controller | Radio it uses | Notes |
|---|---|---|
| **1914** (Series X\|S) | **BLE** (firmware v5.x+) | primary; cleanest pairing |
| **1708** (One S) | BR/EDR *or* BLE, by firmware | backup; update its firmware for BLE |

### A gamepad is an "HID" device

Keyboards, mice, and gamepads all speak a standard Bluetooth language called **HID**
(Human Interface Device) — an agreed-on format for "button 3 is pressed, axis X is at
−40." Your ESP32 doesn't need Xbox-specific magic; it needs something that speaks HID
over Bluetooth and knows the quirks of real-world controllers. That something is
**Bluepad32**.

### The `Console` vs `Serial` gotcha (save yourself an hour)

Bluepad32 runs its **own interactive console on the USB serial port** by default.
That console **conflicts with Arduino's `Serial`** — if you print with `Serial` you
get garbage or nothing. Print with **`Console`** instead
(`Console.printf(...)`, `Console.println(...)`). It behaves almost exactly like
`Serial`.

### Ranges you'll normalize (feeds E4)

Bluepad32 reports controller values as integers:

| Control | API method | Raw range | Our meaning |
|---|---|---|---|
| Left stick X | `ctl->axisX()` | −511 … 512 | **steering** (−1 port … +1 starboard) |
| Right trigger (RT) | `ctl->throttle()` | 0 … 1023 | **forward** throttle |
| Left trigger (LT) | `ctl->brake()` | 0 … 1023 | **reverse** throttle |
| Face buttons | `ctl->a()`, `ctl->b()`… | 0/1 | reserved for later |

We'll later map these to a clean **−1.0 … +1.0** command. **Deadzone:** sticks never
rest at exactly 0 — they jitter by a few counts. So any value with magnitude below a
small threshold we force to zero, or the boat would creep/wander at rest.

## Set up the project (given — not the exercise)

If this is your first session on `boat-firmware/`, read
[../../README.md](../../README.md) first — it explains why this project is structured
differently from `phase0-bringup/` and how to copy this exercise's `sketch.cpp` into
`template/main/sketch.cpp`.

## Your task

Fill in the four `TODO`s in [`sketch.cpp`](sketch.cpp) (reproduced here for reference):

```cpp
#include <Arduino.h>
#include <Bluepad32.h>

ControllerPtr myController = nullptr;

void onConnected(ControllerPtr ctl) {
  // TODO 1: if we don't already have a controller, remember this one in
  // `myController`, and print its model name + vendor/product ID with Console.
  // If we already have one, print that we're ignoring this new connection.
}

void onDisconnected(ControllerPtr ctl) {
  // TODO 2: if `ctl` is the controller we're tracking, forget it
  // (set myController back to nullptr) and print that it disconnected.
}

static int deadzone(int v, int threshold) {
  // TODO 3: return 0 if the magnitude of v is below threshold, else return v.
}

void setup() {
  Console.printf("\n[E1] Bluepad32 firmware: %s\n", BP32.firmwareVersion());
  BP32.setup(&onConnected, &onDisconnected);
  BP32.forgetBluetoothKeys();     // clean slate while we're bringing this up
  BP32.enableVirtualDevice(false);
  Console.println("Ready. Put the Xbox pad in pairing mode (hold Sync).");
}

void loop() {
  bool haveData = BP32.update();

  if (haveData && myController && myController->isConnected()
      && myController->isGamepad()) {
    ControllerPtr c = myController;

    // TODO 4: read the left stick X (c->axisX()), right trigger (c->throttle()),
    // left trigger (c->brake()), and the A button (c->a()). Apply deadzone(...)
    // to the stick reading. Print all four with Console.printf, one line per tick.
  }

  delay(50);
}
```

**Guiding questions** (answer these before you code — they're what the hints below
probe):
- Why check `myController == nullptr` before storing a newly connected controller?
  What happens on the water if we don't?
- What's the raw range of `c->axisX()`? Of `c->throttle()`? (You'll need this for E4
  too — write it down.)

### Checkpoint — how to know it's working

Flash it, open the monitor, put the 1914 in pairing mode (hold Sync ~3s until the
light flashes). It should connect (light goes solid) and print a `Connected:` line.
Moving the left stick should swing your printed value roughly from about −500 to +500
through zero; squeezing each trigger should climb from 0 toward ~1023. **At rest,
your printed stick value should read exactly 0** — if it doesn't, your deadzone
threshold is too small. Write down the deadzone threshold you land on; you'll reuse
it in E4.

## Hints

<details>
<summary>Hint 1 — I don't know where to start on <code>onConnected</code></summary>

Think about what has to be true for the rest of the program to work: `loop()` reads
from `myController`. So the first job of `onConnected` is just "remember this
controller exists" — one assignment. The second job is purely informational: tell the
human it worked, using a method on `ctl` that returns its name as a string.
</details>

<details>
<summary>Hint 2 — I don't know the exact API calls</summary>

`ctl->getModelName()` returns the controller's name. `ctl->getProperties()` returns a
struct with `.vendor_id` and `.product_id`. For the deadzone function, `abs(v)` gives
you the magnitude of an int in Arduino/C++.
</details>

<details>
<summary>Hint 3 — still stuck after trying</summary>

Ask: "in `onConnected`, what's the one line that stores the controller, and what's the
one line that prints its name?" That's genuinely all TODO 1 needs beyond the
`if`/`else` shape already in the guiding comment. For TODO 4, the four read calls are
named directly in the comment above them — the only new thing is wrapping the stick
read in `deadzone(...)` and writing one `Console.printf` with four `%d` placeholders.
</details>

## Troubleshooting

Phase-specific traps only — for anything else (install issues, upload failures, port
not found) see [../../../TROUBLESHOOTING.md](../../../TROUBLESHOOTING.md).

- **Garbage or nothing prints:** you used `Serial` somewhere instead of `Console`.
- **Controller never connects:** make sure it's *this* ESP32 you paired to and not
  your PC/phone — those steal the pad. Turn Bluetooth off on nearby devices during
  bring-up. Also make sure no phone/PC is already paired to it — a controller only
  actively connects to one host.
  - **1708:** update its firmware (via the *Xbox Accessories* app). Old firmware sits
    on BR/EDR and is fussier; new firmware = BLE = easy.
- **Connects then immediately drops:** low controller battery, or too far/interfered.
  Fresh batteries, get close.
- **Reconnect is slow every time:** that's `forgetBluetoothKeys()` doing its job
  (clean slate each boot). Once pairing is reliable, comment that line out and the
  pad reconnects on its own.

## Concept check ✅

You now know the two Bluetooth radios and why your plain ESP32 matters, what HID is,
the `Console`-not-`Serial` rule, and the exact raw ranges your controller reports. You
have a live link and real numbers to normalize.

Next → `E2-drive-motor/` — leaves the controller aside for a moment and makes
something **move**.
