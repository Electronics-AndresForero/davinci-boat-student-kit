# E5 — Modes, arming and autonomous mode

**This session covers two concepts — budget your hour:** ~25 min switches/failsafe,
~25 min the autonomous planner, ~10 min verify.

**Goal:** add the safety layer (mode switch, arm switch, link-loss failsafe), then
fill it with a real autonomous run — flip the switch and the boat drives itself for a
set time, then stops. This satisfies the challenge's **physical-switch autonomous
mode** requirement.

**You'll end with:** the arm switch instantly kills the motor no matter what; turning
the controller off stops the boat within half a second; and flipping to AUTO makes
the boat ramp to cruise, run straight for ~15 s, and stop on its own.

---

## Concepts you need first

### Reading a switch safely

An input pin with nothing connected *floats* — reads random noise.
**`INPUT_PULLUP`** fixes this: the pin idles HIGH, and wiring the switch between the
pin and **GND** makes a closed switch read **LOW**. On this board, wire switches to a
header's **signal pin and GND only — never its 5V pin** (5V into a 3.3V GPIO destroys
it).

### Arming is a hardware gate, independent of software

When the arm switch is off, the motor must go neutral **no matter what any code or
controller says**. You arm last, disarm first.

### Failsafe: assume the link will drop

Every `Command` already carries a `stamp` (timestamp). Each loop, if the newest
command is older than `FAILSAFE_MS`, force `armed = false`. One check catches both
"controller off" and "out of range."

### Autonomous v0 is open-loop and deliberately dumb

Command actions blindly on a timer — "go forward at cruise for N seconds" — with no
sensor feedback. It can't chase a bad reading into a wall, and it always stops
itself. It *will* drift off a straight line without a compass/IMU — that's an honest,
expected limitation, not a bug.

### The planner is just another `Command` source

Exactly like the pad was in E4. The actuators, ramp, arm switch, and failsafe don't
change at all — that's the payoff of the architecture you've been building since E4.

## Wiring (given)

Two SPST switches, each between a signal pin and that header's GND: **mode switch on
GPIO 26** (closed/LOW = AUTO, open/HIGH = RC), **arm switch on GPIO 32**
(closed/LOW = ARMED, open/HIGH = disarmed).

```
   GPIO26 ──● ╱ ●── GND     (mode: closed = AUTO)
   GPIO32 ──● ╱ ●── GND     (arm:  closed = ARMED)
   ESP32 internal pull-ups hold both HIGH when open — no resistors needed.
```

## Your task

You're extending your E4 code. Paste in your `PaddleDrive`, `Steering`, `Command`,
`onConnect`/`onDisconnect`, `deadzone()`, and `readController` from E4 unchanged.
Then fill in the three `TODO`s in [`sketch.cpp`](sketch.cpp):

```cpp
Mode readMode() {
  // TODO 1: return MODE_AUTO if the mode switch reads LOW, else MODE_RC.
}

void applyFailsafe(Command& c) {
  // TODO 2: force c.armed = false if EITHER the newest command is older
  // than FAILSAFE_MS, OR the arm switch reads HIGH (open = disarmed).
}

class AutoPlanner {
public:
  void arm()    { t0 = millis(); running = true; }
  void disarm() { running = false; }

  void poll(Command& c) {
    // TODO 3: compute elapsed = millis() - t0. If `running` AND elapsed <
    // AUTO_RUN_MS: set c.throttle = AUTO_CRUISE, c.steer = 0, c.armed = true.
    // Otherwise: c.throttle = 0, c.armed = false. Either way, c.stamp = millis().
  }
private:
  unsigned long t0 = 0; bool running = false;
};
```

**Guiding questions:**
- `applyFailsafe` has two independent reasons to disarm. Why "independent" — what
  real failure does each one catch that the other wouldn't?
- Why does `AutoPlanner::poll()` set `c.stamp = millis()` in **both** branches
  (running and finished), not just the running one? What would `applyFailsafe` do if
  it didn't?

### Checkpoint — how to know it's working

Boat on a stand, wheels off the ground. **Arm gate:** mode on RC, pad paired, squeeze
RT with arm switch **off** → nothing spins, LED blinking. Flip arm **on** → LED solid,
RT now drives. **Failsafe:** while driving, turn the controller off → motor stops
within ~½ second. **Autonomous:** flip to AUTO → paddles ramp to cruise on their own,
run ~15 s, stop; LED speeds up its blink when idle in AUTO. **Instant disarm:** flip
arm off mid-run → immediate stop, from either mode.

## Hints

<details>
<summary>Hint 1 — TODO 1 and TODO 2 shape</summary>

`readMode` is one line: a ternary or if/else comparing `digitalRead(PIN_MODE_SWITCH)`
to `LOW`. `applyFailsafe` computes two booleans (`stale`, `armOff`) and sets
`c.armed = false` if either is true — it never sets `c.armed = true`, only ever takes
it away.
</details>

<details>
<summary>Hint 2 — TODO 3 shape</summary>

This mirrors E2's ramp phases in spirit: one condition
(`running && elapsed < AUTO_RUN_MS`) decides which of two command sets to write.
Write the "go" case first, then the "stop" case — both end by stamping `millis()`.
</details>

<details>
<summary>Hint 3 — still stuck</summary>

If arming/failsafe work but AUTO does nothing: is `planner.arm()` actually being
called on the mode-switch edge? Add a `Console.printf` inside `arm()` to confirm it
fires. If AUTO runs but never stops: print `elapsed` each loop and check it's
actually counting up past `AUTO_RUN_MS`.
</details>

## Troubleshooting

Phase-specific traps — general tooling issues are in
[../../../TROUBLESHOOTING.md](../../../TROUBLESHOOTING.md).

- **Switch reads random / does nothing:** not set to `INPUT_PULLUP`, or wired to the
  header's 5V pin instead of GND.
- **Logic feels inverted:** with `INPUT_PULLUP`, closed = LOW — double check your
  `==LOW` vs `==HIGH` comparisons match the physical wiring.
- **Motor won't stop on controller-off:** `FAILSAFE_MS` too large, or `stamp` isn't
  being refreshed on every fresh command.
- **Boat veers instead of straight in AUTO:** expected — that's open-loop drift, a
  feedback problem, not a bug (out of scope for this challenge cycle).

## Notebook

This session produces the firmware section's core: modes implemented, parameters
tuned (deadzone, slew, cruise throttle, failsafe timeout), and the physical-switch
requirement the rubric explicitly scores. Note any known issues honestly — an
undocumented bug scores worse than a documented one.

## Where this leaves you

By the end of E5 the electronics are functionally complete: the boat drives manually,
drives itself, fails safe, and can run untethered. Everything past here is the
mechanical build — and, only if lake day shows Bluetooth's range isn't enough,
`E7-espnow-backup/` (see the repo root).
