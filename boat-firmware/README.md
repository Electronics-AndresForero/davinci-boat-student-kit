# boat-firmware — the real project (E1 through E5)

This folder holds **one PlatformIO project** — `template/` — that you keep reusing
from E1 all the way through E5. Each exercise doesn't get its own project; instead you
**swap in a new file** as the exercise progresses. That mirrors how the real firmware
is built: one program that grows a feature at a time, not five disconnected demos.

## Why this project looks different from `phase0-bringup/`

Phase 0 was a plain Arduino project. This one **must** be built differently, because
of the Bluetooth library we use:

Keyboards, mice, and gamepads all speak a standard Bluetooth language called **HID**.
Your ESP32 needs something that speaks HID and knows the quirks of real controllers —
that's **Bluepad32**. The stock Arduino-ESP32 core ships with Espressif's default
Bluetooth stack (Bluedroid); Bluepad32 doesn't use it — it swaps in a leaner stack
called **BTstack**, which has to sit *underneath* the Arduino layer, not alongside it.

The way Bluepad32's author solves this: run **Arduino as a component of ESP-IDF**
(Espressif's lower-level framework). You still write ordinary `setup()`/`loop()`
Arduino code — but underneath, ESP-IDF + BTstack are in charge. Practically, this
means:

- the project lives inside `template/`, which is a **git submodule** pointing at the
  official [esp-idf-arduino-bluepad32-template](https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template) —
  the reference project Bluepad32's own author maintains, already wired up correctly.
  You don't need to configure any of this yourself.
- your code lives at **`template/main/sketch.cpp`**, not `src/main.cpp`.
- you print with **`Console`, not `Serial`** — Bluepad32 owns the USB serial console
  for its own interactive console, so `Serial.print(...)` gets you garbage or nothing.
  Use `Console.printf(...)` / `Console.println(...)` everywhere in this project.

## How to work through an exercise

Each exercise lives in `exercises/EN-.../`:

```
boat-firmware/
  template/                        ← the Bluepad32 project (git submodule)
    main/sketch.cpp                ← THIS is what you edit / replace each session
  exercises/
    E1-xbox-pairing/
      sketch.cpp                   ← copy this OVER template/main/sketch.cpp
      README.md                    ← concepts, task, hints, checkpoint
    E2-drive-motor/...
    E3-servo-rudder/...
    E4-rc-integration/...
    E5-modes-arming-auto/...
```

For each session:

1. Open that exercise's `README.md` and read the concepts section first.
2. Copy its `sketch.cpp` over `template/main/sketch.cpp`:
   ```bash
   cp boat-firmware/exercises/E1-xbox-pairing/sketch.cpp boat-firmware/template/main/sketch.cpp
   ```
   (or just open the exercise's `sketch.cpp` in VS Code, select all, copy, and paste
   it over the contents of `template/main/sketch.cpp` — whichever is easier.)
3. Fill in the `TODO`s directly in `template/main/sketch.cpp`.
4. Open `template/` as the PlatformIO project (**File → Open Folder** →
   `boat-firmware/template`) and Build + Upload the `esp32dev` environment.
5. When you're done and it works, **save a copy of your finished code** back into that
   exercise's own folder (e.g. `exercises/E1-xbox-pairing/sketch.cpp`) so it's not
   lost when you copy the next exercise's skeleton over it in the next session. Later
   exercises tell you to paste in classes/functions from earlier ones — you'll need
   them.

## First-time setup for this project specifically

If you haven't done [SETUP.md](../SETUP.md) yet (VS Code, PlatformIO, the one-time
click patch), do that first — this project won't build without it.

The **first build** of `template/` downloads the ESP-IDF toolchain and a couple dozen
managed components — this takes several minutes the first time, even on a good
connection. That's normal, not a hang. Let it finish.

## Troubleshooting

Phase-specific traps live in each exercise's own README. For anything about the
Bluepad32 project setup itself, see [../TROUBLESHOOTING.md](../TROUBLESHOOTING.md) —
in particular the click/esptool patch and the Windows long-path note, both of which
matter *specifically* for this project (they don't affect `phase0-bringup/`).
