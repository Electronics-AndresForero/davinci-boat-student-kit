# E7 (handheld) — the transmitter

**Goal:** read the Xbox pad on a second ESP32 and relay its throttle/steer over
ESP-NOW to the boat, ~50 times a second.

See [../../README.md](../../README.md) (the `E7-espnow-backup/` root) for the full
concepts on what ESP-NOW is and why this needs Bluetooth + Wi-Fi coexistence — read
that first if you haven't.

## Your task

Fill in the `TODO` in [`sketch.cpp`](sketch.cpp) (reproduced here for reference):

```cpp
void loop(){
  BP32.update();

  Packet p{};
  // TODO: if the pad is connected, fill p.throttle / p.steer the same way
  // your E4 readController did (RT/LT and axisX, deadzoned), and set
  // p.flags = 1. If not connected, leave p as all-zero / flags = 0.
  // Then send it: esp_now_send(BOAT_MAC, (uint8_t*)&p, sizeof(p));

  delay(20);
}
```

Before flashing, **paste the boat's MAC address** (from `../../get-boat-mac/`) into
`BOAT_MAC[]` near the top of the file.

## Hints

<details>
<summary>Hint 1</summary>

This is a direct copy of your E4 `readController` logic, just writing into a
`Packet` instead of a `Command`, followed by one `esp_now_send` call. `pad &&
pad->isConnected() && pad->isGamepad()` is the same connectivity check you used
before.
</details>

<details>
<summary>Hint 2 — still stuck</summary>

`p.throttle = deadzone(pad->throttle()/1023.f - pad->brake()/1023.f);` and
`p.steer = deadzone(pad->axisX()/512.f);` are the two assignments, then
`p.flags = 1;`, then the `esp_now_send(...)` call shown in the TODO comment — verbatim.
</details>

## Checkpoint

See the parent `E7-espnow-backup/README.md`'s Verify section — this piece is tested
together with `boat-receiver/`.

## Guiding question

Why must the `Packet` struct be byte-for-byte identical (same fields, same order,
same `__attribute__((packed))`) on both the handheld and the boat? What would you
observe if they drifted apart?
