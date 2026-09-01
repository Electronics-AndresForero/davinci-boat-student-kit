# handheld — the ESP-NOW transmitter

Same project style as `boat-firmware/` at the repo root: `template/` is a **git
submodule** of the official Bluepad32 template (this board still needs Bluetooth, to
read the Xbox pad), and your code goes in `template/main/sketch.cpp`.

## How to work through this

1. Read [`exercises/E7-handheld/README.md`](exercises/E7-handheld/README.md) for the
   concepts and task.
2. First, get the boat's MAC address — see `../get-boat-mac/`.
3. Copy `exercises/E7-handheld/sketch.cpp` over `template/main/sketch.cpp`, paste in
   the boat's MAC, and fill in the `TODO`.
4. Open `template/` as the PlatformIO project and Build + Upload the `esp32dev`
   environment — to your **second** ESP32 (the handheld), not the boat.

If you haven't done the repo root's [SETUP.md](../../SETUP.md) yet (VS Code,
PlatformIO, the one-time click patch), do that first.
