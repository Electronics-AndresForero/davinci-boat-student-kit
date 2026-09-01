# E3 — The servo rudder

**Goal:** command the **MG90S** servo that swings the stern rudder — center it, sweep
it port↔starboard, and map a clean **−1…+1 steering value** to a physical angle. We
generate the servo signal with **LEDC** (same peripheral as the motor, very different
frequency), so we add **no new library**.

**You'll end with:** the servo horn sweeping smoothly left↔center↔right on a loop, and
a `Steering` class with a `set(-1..1)` interface — the twin of E2's `PaddleDrive`.

---

## Concepts you need first

### A hobby servo is a *position* device (closed loop)

A DC motor (E2) is an **open-loop speed** device — you tell it "how hard," not
"where." A hobby servo is the opposite: inside one small case are a **DC motor + gear
reduction + a potentiometer geared to the output shaft + a little control circuit**.
You command a **target position**; the circuit reads the pot (the *actual* position),
compares, and drives the motor until actual = target, then **holds** it there against
load. That's a **closed feedback loop**.

Consequence for us: we don't PWM a servo for "speed." We send it a **position
command**, and it does the work of getting there and staying.

### How the position command is encoded (the 50 Hz pulse)

The command is a **pulse train**: one pulse roughly every **20 ms (50 Hz)**, and the
**width of the pulse** encodes the angle — *not* the frequency, *not* the duty in the
motor sense.

```
   |‹—— ~20 ms frame (50 Hz) ——›|
   ┌──┐                         ┌──┐
   │  │                         │  │        pulse WIDTH = commanded angle
 ──┘  └─────────────────────────┘  └──
    ↕
  1.0 ms ≈ full port        1.5 ms ≈ center        2.0 ms ≈ full starboard
```

Standard band is **1.0–2.0 ms** around a **1.5 ms center**. Many servos (the MG90S
included) travel further with **~0.5–2.5 ms**, but the *exact* endpoints vary
unit-to-unit — which is why we keep a center and a range in config and **trim by
eye**.

### ⚠️ Never command past the mechanical stop

If you send a pulse that asks for an angle beyond where the rudder linkage (or the
servo's own gears) can physically go, the servo keeps driving against the stop: it
**buzzes, draws stall current, heats up, and strips gears**. Start with a conservative
range (±400 µs from center here) and only widen it after seeing where the real rudder
hits its limits.

### The math — resolution vs. microseconds

At 50 Hz the frame is 20,000 µs. We pick **16-bit** resolution (duty `0…65535`), which
is legal (`50 × 2¹⁶ = 3.3 MHz`, well under the 80 MHz limit) and gives fine timing:
each duty count is `20000 µs / 65536 ≈ 0.3 µs`. To send a pulse of `us` microseconds:

```
duty = us × 65536 / 20000
```

That's the whole secret. A `writeMicroseconds()` helper hides it.

## Wiring (given)

The MG90S has a standard 3-wire servo lead, and GPIO **25** is a servo header with
exactly those three pins — nothing to solder:

| MG90S wire | Color | Header pin |
|---|---|---|
| Signal | orange (or white) | **25** (signal) |
| Power +5 V | red | 5 V |
| Ground | brown (or black) | GND |

Servo power comes from the header's 5 V rail, which the board derives from the
barrel-jack input — so **the jack must be powered** for the servo to move; USB-only
won't reliably drive it.

> **First run:** leave the horn **not yet screwed to the rudder** so a mis-trim can't
> jam the linkage.

## Your task

Fill in the two `TODO`s in [`sketch.cpp`](sketch.cpp) (reproduced here for reference):

```cpp
class Steering {
public:
  void begin() {
    ledcAttach(PIN_STEER, SERVO_FREQ, SERVO_BITS);
    center();
  }

  void set(float s) {
    // TODO 1: clamp s to [-1, 1], then convert it to a pulse width in
    // microseconds: STEER_CENTER_US + (s scaled by STEER_RANGE_US).
    // Pass that microsecond value to writeMicroseconds().
  }

  void center() { writeMicroseconds(STEER_CENTER_US); }

private:
  void writeMicroseconds(int us) {
    // TODO 2: convert `us` (a pulse width) into a 16-bit LEDC duty value
    // using the formula above, and ledcWrite it to PIN_STEER. Watch for
    // integer overflow — cast to a wide type (uint64_t) before multiplying
    // us by 65536.
  }
};
```

**Guiding questions:**
- Why does `writeMicroseconds` need a *separate* private helper instead of putting the
  duty-conversion math directly in `set()`? (Hint: `center()` needs it too.)
- If `STEER_RANGE_US` were 800 instead of 400, what would `set(1.0)` command in
  microseconds — and is that still inside the "standard band" from the concepts
  section?

### Checkpoint — how to know it's working

Horn loose, jack powered, upload. The horn should **sweep left↔right smoothly**,
console printing the value as it goes. Once you see the sweep: **fit the horn** so it
points straight down the rudder line when the printed value passes through 0, screw
it on, then check the ends — if it buzzes or hits a hard stop before `s = ±1`,
**reduce `STEER_RANGE_US`** and re-test. Write down your final `STEER_CENTER_US` and
`STEER_RANGE_US` — you'll reuse both in E4.

## Hints

<details>
<summary>Hint 1 — the <code>set()</code> formula</summary>

You want: at `s = -1`, output `STEER_CENTER_US - STEER_RANGE_US`; at `s = 0`, output
`STEER_CENTER_US`; at `s = +1`, output `STEER_CENTER_US + STEER_RANGE_US`. That's a
single linear expression — `center + s * range` — cast to an `int`.
</details>

<details>
<summary>Hint 2 — the duty conversion</summary>

`(uint32_t)((uint64_t)us * (1UL << SERVO_BITS) / SERVO_PERIOD_US)` is the whole
conversion — it's the `duty = us × 65536 / 20000` formula, written to avoid
overflowing a 32-bit int mid-calculation.
</details>

<details>
<summary>Hint 3 — still stuck</summary>

Ask: "what two things does `set()` need to do — turn `s` into a microsecond pulse
width, then hand that off to something that already knows how to output a pulse
width?" That second something is `writeMicroseconds`, which you're also filling in —
so `set()` should end with a single call to it.
</details>

## Troubleshooting

Phase-specific traps — general tooling issues are in
[../../../TROUBLESHOOTING.md](../../../TROUBLESHOOTING.md).

- **Servo doesn't move at all:** jack not powered (5V rail dead), or signal on the
  wrong header pin.
- **Buzzing / hot / jitter at the ends:** commanding past the mechanical stop — lower
  `STEER_RANGE_US`.
- **Moves the wrong way:** flip the sign in `set()`, or rotate the horn 180° on the
  spline.
- **Center isn't straight:** adjust `STEER_CENTER_US` a few µs at a time, or re-seat
  the horn one spline tooth over.

## Notebook

Feeds the electronics wing's pin map (GPIO 25, verified) and the design wing's rudder
geometry section — your final trim values are what the physical linkage gets built
around.

Next → `E4-rc-integration/`
