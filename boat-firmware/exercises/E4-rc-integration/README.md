# E4 — RC integration

**Goal:** put it all together — the **Xbox controller drives the paddle motor and the
rudder at the same time.** Right trigger = forward, left trigger = reverse, left
stick = steer.

**You'll end with:** boat on a stand (wheels off the ground), controller in hand,
squeezing RT spins the paddles faster, LT reverses, and the stick swings the rudder —
all live, all at once.

> **Everything from E1–E3 comes together here.** Same wiring: motor on **M0
> (27/13)**, servo on **25**, board powered by the **jack with the ON/OFF switch ON**,
> controller paired. Nothing new to wire.

---

## Concepts you need first

### "Who decides" vs "what moves"

This is the idea that makes the firmware expandable, so it's worth understanding.

You built two **actuator** objects in E2/E3 — `PaddleDrive` and `Steering` — each
exposing the *same* tiny interface: **`set(-1 … +1)`**. They don't know or care
*where* the number comes from. That's deliberate.

We put a **single data structure between the decision and the motion**:

```
   [ input: Xbox pad ]                     [ actuators ]
        │  reads sticks/triggers               ▲
        ▼                                       │ .set(-1..1)
   ┌─────────────────┐   fills    ┌──────────┐  │
   │ read controller │──────────▶ │ Command  │──┴──▶ PaddleDrive.set(throttle)
   └─────────────────┘            │ throttle │      Steering.set(steer)
                                  │ steer    │
                                  │ armed    │
                                  └──────────┘
```

A **`Command`** is just: `throttle`, `steer`, `armed`, and a timestamp. The rule:

- **Inputs only ever fill a `Command`.** They never touch a motor.
- **Actuators only ever read a `Command`.** They never look at a controller.

Why this matters: in E5, a physical switch chooses *which* source fills the
`Command` (Xbox vs. autonomous) — the actuator code **doesn't change at all**. And the
failsafe lives in one place: "lost the controller? set `armed = false`."

### Steering a *rudder* boat ≠ mixing a tank

A skid-steer/tank robot has to **mix** — it turns by driving its left and right
wheels at *different* speeds. **Our boat doesn't.** It has **one drive motor** (both
paddles on one axle) and a **separate rudder servo**. So the mapping is direct:
`throttle` → the motor, `steer` → the servo, with **no mixing math**.

### Normalizing the raw controller numbers (from E1)

Bluepad32 gives integers; we convert to the clean −1…+1 the `Command` wants:

- `throttle()` (RT) and `brake()` (LT): `0…1023` → divide by 1023. Forward minus
  reverse: **`throttle = RT/1023 − LT/1023`** → −1…+1.
- `axisX()` (left stick): `−512…511` → divide by 512 for steering.
- **Deadzone** each, using the value you measured in E1.

### One reused safety idea: the throttle ramp

We keep the **slew limiter** from E2 — the applied throttle chases the commanded
throttle by a limited step per loop, so even a slammed RT ramps the paddles up
smoothly. Steering we send straight through — a servo is already mechanically
limited and we *want* crisp rudder response.

## Your task

You're reusing your own `PaddleDrive` and `Steering` classes from E2/E3. Fill in the
two `TODO`s in [`sketch.cpp`](sketch.cpp) — and **paste your finished `PaddleDrive`
and `Steering` classes** where the comment says to (reproduced here for reference):

