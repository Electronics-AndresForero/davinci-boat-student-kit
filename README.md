# DaVinci Paddle Boat — Electronics Student Kit

Everything the **electronics pair** needs to build the firmware for the da Vinci
paddle boat (EIA Challenge): from a bare ESP32 board that's never been flashed, to a
boat you drive with an Xbox controller and switch into an autonomous run.

**No prior ESP32 experience assumed.** If you've never used git, VS Code, or flashed a
microcontroller before, start at [SETUP.md](SETUP.md) — it walks through installing
every tool from zero, on both Mac and Windows.

**This repo is scoped to firmware/programming only** — no hull design, no BOM
sourcing, no fabrication files. Clone it on its own and it's everything a
student pair needs. It's also used as a git submodule inside the broader
`EIA_boat_2027` project repo (hull design, drivetrain CAD, etc.), which
references this repo alongside the mechanical work — but you don't need that
repo to do any of the exercises below.

## Repo structure: the `boat-firmware/auto_mode_xbox_controller` submodule

`boat-firmware/auto_mode_xbox_controller/` is itself a **git submodule**, pinned to
[Ricardo Quesada's ESP-IDF + Arduino + Bluepad32 template](https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template)
— the build scaffolding (PlatformIO project, Bluepad32 component, examples)
that every exercise builds on top of.

**After cloning this repo, always run:**

```
git submodule update --init --recursive
```

(or clone with `git clone --recursive <this-repo-url>` in the first place) —
otherwise `boat-firmware/auto_mode_xbox_controller/` sits empty.

**Before editing any file inside `boat-firmware/auto_mode_xbox_controller/`** (most commonly
`main/sketch.cpp`, once you're integrating exercises into the real boat's
firmware), check out the custom branch first:

```
cd boat-firmware/auto_mode_xbox_controller
git checkout boat-firmware-custom
```

Skipping this leaves you on a **detached HEAD** — your edits still work locally,
but they're not on any branch, so git can lose them (e.g. on garbage collection)
the moment you check out something else. The submodule's `origin` remote points
at the upstream template (someone else's public repo) — there's nowhere to push
your own firmware changes to, so `boat-firmware-custom` only ever exists
locally, as a safe place to commit. After committing there, go back to this
repo's root and record the update:

```
cd ../..                      # back to davinci-boat-student-kit/
git add boat-firmware/auto_mode_xbox_controller
git commit -m "..."
```

That second commit is what makes this repo "remember" which exact state of the
template your firmware changes are pinned to.

## Do this first

1. **[SETUP.md](SETUP.md)** — install VS Code, PlatformIO, and the one-time patch every
   fresh machine needs. Do this once per computer, before anything else.
2. **`phase0-bringup/`** — flash your first program (an LED blink), prove the board
   works, learn what the pins mean. No Bluetooth, no motors — a warm-up.
3. **`boat-firmware/`** — the real project. One PlatformIO project you keep reusing
   session after session, swapping in a new `exercises/EN-.../sketch.cpp` each time.

## The path (one session ≈ one exercise, ~1 hour each)

| # | Exercise | You'll end with |
|---|---|---|
| 0 | `phase0-bringup/` | LED blinking, serial counter printing — board confirmed alive |
| 1 | `boat-firmware/exercises/E1-xbox-pairing/` | Xbox controller paired over Bluetooth, stick/trigger values streaming to console |
| 2 | `boat-firmware/exercises/E2-drive-motor/` | Paddle motor ramping forward/reverse on an automated test sequence |
| 3 | `boat-firmware/exercises/E3-servo-rudder/` | Rudder servo sweeping port↔center↔starboard, trimmed |
| 4 | `boat-firmware/exercises/E4-rc-integration/` | Controller drives motor **and** rudder together, live |
| 5 | `boat-firmware/exercises/E5-modes-arming-auto/` | Mode switch, arm switch, link-loss failsafe, and a real autonomous timed run |
| 7 | `E7-espnow-backup/` | *(optional — only if Bluetooth's ~10–15 m range isn't enough on lake day)* a handheld relay extends control to ~100–200 m |

Each exercise folder has its own `README.md` with the concepts you need, the wiring,
your task (code with `TODO`s to fill in), guiding questions, a checkpoint to know
you're done, hints if you're stuck, and troubleshooting.

**Don't peek at solutions before trying.** These are real exercises — the skeleton
code has gaps on purpose. If you're stuck for more than ~15 minutes, open the hints
in that exercise's README before asking for the answer outright.

## The board (the one fact everything depends on)

**Maker-ESP32-Pro** — an ESP32-WROOM-32E with an onboard SS6625E motor driver.

| Function | GPIO | Notes |
|---|---|---|
| Paddle drive motor | **27 + 13** | Motor port M0 (SS6625E), PWM **5 kHz** |
| Rudder servo (MG90S) | **25** | Servo header, 50 Hz |
| Mode switch (RC/AUTO) | **26** | `INPUT_PULLUP`, switch to GND |
| Arm switch | **32** | `INPUT_PULLUP`, switch to GND |
| Status LED | **33** | External LED + 330 Ω resistor |

These pins deliberately avoid the ESP32's **strapping pins** (0, 2, 5, 12, 15), which
control boot behavior — `phase0-bringup/` explains why.

## Hardware checklist

| Part | Qty | Role |
|---|---|---|
| Maker-ESP32-Pro board | 1 | brains + onboard motor driver |
| Xbox Wireless Controller 1914 (Series X\|S) | 1 | primary controller (BLE) |
| Xbox One S controller 1708 | 1 | backup (update its firmware for BLE) |
| Yellow TT motor (3–6 V) | 1 | **bench-test** motor for E2/E4/E5 |
| JGA25-370 6 V gearmotor (170 RPM, 0.6 kg·cm) | 1 | the real paddle drive motor (final boat) |
| MG90S servo | 1 | rudder |
| 2S 18650 pack + holder (7.4 V nominal, up to 8.4 V full charge) | 1 | propulsion power |
| SPDT/toggle switches | 2 | mode + arm |
| LED + 330 Ω resistor | 1 | status indicator |
| USB-C/micro cable (data, not charge-only) | 1 | flashing + serial |

> ⚠️ **The real drive motor is 6 V-rated, same as the bench TT motor** — both are well
> under our 2S 18650 pack's 7.4–8.4 V. Every exercise's `DUTY_CAP`/`TEST_DUTY_CAP`
> (0.60) is a **permanent** safety cap for that reason, not just a bench-testing
> stand-in to be raised later. Never set it toward 1.0 on this pack.

## Safety rules (read once, keep in mind every session)

- **E2 onward: keep the motor loose on the bench** until it's actually mounted — it
  can hop.
- **E4 onward: put the boat/paddle assembly on a stand, wheels off the ground**, before
  powering up — both actuators are live.
- **Never feed battery voltage into the `3V3` or `5V` pins** — those are regulator
  *outputs*. Doing this kills the ESP32 instantly. Battery power only goes into the
  6–16 V barrel jack or `VIN`.
- **The board's ON/OFF switch must be ON** for the motor rail to work — USB alone
  doesn't power motors/servos.
- If anything smells hot, smokes, or a switch does the opposite of what you expect —
  **stop and ask**, don't guess.

## Troubleshooting

Read [TROUBLESHOOTING.md](TROUBLESHOOTING.md) first for anything not specific to one
exercise (install issues, upload failures, port not found, power problems). Each
exercise's own README covers traps specific to that step.

## Where this leaves you

By E5 the electronics are functionally complete: the boat drives manually, drives
itself, fails safe, and can run untethered on battery power. Everything past that is
the mechanical build (mounting, waterproofing, water trials) and, only if Bluetooth's
range proves too short on lake day, E7's long-range relay.
