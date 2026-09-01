# Setup — from a blank computer to your first flash

Do this **once per computer**, before `phase0-bringup/`. It's long on purpose — every
step says not just *what* to do but **where** to do it (which window, which app) and
**how** (exactly what to click or type). Pick your OS below.

If anything doesn't match what you see on screen (installers change over time), that's
normal — the goal at each step is stated, so you can recognize when you've reached it
even if a button is in a slightly different place.

---

## Windows

You'll spend this whole section moving between **two places**: your web browser (to
download things) and **PowerShell** (a text-command window, for typing commands).
Every step below says which one to use.

### 1. Install Git

Git is the tool that downloads (and keeps updated) the code in this repo.

1. **In your web browser**, go to <https://git-scm.com/download/win> — the download
   starts automatically and lands in your **Downloads** folder.
2. **Run the installer** you just downloaded: most browsers show a small download
   notification (usually at the top-right or bottom of the window) — click it to run
   the file directly. If you don't see that, open **File Explorer → Downloads** and
   double-click the file (named something like `Git-2.xx.x-64-bit.exe`).
3. On every installer screen, the **defaults are fine** — just click **Next** until
   **Install**, then **Finish**.
4. **Open PowerShell** — click the **Start menu** (Windows icon, bottom-left of the
   screen), type `PowerShell`, and press **Enter**. A dark/blue text window opens —
   this is where you'll type commands for the rest of Windows setup. **Keep this
   window open**; you'll come back to it in later steps (if you do close it, reopen it
   the same way: Start menu → type `PowerShell` → Enter).
5. **In that PowerShell window**, type this and press Enter, to check the install
   worked:
   ```powershell
   git --version
   ```
   You should see something like `git version 2.xx.x.windows.1` printed back. If you
   see `'git' is not recognized...` instead, close this PowerShell window, open a new
   one (Start menu → `PowerShell` → Enter — a fresh window is needed to pick up the
   install), and try again.

### 2. Turn on long file path support (do this now, before cloning anything)

Some of the code we use (the Bluetooth library) has very deeply nested folders and
long file names. Windows has historically limited file paths to 260 characters, which
can silently break a clone partway through — you'd end up with folders that *look*
right but are missing files, and confusing build errors much later. Turn the limit off
now, once, so it's never a problem.

**In the same PowerShell window** from step 1, type:
```powershell
git config --global core.longpaths true
```

*(Optional, extra safety — this one needs an **administrator** PowerShell window,
which is a different, separate window from your normal one: Start menu → type
`PowerShell` → but this time **right-click** the result and choose **"Run as
administrator"** → click **Yes** on the permission prompt → then, in that new admin
window, type:)*
```powershell
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" `
  -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
```
This one needs a restart to take effect; the `git config` line above (in your normal,
non-administrator PowerShell) is the one that matters most and doesn't need a restart
— you can skip the administrator step if you want to keep moving.

### 3. Install VS Code

VS Code is the editor you'll write and build all your code in.

1. **In your web browser**, go to <https://code.visualstudio.com/> — it should offer
   the Windows download automatically; if not, click the Windows button. It lands in
   your **Downloads** folder.
2. **Run the installer** the same way as Git in step 1 (click the browser's download
   notification, or open **File Explorer → Downloads** and double-click it).
3. On every installer screen, the defaults are fine — **Next** through to **Install**,
   then **Finish**. It should open VS Code automatically when done; if not, open it
   from the **Start menu** (type `Visual Studio Code` → Enter).

### 4. Install the PlatformIO extension

PlatformIO is what actually builds and flashes the firmware — it replaces the Arduino
IDE with something more reliable for this project. This step happens **inside VS
Code**, not in PowerShell or the browser.

1. **In VS Code**, click the **Extensions** icon in the left-hand sidebar (it looks
   like four small squares, one detached), or press `Ctrl+Shift+X`.
2. In the search box that appears at the top, type **"PlatformIO IDE"**.
3. Click the blue **Install** button on the result published by **PlatformIO**.
4. **Wait.** It downloads a whole Python-based toolchain in the background — this can
   take several minutes on a slow connection. You'll see progress in the bottom-right
   corner of VS Code. Let it finish; don't close VS Code.
5. When it's done, **fully close and reopen VS Code** (close every VS Code window,
   then reopen the app) — the extension needs a real restart to register. You should
   now see a little **ant/alien icon** in the left sidebar. That's PlatformIO.

### 5. Apply the one-time Bluepad32 patch

**Every fresh PlatformIO install** hits a known first-build failure with the Bluetooth
library we use (a version mismatch between two of its dependencies). This isn't your
code — it's a one-line fix you apply once, right now, before it ever bites you.

**Where to type this:** back in your **PowerShell window** (reopen it if it's closed:
Start menu → type `PowerShell` → Enter).

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pip.exe" install "click==8.1.8"
```

> **What is `$env:USERPROFILE`?** It's a placeholder PowerShell fills in automatically
> for **your personal Windows folder** — normally `C:\Users\<your Windows username>`
> (e.g. `C:\Users\andres.forero`). You never need to type your actual username or
> figure out what it is; PowerShell substitutes it for you the moment you press Enter.
> The whole command just means "run the `pip.exe` program that lives inside the
> PlatformIO folder PlatformIO already created for you, and tell it to install a
> specific version of `click`."

