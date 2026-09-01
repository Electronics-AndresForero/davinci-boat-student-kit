// ============================================================================
//  HANDHELD transmitter: Xbox pad (BT) -> ESP-NOW -> boat.
//  Runs Bluetooth + Wi-Fi together (coexistence). Uses core-3.x ESP-NOW API.
// ============================================================================
#include <Arduino.h>
#include <Bluepad32.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// <-- paste your boat's MAC here (from get-boat-mac/)
uint8_t BOAT_MAC[6] = { /* paste your boat's MAC bytes here, e.g. 0x24, 0x6F, ... */ };
constexpr int RADIO_CHANNEL = 1;          // MUST match the boat

// the exact bytes we send each tick
struct __attribute__((packed)) Packet {
  float   throttle;   // -1..+1
  float   steer;      // -1..+1
  uint8_t flags;      // bit0 = armed/connected
};

ControllerPtr pad = nullptr;
void onConnect(ControllerPtr c){ if(!pad) pad=c; }
void onDisconnect(ControllerPtr c){ if(pad==c) pad=nullptr; }
float deadzone(float v){ return fabs(v)<0.06f ? 0.f : v; }

void setup(){
  // --- Wi-Fi / ESP-NOW side ---
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();                                   // don't join any router
  esp_wifi_set_channel(RADIO_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_now_init();
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BOAT_MAC, 6);
  peer.channel = RADIO_CHANNEL;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  // --- Bluetooth / Xbox side (runs alongside Wi-Fi: coexistence) ---
  BP32.setup(&onConnect, &onDisconnect);
  BP32.enableVirtualDevice(false);
  Console.println("\n[handheld] ready. Pair the Xbox pad (Sync).");
}

void loop(){
  BP32.update();

  Packet p{};
  // TODO: if the pad is connected, fill p.throttle / p.steer the same way
  // your E4 readController did (RT/LT and axisX, deadzoned), and set
  // p.flags = 1. If not connected, leave p as all-zero / flags = 0.
  // Then send it: esp_now_send(BOAT_MAC, (uint8_t*)&p, sizeof(p));

  delay(20);                                           // ~50 Hz
}
