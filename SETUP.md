# Setup — from a blank computer to your first flash

Do this **once per computer**, before `phase0-bringup/`. It's long on purpose — every
step is here so nothing is assumed. Pick your OS below.

If anything doesn't match what you see on screen (installers change over time), that's
normal — the goal at each step is stated, so you can recognize when you've reached it
even if a button is in a slightly different place.

---

## Windows

### 1. Install Git

Git is the tool that downloads (and keeps updated) the code in this repo.

1. Go to <https://git-scm.com/download/win> — the download starts automatically.
2. Run the installer. On every screen, the **defaults are fine** — just click **Next**
   until **Install**, then **Finish**.
3. Open **PowerShell** (Start menu → type "PowerShell" → Enter) and check it worked:
   ```powershell
   git --version
   ```
   You should see something like `git version 2.xx.x.windows.1`. If you see
   `'git' is not recognized...`, close and reopen PowerShell (it needs a fresh window
   to pick up the install) and try again.

### 2. Turn on long file path support (do this now, before cloning anything)

Some of the code we use (the Bluetooth library) has very deeply nested folders and
long file names. Windows has historically limited file paths to 260 characters, which
can silently break a clone partway through — you'd end up with folders that *look*
right but are missing files, and confusing build errors much later. Turn the limit off
now, once, so it's never a problem:

```powershell
git config --global core.longpaths true
```

*(Optional, extra safety — needs an admin PowerShell: Start menu → type PowerShell →
right-click → "Run as administrator" → then:)*
```powershell
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" `
  -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