If that says something like "the system cannot find the path specified," PlatformIO's
first-run setup (step 4.4 above) hasn't finished yet — go back, wait for it, then
retry this command.

### 6. Choose where this project lives, and clone it

**Where to type this:** the same **PowerShell window** as before.

First, pick a **short** folder path — avoid deeply nested cloud-synced folders (like a
company OneDrive-redirected `Documents`) if you can, since that eats into the path
length you just fixed in step 2. `C:\Electronics\` is a good, safe choice — that's
what the commands below create and use.

```powershell
cd C:\
mkdir Electronics
cd Electronics
git clone --recursive --shallow-submodules https://github.com/Electronics-AndresForero/davinci-boat-student-kit.git
```

*(What these do, line by line: `cd C:\` moves you to the root of your `C:` drive;
`mkdir Electronics` creates a new folder called `Electronics` right there; the second
`cd Electronics` moves into that new folder; `git clone ...` downloads this whole
project into it, creating `C:\Electronics\davinci-boat-student-kit\`.)*

**`--recursive` matters** — this project bundles the Bluetooth library as a
sub-project, and that flag fetches it too. **`--shallow-submodules` matters too** —
without it, git fetches that sub-project's *entire history* (a very slow, very large
download); with it, you only get the current snapshot, which is all you need. If you
forget either flag, run this **from inside the cloned folder**
(`C:\Electronics\davinci-boat-student-kit`, in the same PowerShell window) to fix it
after the fact:
```powershell
git submodule update --init --recursive --depth 1
```

This first clone downloads a fair amount of data (the Bluetooth library is large) —
give it a few minutes on a normal connection. The PowerShell window will look "stuck"
with no new text while it works — that's normal, not a freeze.

### 7. Install a USB-serial driver (if your board isn't detected)

Plug the board into a USB port on your computer with a USB cable — **make sure it's a
data cable, not a charge-only one** (many phone-charging cables have no data lines and
this is the #1 reason "nothing happens"). Then check **Windows' Device Manager**
(Start menu → type `Device Manager` → Enter → look under **"Ports (COM & LPT)"**) for
a new COM port. If nothing new shows up there:

- Most Maker-ESP32-Pro boards use a **CP2102** USB-to-serial chip → **in your browser**,
  go to the
  [CP210x driver page at Silicon Labs](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers),
  download and run their installer the same way as Git/VS Code above.
- Some use a **CH340** chip instead → search "CH340 driver Windows" if CP210x doesn't
  fix it, and install that one the same way.

### 8. Open the project and do your first build

**Where:** back in **VS Code**.

1. **File → Open Folder** (top menu bar) → in the folder browser that pops up,
   navigate to and select
   `C:\Electronics\davinci-boat-student-kit\phase0-bringup` → click **Select Folder**.
2. Plug the board in via USB (if it isn't already).
3. Look at the **blue status bar running along the very bottom edge of the VS Code
   window** — you'll see small icons: a **checkmark** (Build), a **right arrow**
   (Upload), a **plug** (Serial Monitor).
4. **Click the checkmark.** A terminal panel opens at the bottom of VS Code showing
   build progress — the first build takes a while (it downloads the ESP32 toolchain).
   Watch it; you want the last lines to say `[SUCCESS]`.
5. **Click the right arrow** to flash (upload) it to the board.
6. **Click the plug** to open the Serial Monitor (a panel that shows what the board is
   printing) and see it running.

If step 4 or 5 fails, check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) before assuming
it's your fault — most first-time failures are one of the known issues listed there.

---

## Mac

You'll move between **two places**: your web browser (to download things) and
**Terminal** (a text-command window, for typing commands). Every step below says
which one to use.

### 1. Install the Xcode Command Line Tools

This gives you `git` and other basic developer tools.

1. **Open Terminal**: press `⌘+Space` (Command key + Spacebar) to open Spotlight
   search, type `Terminal`, press **Enter**. A window with a text prompt opens — this
   is where you'll type commands for the rest of Mac setup. **Keep this window open**;
   you'll reuse it in later steps (if you close it, reopen the same way).
2. **In that Terminal window**, type and press Enter:
   ```bash
   xcode-select --install
   ```
3. A popup window appears on your screen (not inside Terminal) — click **Install** on
   it, accept the license, and wait for it to finish (a few minutes; you'll see a
   progress bar in the popup).
4. Back **in Terminal**, check it worked:
   ```bash
   git --version
   ```
   You should see something like `git version 2.xx.x (Apple Git-xxx)` printed back.

### 2. Install VS Code

VS Code is the editor you'll write and build all your code in.

1. **In your web browser**, go to <https://code.visualstudio.com/> and click the Mac
   download button. It lands in your **Downloads** folder.
2. **Open Finder → Downloads**, find the file you just downloaded (a `.zip`), and
   double-click it to unzip it — this creates a **Visual Studio Code** app icon in
   that same Downloads folder.
3. **Drag that icon into your Applications folder** (open a second Finder window via
   Finder → Go → Applications, and drag the icon there) — this is how Mac apps get
   "installed."
4. **Open it**: Finder → Applications → double-click **Visual Studio Code**. The
   first time, macOS may ask "are you sure you want to open this app downloaded from
   the internet?" — click **Open** to allow it.

### 3. Install the PlatformIO extension

This step happens **inside VS Code**, not in Terminal or the browser.

1. **In VS Code**, click the **Extensions** icon in the left-hand sidebar (four small
   squares), or press `⌘+Shift+X`.
2. In the search box at the top, type **"PlatformIO IDE"**.
3. Click the blue **Install** button on the result published by **PlatformIO**.
4. **Wait** for its first-run setup to finish (downloads a Python-based toolchain —
   can take several minutes; you'll see progress in the bottom-right of VS Code).
5. **Fully quit and reopen VS Code** (`⌘+Q` to quit completely, then reopen it from
   Applications) — you should now see an **ant/alien icon** in the left sidebar.

### 4. Apply the one-time Bluepad32 patch

Same known issue as Windows — a version mismatch in one of the Bluetooth library's
dependencies that hits every fresh install. Fix it once, now.

**Where to type this:** back in your **Terminal window** (reopen it if it's closed:
`⌘+Space` → type `Terminal` → Enter).

```bash
~/.platformio/penv/bin/pip install "click==8.1.8"
```

> **What does `~` mean?** It's shorthand every Mac/Linux terminal understands for
> **your personal home folder** — normally `/Users/<your Mac username>`. You never
> need to type or figure out your actual username; the terminal substitutes it
> automatically. The command just means "run the `pip` program that lives inside the
> PlatformIO folder PlatformIO already created for you, and tell it to install a
> specific version of `click`."

If that says `No such file or directory`, PlatformIO's first-run setup (step 3.4
above) hasn't finished yet — go back, wait for it, then retry this command.

### 5. Clone the project

**Where to type this:** the same **Terminal window** as before.

```bash
cd ~/Documents
mkdir -p Electronics && cd Electronics
git clone --recursive --shallow-submodules https://github.com/Electronics-AndresForero/davinci-boat-student-kit.git
```

*(What these do, line by line: `cd ~/Documents` moves you into your Documents folder;
`mkdir -p Electronics && cd Electronics` creates a new `Electronics` folder there (if
it doesn't already exist) and moves into it; `git clone ...` downloads this whole
project into it, creating `~/Documents/Electronics/davinci-boat-student-kit/`.)*

**`--recursive` matters** — it pulls in the Bluetooth library, which is bundled as a
sub-project. **`--shallow-submodules` matters too** — without it, git fetches that
sub-project's *entire history* (a very slow, very large download) instead of just
the current snapshot. If you forget either flag, run this **from inside the cloned
folder** (`~/Documents/Electronics/davinci-boat-student-kit`, in the same Terminal
window):
```bash
cd davinci-boat-student-kit
git submodule update --init --recursive --depth 1
```

This downloads a fair amount of data — give it a few minutes. The Terminal will look
"stuck" with no new text while it works — that's normal, not a freeze.

### 6. USB driver (usually not needed, but if the board isn't detected)

Plug the board into a USB port on your Mac with a **data-capable** cable (not
charge-only). Modern macOS usually recognizes CP2102-based boards without any extra
driver — you can check by opening PlatformIO's port list (see step 7 below) and
looking for a new entry once it's plugged in. If nothing shows up:

- **In your browser**, go to the
  [CP210x VCP driver page at Silicon Labs](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers),
  download the Mac installer, and run it the same way as VS Code above (open the
  downloaded file from Downloads, follow its installer).
- On Apple Silicon Macs, macOS may block the driver the first time — go to the
  **Apple menu → System Settings → Privacy & Security**, scroll down, and click
  **Allow** next to the blocked system extension notice, then restart your Mac.

### 7. Open the project and do your first build

**Where:** back in **VS Code**.

1. **File → Open Folder** (top menu bar) → in the folder browser that pops up,
   navigate to and select
   `~/Documents/Electronics/davinci-boat-student-kit/phase0-bringup` (your Home
   folder → Documents → Electronics → davinci-boat-student-kit → phase0-bringup) →
   click **Open**.
2. Plug the board in (if it isn't already).
3. Look at the **blue status bar running along the very bottom edge of the VS Code
   window**: a **checkmark** icon (Build), a **right arrow** icon (Upload), a **plug**
   icon (Serial Monitor).
4. **Click the checkmark icon** to Build. A terminal panel opens at the bottom showing
   progress — the first build takes a while (downloads the ESP32 toolchain). You want
   the last lines to say `[SUCCESS]`.
5. **Click the right arrow icon** to Upload.
6. **Click the plug icon** to open the Serial Monitor and see it running.

If Build or Upload fails, check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) first.

---

## You're set up when...

`phase0-bringup` builds, uploads, an LED blinks on your board, and the serial monitor
prints a counter. That's it — move on to `phase0-bringup/README.md` if you haven't
already read it, then start on E1.
