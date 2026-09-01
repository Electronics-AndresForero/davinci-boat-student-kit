# Troubleshooting — common issues

General problems that can show up in any exercise. Each exercise's own README covers
traps specific to that step (e.g. LEDC frequency limits are in E2, not here) — check
there first if the problem is about something you just wired or just coded. Come back
here for install/tooling/power problems.

---

## VS Code / PlatformIO

- **PlatformIO install hangs or fails on first run.** It downloads a Python-based
  toolchain the first time — this can take several minutes on a slow connection. Let
  it finish; don't cancel and retry, that can leave a half-installed core. If it's
  genuinely stuck (10+ minutes, no network activity), fully close VS Code and reopen.
- **No PlatformIO icon (ant/alien) in the sidebar after installing the extension.**
  Fully restart VS Code (close every window, not just "reload window") — the
  extension needs a real restart to register.
- **First Bluepad32 build fails at the very last step**, with a Python error ending in
  something like `TypeError: ParamType.get_metavar() missing 1 required positional
  argument: 'ctx'`. This is the known click/esptool version mismatch — see
  [SETUP.md](SETUP.md) step "Apply the one-time Bluepad32 patch" and run it, then
  rebuild. This is a one-time fix per computer, not per project.
- **Build succeeds but Upload fails / hangs on "Connecting…....".** The board's
  auto-reset didn't fire. **Hold the BOOT button, tap EN/RST, release BOOT**, then try
  Upload again. Also try a different USB **cable** — many phone-charging cables carry
  power only, no data lines, and look identical to ones that work.
- **Port not found (Windows).** Install the CP210x or CH340 driver — see SETUP.md
  step 7 (Windows) or 6 (Mac).
- **No serial output, or garbage characters in the monitor.** The monitor's baud rate
  doesn't match the code. Both sides must be **115200**.
- **Windows: clone appeared to work but the build says a component/file is missing.**
  This is almost always the long-path issue — go back to SETUP.md step 2 and confirm
  you ran `git config --global core.longpaths true` **before** cloning. If you cloned
  before doing that, delete the folder and re-clone after fixing it.

## Board / power

- **Nothing happens, code compiled and uploaded fine.** Work through, in order:
  1. Board's physical **ON/OFF switch** — must be ON.
  2. Input voltage **≥ 6 V** on the barrel jack (a 4×AA pack at ~4.8 V is under spec).
  3. USB connected (for logic/flashing) **and** the jack connected (for motor power) —
     both together is normal and expected, they don't conflict.
- **Board reboots or browns out the instant a motor spins.** The power supply is
  sagging under motor current — a power problem, not a code bug. A stalled motor can
  pull well over 1 A. Use a stiffer supply (the 18650 pack, not a phone charger) or a
  current-limited bench supply for testing.
- **Board resets in a loop / won't boot at all.** Something is pulling a strapping pin
  (GPIO 0, 2, 5, 12, 15) the wrong way at power-on. Disconnect anything wired to those
  pins — our pin map avoids them on purpose, so this usually means a wiring mistake.
- **Never feed battery voltage into the `3V3` or `5V` pins.** Those are regulator
  *outputs*. Doing this kills the ESP32 instantly. Battery power only goes into the
  6–16 V jack or `VIN`.

## Bluetooth / gamepad

- **Controller won't pair.** Charge it fully — a low-battery Xbox pad can fail to
  advertise. Make sure no phone/PC is already paired to it (a controller only
  actively connects to one host at a time — forget it elsewhere first).
- **Pairs but no input shows up.** Make sure the pad isn't asleep (tap any button to
  wake it).
- **Works, then randomly disconnects.** Realistic BLE range is ~10–15 m, or another
  2.4 GHz device (Wi-Fi router, microwave) is crowding the band. This is exactly the
  failure mode E5's failsafe is designed to catch safely.

## Motors / servos / wiring

- **Motor does nothing.** See the board/power checklist above first, then: is it in
  the correct port (**M0**, not M1–M3)? Does it spin when touched directly to a 3–6 V
  source (rules out a dead motor)? Full walkthrough in E2's README.
- **Servo jitters instead of holding position.** Usually a shared power rail sagging
  under motor current — servos are sensitive to supply noise. Try powering the servo
  from a separate/regulated source if it only jitters while the drive motor also runs.
- **Only one direction works on the motor.** One of its two driver GPIOs isn't getting
  PWM — recheck the pin assignment and that both leads are seated in the same port.

## When none of this fixes it

Check that exercise's own Troubleshooting section first — it covers traps specific to
that step. Then ask, and bring what you already tried, not just "it doesn't work."
