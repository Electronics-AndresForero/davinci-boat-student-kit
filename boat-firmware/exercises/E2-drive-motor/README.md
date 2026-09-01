# E2 — Drive the paddle motor

**Goal:** make a motor spin forward and reverse at controllable speed, driven by the
board's onboard **SS6625E** driver and the ESP32's **LEDC** hardware PWM.

**You'll end with:** your bench TT motor smoothly ramping up forward, holding, ramping
down, then doing the same in reverse, on a loop.

---

## Concepts you need first

### What an H-bridge is (and why we need one)

A motor spins one way when current flows A→B through it, and the *other* way when
current flows B→A. To reverse direction you must **reverse the current** — but a
battery's polarity is fixed. An **H-bridge** solves this with four electronic
switches arranged in an "H," the motor across the middle:

```
        +V                     +V
        │                       │
      [S1]                    [S3]
        │        motor          │
        ├────────(M)────────────┤
      [S2]                    [S4]
        │                       │
       GND                     GND

  Forward: close S1 + S4  → current flows left-to-right
  Reverse: close S3 + S2  → current flows right-to-left
```

You **never** close S1+S2 (or S3+S4) together — that shorts +V straight to GND. A
proper driver chip handles that interlock for you.

### The SS6625E is that driver — already on your board

You don't wire discrete transistors: the **SS6625E** on the Maker-ESP32-Pro *is* a
set of H-bridges. Each motor port exposes **two logic inputs** (two ESP32 GPIOs). For
port **M0** those are **GPIO 27 and GPIO 13**:

| GPIO 27 | GPIO 13 | Motor does |
|---|---|---|
| PWM | LOW (0) | **forward** at PWM% speed |
| LOW (0) | PWM | **reverse** at PWM% speed |
| HIGH | HIGH | brake (short) |
| LOW | LOW | coast |

So **speed** comes from *how hard* we PWM the active pin, and **direction** comes
from *which* pin we PWM.

### PWM, and LEDC done right

**PWM:** switch the pin on/off thousands of times a second; the fraction of each
cycle that's **on** is the **duty cycle**, and the motor responds to the *average*.
100% duty = full speed, 0% = stop.

> ⚠️ **Don't assume the driver can switch as fast as the ESP32.** The onboard
> **SS6625E has a limited input switching speed**; drive it too fast (e.g. 20 kHz) and
> its output can collapse to almost nothing — the motor just **sits still**, looking
> exactly like a wiring or power fault. Start at the vendor-proven **5 kHz** (it
> works); only raise it later if you *test* that the motor still drives at the higher
> frequency.

Our core's LEDC API is **pin-based**, not channel-based: `ledcAttach(pin, freq, bits)`
configures and attaches in one call, and `ledcWrite(pin, duty)` writes straight to
that pin — no channel numbers to track. That's the API `PaddleDrive` below uses.

### Ramping (slew limiting) — why we never snap to full

Commanding a stopped motor straight to 100% yanks a big **inrush current** and
mechanically jolts the paddle wheels and gear train. So we **ramp**: change the
applied throttle by a limited amount per loop. You'll see this ramp in the demo, and
it becomes the shared throttle-slew logic in E4.

## Wiring (given)

1. **Motor → M0.** Screw/plug the TT motor's two leads into the **M0** motor output.
   If it spins the wrong way during the test, swap the two leads.
2. **Power → barrel jack, from the 2S 18650 pack** (two cells in series, 7.4 V
   nominal, up to 8.4 V fully charged — this is our actual power source from here on,
   not just a bench-test stand-in). A current-limited bench supply around 6 V works
   too if you don't have the pack wired up yet. USB stays connected for flashing and
   the console. **ON/OFF switch must be ON.**

> **Safety:** first power-up, keep the motor **loose on the bench** (it'll hop). Hand
> near the supply switch.

## Your task

Fill in `set()` in [`sketch.cpp`](sketch.cpp) (reproduced here for reference):

```cpp
class PaddleDrive {
public:
  void begin() {
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

  void stop() { ledcWrite(PIN_DRIVE_A, 0); ledcWrite(PIN_DRIVE_B, 0); }
};
```

**Guiding questions:**
- Why must `PIN_DRIVE_B` be held at exactly 0 while `PIN_DRIVE_A` is PWMing forward —
  what goes wrong (electrically) if both pins carry PWM at once?
- `fabs(t) * PWM_MAX` — why the magnitude and not `t` directly? What would happen for
  reverse if you didn't take the absolute value?

### Checkpoint — how to know it's working

Motor loose, 2S 18650 pack connected via the jack, upload. It should **spin up forward over ~1 s, hold ~2 s,
ramp down to stop, then repeat in reverse** — smoothly, not snapping. A faint 5 kHz
whine while running is normal (that's the PWM switching); **silence with no motion**
is the bad sign. If it spins the wrong way first, that's a wiring/lead-polarity
thing, not a code bug — swap the two motor leads.

## Hints

<details>
<summary>Hint 1 — I don't know how to structure <code>set()</code></summary>

Three separate concerns, in order: clamp the input, compute one duty number from its
size, then decide which of the two pins gets that duty and which gets 0. Write it as
three small steps, not one clever line.
</details>

<details>
<summary>Hint 2 — exact functions to use</summary>

`constrain(t, -1.0f, 1.0f)` clamps. `(uint32_t)(fabs(t) * PWM_MAX)` gives the duty. An
`if (t >= 0) { ... } else { ... }` picks which pin is active.
</details>

<details>
<summary>Hint 3 — still stuck</summary>

Ask: "for forward, which pin gets `ledcWrite(pin, duty)` and which gets
`ledcWrite(pin, 0)`? For reverse, is it the same two pins with roles swapped?" If you
can answer that in words, the code is a direct translation of the answer.
</details>

## Troubleshooting

Phase-specific traps — general tooling issues are in
[../../../TROUBLESHOOTING.md](../../../TROUBLESHOOTING.md).

- **Motor does nothing, code compiles — work through these in order:**
  1. **Power switch ON.**
  2. **Input ≥ 6 V** on the jack.
  3. **PWM frequency ≤ ~5 kHz** — if you left it faster the SS6625E may output nothing.
  4. **Motor actually works?** Touch its leads straight to a 3–6 V source.
  5. **Seated in M0?** Not M1–M3.
  6. **Code running?** Add an LED blink on GPIO 33 in `loop()` to confirm.
- **Compile error on `ledcAttach`/`ledcWrite`:** you pasted 2.x `ledcSetup` code from
  an old tutorial — use only the pin-based API shown here.
- **Motor stutters or resets the board:** inrush/stall pulling too much current —
  lower `TEST_DUTY_CAP` or use a stiffer supply.
- **Only one direction works:** one of the two GPIOs isn't reaching M0.
- **Board browns out / USB disconnects when it spins:** make sure motor power is the
  **jack**, not USB 5 V.

## Notebook

Feeds the electronics wing's pin map (M0 = GPIO 27/13, verified) and the start of the
power-budget section (note the current draw you observe, even roughly).

Next → `E3-servo-rudder/`