```
This one needs a restart to take effect; the `git config` line above is the one that
matters most and doesn't need a restart.

### 3. Install VS Code

1. Go to <https://code.visualstudio.com/> and download for Windows.
2. Run the installer — defaults are fine.
3. Open VS Code.

### 4. Install the PlatformIO extension

PlatformIO is what actually builds and flashes the firmware — it replaces the Arduino
IDE with something more reliable for this project.

1. In VS Code, click the **Extensions** icon in the left sidebar (four squares), or
   press `Ctrl+Shift+X`.
2. Search **"PlatformIO IDE"**.
3. Click **Install** on the one published by **PlatformIO**.
4. **Wait.** It downloads a whole Python-based toolchain in the background — this can
   take several minutes on a slow connection. Let it finish; don't close VS Code.
5. When it's done, **fully restart VS Code** (close every window, reopen it) — the
   extension needs a real restart to register. You should now see a little
   **ant/alien icon** in the left sidebar. That's PlatformIO.

### 5. Apply the one-time Bluepad32 patch

**Every fresh PlatformIO install** hits a known first-build failure with the Bluetooth
library we use (a version mismatch between two of its dependencies). This isn't your
code — it's a one-line fix you apply once, right now, before it ever bites you:

```powershell
%USERPROFILE%\.platformio\penv\Scripts\pip.exe install "click==8.1.8"
```

If that says the path doesn't exist, PlatformIO's first-run setup (step 4.4) hasn't
finished yet — wait for it, then retry.

### 6. Choose where this project lives, and clone it

Pick a **short** folder path — avoid deeply nested cloud-synced folders (like a
company OneDrive-redirected `Documents`) if you can, since that eats into the path
length you just fixed above. `C:\Electronics\` is a good, safe choice.

```powershell
cd C:\
mkdir Electronics
cd Electronics
git clone --recursive https://github.com/Electronics-AndresForero/davinci-boat-student-kit.git
```

**`--recursive` matters** — this project bundles the Bluetooth library as a
sub-project, and that flag fetches it too. If you forget it, run this from inside the
cloned folder to fix it after the fact:
```powershell
git submodule update --init --recursive
```

This first clone downloads a fair amount of data (the Bluetooth library is large) —
give it a few minutes on a normal connection.

### 7. Install a USB-serial driver (if your board isn't detected)

Plug the board in with a USB cable — **make sure it's a data cable, not a
charge-only one** (many phone-charging cables have no data lines and this is the #1
reason "nothing happens"). If Windows doesn't show a new COM port for it:

- Most Maker-ESP32-Pro boards use a **CP2102** USB-to-serial chip → install the
  [CP210x driver from Silicon Labs](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers).
- Some use a **CH340** chip instead → search "CH340 driver Windows" if CP210x doesn't
  fix it.

### 8. Open the project and do your first build

1. In VS Code: **File → Open Folder** → select
   `C:\Electronics\davinci-boat-student-kit\phase0-bringup`.
2. Plug the board in via USB.
3. Look at the **blue status bar at the very bottom of VS Code** — you'll see small
   icons: a **checkmark** (Build), a **right arrow** (Upload), a **plug** (Serial
   Monitor).
4. Click the **checkmark**. First build takes a while (it downloads the ESP32
   toolchain) — watch the terminal panel that opens; you want it to end in
   `[SUCCESS]`.
5. Click the **right arrow** to flash it to the board.
6. Click the **plug** to open the Serial Monitor and see it running.

If step 4 or 5 fails, check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) before assuming
it's your fault — most first-time failures are one of the known issues listed there.

---

## Mac

### 1. Install the Xcode Command Line Tools

This gives you `git` and other basic developer tools.

1. Open **Terminal** (⌘+Space, type "Terminal", Enter).
2. Run:
   ```bash
   xcode-select --install
   ```
3. A popup appears — click **Install**, accept the license, wait for it to finish
   (a few minutes).
4. Check it worked:
   ```bash
   git --version
   ```
   You should see something like `git version 2.xx.x (Apple Git-xxx)`.

### 2. Install VS Code

1. Go to <https://code.visualstudio.com/> and download for Mac.
2. Open the downloaded `.zip`, drag **Visual Studio Code** into **Applications**.
3. Open it from Applications (first time, macOS may ask you to confirm you trust it —
   allow it).

### 3. Install the PlatformIO extension

1. In VS Code, click the **Extensions** icon in the left sidebar (four squares), or
   press `⌘+Shift+X`.
2. Search **"PlatformIO IDE"**.
3. Click **Install** on the one published by **PlatformIO**.
4. **Wait** for its first-run setup to finish (downloads a Python-based toolchain —
   can take several minutes).
5. **Fully quit and reopen VS Code** (⌘+Q, then reopen) — you should now see an
   **ant/alien icon** in the left sidebar.

### 4. Apply the one-time Bluepad32 patch

Same known issue as Windows — a version mismatch in one of the Bluetooth library's
dependencies that hits every fresh install. Fix it once, now:

```bash
~/.platformio/penv/bin/pip install "click==8.1.8"
```

If that path doesn't exist yet, PlatformIO's first-run setup (step 3.4) hasn't
finished — wait for it, then retry.

### 5. Clone the project

```bash
cd ~/Documents
mkdir -p Electronics && cd Electronics
git clone --recursive https://github.com/Electronics-AndresForero/davinci-boat-student-kit.git
```

**`--recursive` matters** — it pulls in the Bluetooth library, which is bundled as a
sub-project. If you forget it:
```bash
cd davinci-boat-student-kit
git submodule update --init --recursive
```

This downloads a fair amount of data — give it a few minutes.

### 6. USB driver (usually not needed, but if the board isn't detected)

Plug the board in with a **data-capable** USB cable (not charge-only). Modern macOS
usually recognizes CP2102-based boards without any extra driver. If nothing shows up
in PlatformIO's port list:

- Install the [CP210x VCP driver from Silicon Labs](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers).
- On Apple Silicon Macs, macOS may block the driver the first time — go to
  **System Settings → Privacy & Security**, scroll down, and click **Allow** next to
  the blocked system extension, then restart.

### 7. Open the project and do your first build

1. In VS Code: **File → Open Folder** → select
   `~/Documents/Electronics/davinci-boat-student-kit/phase0-bringup`.
2. Plug the board in.
3. In the **blue status bar at the bottom of VS Code**: click the **checkmark** icon
   to Build (first build takes a while — downloads the ESP32 toolchain).
4. Click the **right arrow** icon to Upload.
5. Click the **plug** icon to open the Serial Monitor.

If Build or Upload fails, check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) first.

---

## You're set up when...

`phase0-bringup` builds, uploads, an LED blinks on your board, and the serial monitor
prints a counter. That's it — move on to `phase0-bringup/README.md` if you haven't
already read it, then start on E1.