```cpp
struct Command {
  float throttle = 0, steer = 0;
  bool  armed = false;
  unsigned long stamp = 0;
};

// --- paste your E2 PaddleDrive and E3 Steering classes here, unchanged ---

PaddleDrive drive;
Steering    steer;

ControllerPtr pad = nullptr;
void onConnect(ControllerPtr c)    { if (!pad) pad = c; }
void onDisconnect(ControllerPtr c) { if (pad == c) pad = nullptr; }

float deadzone(float v) { return fabs(v) < DEADZONE ? 0.f : v; }

bool readController(Command& cmd) {
  if (!pad || !pad->isConnected() || !pad->isGamepad()) return false;
  // TODO 1: fill cmd.throttle (RT/1023 - LT/1023, deadzoned) and
  // cmd.steer (axisX/512, deadzoned). Set cmd.armed = true and
  // cmd.stamp = millis(). Return true.
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastLoop) / 1000.0f; lastLoop = now;

  BP32.update();
  if (!readController(cmd)) cmd.armed = false;

  // TODO 2: compute `target` (cmd.throttle * DUTY_CAP if armed, else 0),
  // ramp `applied` toward `target` by at most THROTTLE_SLEW * dt per loop
  // (reuse the constrain-based approach idea from E2), then call
  // drive.set(applied) and steer.set(cmd.armed ? cmd.steer : 0.0f).

  digitalWrite(PIN_LED, cmd.armed ? HIGH : (now / 300 % 2));
  delay(10);
}
```

**Guiding questions:**
- Why does `readController` set `cmd.armed = true` unconditionally when the pad is
  connected, but `loop()` sets `cmd.armed = false` when it *isn't*? Where does "the
  controller went out of range" actually get handled by this design?
- What would visibly go wrong on the water if you forgot the deadzone on `cmd.steer`?

### Checkpoint — how to know it's working

> ⚠️ **Both actuators are live now.** Put the boat/paddle assembly **on a stand with
> the wheels off the ground**, and keep the rudder linkage clear.

Pair the pad — LED goes solid. Squeeze RT: paddles ramp up forward, smoothly. Release:
they ramp down. LT: reverse. Push the stick left/right: rudder swings **while the
paddles keep spinning** — that simultaneity is the actual integration test. Turn the
controller off: the LED starts blinking and the motor stops (a preview of E5's
failsafe).

## Hints

<details>
<summary>Hint 1 — TODO 1 shape</summary>

Four assignments and a `return true;` — nothing more.
`cmd.throttle = deadzone(pad->throttle()/1023.0f - pad->brake()/1023.0f);` is the
pattern; steer follows the same shape with `axisX()/512.0f`.
</details>

<details>
<summary>Hint 2 — TODO 2 shape</summary>

This is the same ramp idea as E2's `approach()` helper, just inlined:
`float target = cmd.armed ? cmd.throttle * DUTY_CAP : 0.0f;` then move `applied`
toward `target` by at most `THROTTLE_SLEW * dt`, using
`constrain(target - applied, -maxStep, maxStep)` added to `applied`.
</details>

<details>
<summary>Hint 3 — still stuck</summary>

Ask: "does `steer.set()` need any ramping like the motor does, or can it be called
directly with `cmd.steer`?" (Reread the concepts section on why steering skips the
ramp.) If TODO 1 is right but nothing moves, re-check E1/E2/E3's trims are pasted in
correctly.
</details>

## Troubleshooting

Phase-specific traps — general tooling issues are in
[../../../TROUBLESHOOTING.md](../../../TROUBLESHOOTING.md).

- **Pairs but nothing moves:** re-confirm E2 basics — switch ON, the 2S 18650 pack
  connected via the jack (not just USB).
- **Motor hums but won't turn (servo fine):** current starvation — the motor now
  shares the supply with the servo + Bluetooth radio. A 1A wall charger isn't enough
  for this, which is exactly why we use the 2S 18650 pack instead — double check
  that's what's actually plugged into the jack, not a USB power bank or phone
  charger.
- **Creeps at rest:** deadzone too small — raise `DEADZONE`.
- **Rudder backwards / off-center:** re-apply your E3 trims, or flip the sign of
  `cmd.steer`.

## Notebook

This is the electronics wing's centerpiece: draw the **block diagram** (controller →
drivers → motors; battery → regulation) from what you actually built here, and record
the pin map as wired.

Next → `E5-modes-arming-auto/`
