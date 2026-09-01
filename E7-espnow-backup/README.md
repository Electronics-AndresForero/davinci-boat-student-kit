# E7 — ESP-NOW long-range link *(optional / reserve)*

**Only build this if lake day shows Bluetooth's real range (~10–15 m) doesn't cover
the actual course.** It's not part of the default schedule — it's here so you have it
ready if you need it.

**Goal:** break past Bluetooth's range ceiling so you can drive the boat across a real
lake — **while still using the Xbox controller.** We add a **second ESP32 in your
hand** that reads the pad over Bluetooth and relays commands to the boat over
**ESP-NOW**, a long-range Espressif radio protocol (~100–200 m line-of-sight).

**You'll end with:** a handheld transmitter (ESP32 + Xbox pad) and the boat driving
from it at distances Bluetooth could never reach — with the same arming and failsafe
you already built in E5.

This is the most advanced piece here — read the concepts fully before starting.

---

## The concept: what ESP-NOW actually is

### The problem with "normal" wireless

Both **Bluetooth** and **Wi-Fi** spend a lot of effort *maintaining a connection*:
pairing, handshakes, keeping a session alive, (for Wi-Fi) joining a router and
getting an IP address. That machinery costs **range, latency, and setup complexity**
— none of which a "send stick position to a boat 40 m away" job needs.

### ESP-NOW: connectionless messaging on the Wi-Fi radio

**ESP-NOW** is Espressif's own lightweight protocol. It uses the **Wi-Fi radio's
physical layer** but **throws away everything above it** — no router, no IP, no
association, no pairing session. Instead it sends tiny **vendor-specific action
frames** addressed **directly to another device's MAC address**. Think of it as a
**walkie-talkie to a specific serial number**, versus Bluetooth's "place a phone call
and hold the line."

| Property | ESP-NOW | Why it matters here |
|---|---|---|
| **Range** | ~100–200 m+ line-of-sight | covers a whole lake |
| **Latency** | ~1–2 ms | control feels instant |
| **Connectionless** | fire-and-forget to a MAC; no session to "drop" | robust to the boat going in/out of range — it just resumes |
| **Payload** | up to 250 bytes/packet | a `Command` is ~9 bytes — tons of room |
| **Setup** | no router, no IP, no pairing dance | just tell each side the other's MAC + a common channel |

### How addressing works (MACs and channels)

Every ESP32 has a unique **MAC address**. ESP-NOW sends **to a MAC**, so each side
must know the other's. You **register a peer** by its MAC, then
`esp_now_send(mac, data, len)`. Two rules that trip everyone up:

1. **Both devices must be on the same Wi-Fi channel** (we use **1**). If they
   disagree, packets silently vanish.
2. There's still **no "connection"** to lose. If the boat is out of range, sends just
   fail quietly until it's back in range.

### Why the failsafe you already built is perfect for this

ESP-NOW being connectionless means "link lost" isn't an event you get notified of —
the boat simply **stops receiving packets**. But your **E5 failsafe already handles
exactly that**: every command carries a `stamp`, and if the newest one is older than
`FAILSAFE_MS`, the boat disarms. A boat that drifts out of ESP-NOW range **stops on
its own** and resumes when packets return — **no new safety code needed.**

### ⚠️ The one hard part: Bluetooth + Wi-Fi at the same time

The **handheld** must run **Bluetooth** (to read the Xbox pad via Bluepad32) **and**
Wi-Fi/ESP-NOW (to transmit) **simultaneously** on one chip. The ESP32 *can* do this —
it **time-shares its single 2.4 GHz radio** between the two stacks ("coexistence") —
but it's the fiddliest part of this exercise. The **boat** gets *simpler*: it no
longer needs Bluetooth at all, so it runs **only** ESP-NOW.

## What you need

- **A second ESP32** for the handheld — a **plain ESP32** dev board (WROOM-32),
  because it must do Bluetooth for the Xbox pad (not an S3/C3/C6).
- A small battery / USB power bank for the handheld, and a box to hold it + the pad.
- The **boat** keeps its Maker-ESP32-Pro — now running the simpler ESP-NOW receiver
  instead of Bluepad32.

## The three pieces, in order

1. **`get-boat-mac/`** — flash this once to the boat, copy the MAC address it prints.
   Full working code, not an exercise.
2. **`handheld/`** — the transmitter. Same Bluepad32 project style as
   `boat-firmware/` (it's a submodule + an exercise you copy into
   `template/main/sketch.cpp`). Reads the pad, sends over ESP-NOW.
3. **`boat-receiver/`** — the boat's new firmware. A **plain Arduino project**, no
   Bluepad32 needed. Receives ESP-NOW, drives the same actuators as E5, reuses your
   E5 failsafe unchanged.

## Verify

1. **MAC + channel:** boat's MAC pasted into the handheld, `RADIO_CHANNEL` the same
   (1) on both. Flash both.
2. **Bench, close range:** power both, pair the Xbox pad to the **handheld**. Squeeze
   RT → the **boat's** paddles spin; stick steers the rudder.
3. **Failsafe:** turn the **pad** off → handheld sends `flags=0` → boat disarms. Turn
   the **handheld** off entirely → boat stops via the stale-`stamp` failsafe. Both
   stop the boat, as designed.
4. **Range test (outdoors, safe area):** walk the boat away from the handheld and
   confirm control holds far past Bluetooth's old limit. Note where it starts to
   drop — that's your usable range.

## Troubleshooting

Concept-specific traps live in `handheld/exercises/E7-handheld/README.md` and
`boat-receiver/README.md`. For general tooling issues see
[../TROUBLESHOOTING.md](../TROUBLESHOOTING.md).

- **Boat never moves from the handheld:** MAC wrong, or channels differ. Re-check the
  boat's printed MAC and that both set `RADIO_CHANNEL = 1`.
- **Pad drops when Wi-Fi starts (handheld):** BT/Wi-Fi coexistence fighting — keep the
  send rate ≤ 50 Hz, and make sure you're not also trying to join a real Wi-Fi
  network.
- **Short range / dropouts outdoors:** hold the handheld's antenna up and away from
  your body.
- **Boat twitches/garbage:** the `Packet` struct must be byte-identical on both sides
  (it is, if you copied both without editing the struct itself).

## Notebook

If this gets built, note in the firmware section *why* it was needed (the lake day's
measured BLE range) and the range you actually achieved with ESP-NOW.
