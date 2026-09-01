# boat-receiver — the boat's ESP-NOW firmware

This **replaces** the boat's Bluepad32 firmware (`boat-firmware/`) when you're using
the E7 handheld relay — the boat no longer talks Bluetooth to a controller directly,
it just listens for ESP-NOW packets from the handheld.

Unlike `boat-firmware/`, this is a **plain Arduino PlatformIO project** — no ESP-IDF,
no submodule, no Bluepad32 — because the boat doesn't need Bluetooth anymore. Open
this folder directly in VS Code/PlatformIO.

## Your task

Fill in the `TODO` in `src/main.cpp`, and **paste your `PaddleDrive`, `Steering`**
(from E2/E3) where the comment says to. The rest — `applyFailsafe`, the control loop —
is your E5 code, already filled in for you here since you've already proven it works.

```cpp
void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len){
  // TODO: if len == sizeof(Packet), copy the bytes into a local Packet
  // (memcpy(&p, data, sizeof(p))), then fill cmd.throttle, cmd.steer,
  // cmd.armed (from bit0 of p.flags), and cmd.stamp = millis().
}
```

**Note:** to also keep the **autonomous** mode from E5, drop your `AutoPlanner` in
and let the mode switch choose between the planner and the ESP-NOW command — exactly
as E5 did, just with ESP-NOW replacing Bluepad32 as the RC source. Not required for
the basic E7 checkpoint.

## Hints

<details>
<summary>Hint 1</summary>

The receive callback TODO is almost exactly your E5 `applyFailsafe`'s job in
reverse — instead of computing whether to disarm, you're just recording what
arrived. `memcpy(&p, data, sizeof(p))` extracts the struct.
</details>

<details>
<summary>Hint 2 — exact shape</summary>

```cpp
if (len == sizeof(Packet)) {
  Packet p; memcpy(&p, data, sizeof(p));
  cmd.throttle = p.throttle;
  cmd.steer    = p.steer;
  cmd.armed    = (p.flags & 1);
  cmd.stamp    = millis();
}
```
</details>

## Checkpoint

See `../README.md`'s Verify section (this piece is tested together with
`../handheld/`).

## Troubleshooting

- **`esp_now_register_recv_cb` won't compile / wrong callback signature:** you used
  the old 2.x signature `(const uint8_t* mac, …)`. On core 3.x it's
  `(const esp_now_recv_info_t* info, const uint8_t* data, int len)` — same
  "API-changed-in-3.x" trap as LEDC in E2.
- General tooling issues: [../../TROUBLESHOOTING.md](../../TROUBLESHOOTING.md).
