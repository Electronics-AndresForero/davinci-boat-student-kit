# Phase 0 — Toolchain & board bring-up

**Goal:** understand what the ESP32 actually *is*, and get the board to **blink an LED
and talk to you over serial**. Zero motors, zero Bluetooth — just prove the chip is
alive and that you can flash it. Every later exercise reuses everything here.

**You'll end with:** an LED on GPIO 33 blinking once a second, and the serial monitor
printing a counter.

This one has **no `TODO`s** — it's fully working code, meant to be read and flashed as
your first successful build, not an exercise. E1 onward starts asking you to fill
things in.

---

## The concepts (read before touching code)

### What an ESP32 *is*

The **ESP32-WROOM-32E** on your board is not "an Arduino." It's a far more capable
microcontroller module:

- **Two CPU cores** ("PRO" and "APP") running up to **240 MHz**. Most Arduino code
  runs on one core; the radio stack can live on the other. This matters later:
  Bluetooth (E1) runs partly on core 0 while your control loop runs on core 1, which
  is *why* the boat stays responsive while talking to the controller.
- **~520 KB of SRAM** (working memory) and **4 MB of flash** (where your program
  lives). Compare an Arduino Uno: 2 KB RAM, 32 KB flash. The ESP32 has room for a
  whole Bluetooth stack.
- **An integrated 2.4 GHz radio** doing **Wi-Fi + dual-mode Bluetooth** (both the
  older **Classic/BR-EDR** and the newer **BLE**). We only use Bluetooth, but the fact
  that this chip does *both* flavors is the linchpin of E1.
- **GPIOs** — "General-Purpose Input/Output" pins. Each can be an input (read a
  switch) or an output (drive an LED, a servo signal, a motor-driver pin). Many can
  also do special jobs: PWM, ADC (analog read), I²C, SPI, etc.

### GPIO: HIGH, LOW, and the 3.3 V world

A digital output pin is either **HIGH (≈3.3 V)** or **LOW (0 V)**. That's it. An LED
lights when we set its pin HIGH (current flows through it to ground); it's off when
LOW. **Important:** ESP32 logic is **3.3 V, not 5 V** — never feed 5 V into a GPIO.
(A servo's *power* pin is 5 V, but its *signal* pin is 3.3 V logic — fine.)

### Strapping pins (why our pin map looks the way it does)

At the instant of reset, the ESP32 reads a few specific pins to decide **how to
boot** (run your program vs. enter flash-download mode, boot voltage, etc.). These are
the **strapping pins: GPIO 0, 2, 5, 12, 15.** You *can* use them as normal I/O after
boot, but if something is pulling them the "wrong" way at power-on, the board won't
start. To avoid a whole class of "why won't it boot?" pain, **our pin map deliberately
avoids all of them** — the motor port M0 (27/13) and the servo headers (25/26/32/33)
are all clear of strapping pins. Keep this rule in mind whenever you reassign a pin.

### PWM / LEDC (met now, used hard in E2–E3)

A GPIO can only be fully on or fully off — so how do you dim an LED or set a motor to
*half* speed? **PWM (Pulse-Width Modulation):** switch the pin on/off very fast and
vary the fraction of time it's on (the **duty cycle**). 50% duty ≈ half power. The
ESP32 has dedicated PWM hardware called **LEDC** (LED Controller — despite the name it
drives motors and servos too). We'll blink with plain `digitalWrite` here and graduate
to LEDC in E2.

### How flashing even works (auto-reset)

When you hit **Upload**, your PC talks to the board's **USB-to-serial chip**. To put
the ESP32 into download mode automatically, that chip wiggles two control lines
(**EN** = reset, **GPIO0** = boot select) using the serial port's DTR/RTS signals.
That's why you normally *don't* need to press buttons. When auto-reset fails (some
cables/boards), you do it by hand: **hold BOOT, tap EN/RST, release BOOT.**

---

## Wiring (one LED)

The Maker-ESP32-Pro has no onboard user LED, so we borrow the pin that will later be
our **status LED** — **GPIO 33**.

> **Wait — isn't 33 a servo header?** Yes, and that's fine. The headers labeled
> "servo" (25/26/32/33) are just **ordinary GPIOs** wired to a 3-pin connector
> (signal + 5 V + GND) so a servo can plug in without soldering. Nothing forces them
> to drive a servo — any of them can blink an LED or read a switch. Our boat needs
> only **one** servo (the rudder, on 25), so the spare headers become the mode switch
> (26), arm switch (32), and this **status LED (33)**.

A servo header has three lines: **signal**, **5 V**, **GND**. Put an LED (through a
**330 Ω** resistor, long leg = anode toward signal) between the **signal** pin
(GPIO 33) and **GND**.

```
   GPIO33 (servo signal) ──[330Ω]──▶|── GND
                                    LED
                                (long leg = +, toward the resistor/signal)
```

No LED handy? You can skip the wiring and still complete this — the serial counter
proves the board runs. But wiring it now means your status indicator is already in
place for E5.

---

## Flash & verify

1. Plug the board into USB.
2. PlatformIO toolbar (bottom of VS Code): **✓ Build**, then **→ Upload**.
3. Open the **Serial Monitor** (plug icon).

You should see the LED blink at ~1 Hz and lines scrolling:
```
[Phase 0] DaVinci paddle boat — board is alive.
tick 0  (LED on)
tick 1  (LED on)
...
```

### ✅ Verify the board's pin map while you're here

The blink proving GPIO 33 toggles confirms that header is a real, controllable GPIO —
so before E2 trusts 25/26/27/13/32/33, do a quick sanity pass: change
`PIN_STATUS_LED` to **25**, then **26**, **32**, re-upload, and confirm the LED blinks
on each servo header. (Leave 27/13 for E2, where the motor itself is the test.) If any
pin *doesn't* blink, stop and ask — the board's silkscreen may number things
differently than the docs.

---

## Troubleshooting

Phase-specific traps only — for install issues, upload failures, or port-not-found,
see [../TROUBLESHOOTING.md](../TROUBLESHOOTING.md).

- **LED never lights:** check the resistor and LED polarity (long leg toward the
  signal pin), and that you're on the **signal** pin of the header, not 5 V/GND.
- **Board resets in a loop / won't boot:** something is pulling a strapping pin.
  Disconnect anything on GPIO 0/2/5/12/15 and retry.

---

## Concept check ✅

You can now flash the ESP32, you know it's a dual-core 3.3 V chip with Wi-Fi/BT, you
understand GPIO HIGH/LOW, why we avoid the strapping pins, what PWM/LEDC will do for
us, and how auto-reset flashing works.

Next → `boat-firmware/exercises/E1-xbox-pairing/` — the fun part, the Xbox controller.
