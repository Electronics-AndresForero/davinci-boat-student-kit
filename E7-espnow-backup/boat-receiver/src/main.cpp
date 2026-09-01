// ============================================================================
//  BOAT receiver: ESP-NOW -> Command -> motor + rudder.  (No Bluetooth here.)
//  Plain esp32dev Arduino project. Reuses the E5 safety layer.
//  Fill in the TODO in onRecv() below — see this folder's README.
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

constexpr int PIN_DRIVE_A=27, PIN_DRIVE_B=13, PIN_STEER=25, PIN_LED=33;
constexpr int PIN_MODE_SWITCH=26, PIN_ARM_SWITCH=32;
constexpr int MOT_FREQ=5000, MOT_BITS=10, MOT_MAX=(1<<MOT_BITS)-1;
constexpr int SRV_FREQ=50, SRV_BITS=16, SRV_PERIOD_US=20000;
constexpr int STEER_CENTER_US=1500, STEER_RANGE_US=400;
constexpr float THROTTLE_SLEW=2.0f, DUTY_CAP=0.60f;  // tuned for the 2S 18650 pack (7.4 V)
constexpr unsigned long FAILSAFE_MS=500;
constexpr int RADIO_CHANNEL=1;                         // MUST match the handheld

struct __attribute__((packed)) Packet { float throttle, steer; uint8_t flags; };
struct Command { float throttle=0, steer=0; bool armed=false; unsigned long stamp=0; };

// --- paste your PaddleDrive and Steering classes from E2/E3 here, unchanged ---


PaddleDrive drive; Steering steer;

// shared command, written by the ESP-NOW receive callback
volatile Command cmd;

// core-3.x recv callback signature: (const esp_now_recv_info_t*, const uint8_t*, int)
void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len){
  // TODO: if len == sizeof(Packet), copy the bytes into a local Packet
  // (memcpy(&p, data, sizeof(p))), then fill cmd.throttle, cmd.steer,
  // cmd.armed (from bit0 of p.flags), and cmd.stamp = millis().
}

// paste your applyFailsafe() from E5, unchanged:
void applyFailsafe(Command& c){
  bool stale  = (millis() - c.stamp) > FAILSAFE_MS;    // covers ESP-NOW loss too!
  bool armOff = digitalRead(PIN_ARM_SWITCH) == HIGH;
  if (stale || armOff) c.armed = false;
}

float applied=0; unsigned long lastLoop=0;

void setup(){
  pinMode(PIN_MODE_SWITCH,INPUT_PULLUP); pinMode(PIN_ARM_SWITCH,INPUT_PULLUP);
  pinMode(PIN_LED,OUTPUT);
  drive.begin(); steer.begin();
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  esp_wifi_set_channel(RADIO_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_now_init();
  esp_now_register_recv_cb(onRecv);
  lastLoop=millis();
  Serial.begin(115200);
  Serial.println("\n[boat] ESP-NOW receiver ready.");
}

void loop(){
  unsigned long now=millis(); float dt=(now-lastLoop)/1000.f; lastLoop=now;

  Command c = cmd;                       // snapshot the volatile shared struct
  applyFailsafe(c);

  float target = c.armed ? c.throttle*DUTY_CAP : 0.f;
  applied += constrain(target-applied, -THROTTLE_SLEW*dt, THROTTLE_SLEW*dt);
  drive.set(applied);
  steer.set(c.armed ? c.steer : 0.f);

  digitalWrite(PIN_LED, c.armed ? HIGH : (now/400%2));
  delay(10);
}
