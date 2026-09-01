// ============================================================================
//  E7 step 1 — print the boat's Wi-Fi MAC address.
//  Flash this to the BOAT once. Copy the printed address into
//  handheld/exercises/E7-handheld/sketch.cpp as BOAT_MAC[].
//  Given code, not an exercise — after you've copied the MAC down, re-flash
//  the boat with boat-receiver/ instead of this.
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(200);
  WiFi.mode(WIFI_STA);
  Serial.print("BOAT MAC: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // nothing to do — the address only needs to be read once
}
