// ============================================================================
//  BORRADOR -- modo autonomo con MPU-6050 (mantener/girar RUMBO).
//  No compilado, no probado. Revisar TODO.
//  Sensor: MPU-6050 (WHO_AM_I = 0x68).
//
//  Diferencia clave contra borrador_tiempo.txt: en vez de "gira durante 1.5s
//  y ojala quede bien", esto gira "hasta que el giroscopio mida que cambiaste
//  X grados de rumbo" -- es un lazo cerrado sobre el RESULTADO del giro, no
//  sobre cuanto tiempo se mantuvo el comando. Esto importa porque, segun lo
//  discutido, la direccion actual no responde de forma perfectamente
//  predecible (se desvia, hay que corregir) -- un giro por tiempo puro hereda
//  ese error; un giro por angulo medido, no (siempre que el sensor funcione).
//
//  Lo que ESTO NO arregla: la posicion/angulo de arranque, ni el "cuando"
//  empezar a girar (sigue siendo por tiempo, no hay sensor de distancia a la
//  boya). El giroscopio mejora el COMO se ejecuta cada tramo, no el CUANDO.
//
//  NUEVO -- log a flash interna (LittleFS), igual que en borrador_tiempo.txt,
//  pero agregando columnas de giroscopio/rumbo:
//    t_ms, modo, throttle, steer, gyro_z_dps, heading_deg, duty
//  (duty = ciclo de trabajo REAL aplicado al motor, ya con DUTY_CAP y rampa; throttle es el comando antes del tope)
//  Se vuelca por Console al reiniciar (===LOG_START===/===LOG_END===).
//  Con este log + analizar_log.py se puede: (a) ver el rumbo real reconstruido
//  en una prueba manejada por control remoto, (b) que el script proponga
//  TURN_TARGET_DEG y los *_MS_CAP automaticamente en vez de adivinarlos.
//
//  Cableado confirmado (modulo GY-521, trae regulador a bordo -- 3-5V tolerante):
//   VCC->5V, GND->G, SCL->SCL(22), SDA->SDA(21)  -- los 4 desde el header "IIC接口".
//   AD0->G (jumper corto a cualquier tierra de la placa, fija direccion 0x68).
//   XDA, XCL, INT -- sin conectar (no se usan).
//
//  TODO antes de usar esto:
//   - Confirmar WHO_AM_I (registro 0x75) = 0x68 (MPU-6050).
//   - Verificar signo de GYRO_ZOUT: girar el barco a la derecha a mano y
//     confirmar que headingDeg AUMENTA (si no, invertir el signo en el mapeo).
//   - Calibrar TURN_TARGET_DEG con el log/video de la practica (cuantos
//     grados hay que girar realmente para rodear la boya).
//   - Calibrar STEER_KP (empezar bajo, 0.01-0.03, y subir si no corrige lo
//     suficiente; si oscila/zigzaguea, esta muy alto).
//   - Los *_MS_CAP son la red de seguridad si el sensor falla o da lecturas
//     raras -- deben quedar generosos pero no infinitos.
// ============================================================================
#include <Arduino.h>
#include <Bluepad32.h>
#include <Wire.h>
#include <LittleFS.h>

// ---------- pines ----------
constexpr int PIN_DRIVE_A   = 27, PIN_DRIVE_B = 13;
constexpr int PIN_STEER     = 25;
constexpr int PIN_LED       = 33;
constexpr int PIN_ARM_SWITCH  = 32;   // kill switch, cerrado=LOW=armado
constexpr int PIN_AUTO_SWITCH = 26;   // mismo header "Servo" que STEER(25)/LED(33)/ARM(32) --
                                       // confirmado libre en la placa de expansion.
                                       // cerrado=LOW=modo autonomo
constexpr int PIN_SDA = 21, PIN_SCL = 22;   // I2C por defecto del ESP32 devkit -- libres

// ---------- PWM motor / servo (igual a los demas borradores) ----------
constexpr int MOT_FREQ = 5000, MOT_BITS = 10, MOT_MAX = (1 << MOT_BITS) - 1;
constexpr int SRV_FREQ = 50, SRV_BITS = 16, SRV_PERIOD_US = 20000;
constexpr int STEER_CENTER_US = 1500, STEER_RANGE_US = 750;
constexpr float DEADZONE      = 0.06f;
constexpr float THROTTLE_SLEW = 4.0f;
// 0.80 minimo: con menos el barco no se mueve (medido). Es el tope del PWM real:
// throttle 1.0 del comando = 80% de duty. Aplica igual en RC y en autonomo, porque
// ambos pasan por la misma linea `cmd.throttle * DUTY_CAP` en loop().
// OJO: motor 6 V en pack 2S de 7.4 V -- no subir de aqui sin razon.
constexpr float DUTY_CAP      = 0.80f;

// ---------- log a flash (LittleFS) ----------
constexpr unsigned long LOG_INTERVAL_MS   = 50;    // ~20 Hz
constexpr unsigned long LOG_FLUSH_MS      = 1000;
constexpr const char*   LOG_PATH          = "/log.csv";
File logFile;
unsigned long lastLogMs   = 0;
unsigned long lastFlushMs = 0;

void dumpLogOverSerial() {
  File f = LittleFS.open(LOG_PATH, FILE_READ);
  if (!f || f.size() == 0) { if (f) f.close(); return; }
  Console.println("===LOG_START===");
  while (f.available()) Console.println(f.readStringUntil('\n'));
  Console.println("===LOG_END===");
  f.close();
  LittleFS.remove(LOG_PATH);
}

void logRow(unsigned long now, bool autoMode, float throttle, float steer,
            float gyroZDps, float heading, float duty) {
  if (!logFile) return;
  if (now - lastLogMs < LOG_INTERVAL_MS) return;
  lastLogMs = now;
  logFile.printf("%lu,%d,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                 now, autoMode ? 1 : 0, throttle, steer, gyroZDps, heading, duty);
  if (now - lastFlushMs >= LOG_FLUSH_MS) { lastFlushMs = now; logFile.flush(); }
}

// ---------- MPU-6050 ----------
constexpr uint8_t MPU_ADDR      = 0x68;   // AD0 a GND -> 0x68
constexpr uint8_t REG_PWR_MGMT1 = 0x6B;
constexpr uint8_t REG_GYRO_CFG  = 0x1B;
constexpr uint8_t REG_GYRO_ZH   = 0x47;   // Z_OUT_H seguido de Z_OUT_L
constexpr uint8_t REG_WHO_AM_I  = 0x75;
constexpr float   GYRO_LSB_PER_DPS = 131.0f;   // sensibilidad en +-250 dps (config por defecto)

bool  gyroOK = false;        // si el sensor no responde, cae a modo "solo tiempo"
float gyroZBiasRaw = 0.0f;   // sesgo medido en reposo antes de lanzar
float headingDeg = 0.0f;     // rumbo integrado (relativo al arranque, no absoluto)
float lastGyroZDps = 0.0f;   // ultima lectura, cacheada para el log (evita leer 2 veces por ciclo)

void mpuWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg); Wire.write(val);
  Wire.endTransmission();
}
uint8_t mpuReadByte(uint8_t reg) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((int)MPU_ADDR, 1, true);
  return Wire.available() ? Wire.read() : 0;
}
int16_t mpuReadGyroZRaw() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(REG_GYRO_ZH);
  Wire.endTransmission(false);
  Wire.requestFrom((int)MPU_ADDR, 2, true);
  int16_t hi = Wire.read(), lo = Wire.read();
  return (int16_t)((hi << 8) | lo);
}

// Calibra el sesgo del giroscopio Z. LLAMAR CON EL BARCO QUIETO antes de soltarlo.
void calibrateGyroBias() {
  const int N = 200;
  long sum = 0;
  for (int i = 0; i < N; i++) { sum += mpuReadGyroZRaw(); delay(3); }
  gyroZBiasRaw = (float)sum / N;
}

bool mpuBegin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  mpuWrite(REG_PWR_MGMT1, 0x00);   // despertar
  delay(50);
  mpuWrite(REG_GYRO_CFG, 0x00);    // +-250 dps
  uint8_t who = mpuReadByte(REG_WHO_AM_I);
  if (who != 0x68) return false;   // 0x68 = MPU-6050
  calibrateGyroBias();
  return true;
}

void updateHeading(float dt) {
  if (!gyroOK) return;
  lastGyroZDps = (mpuReadGyroZRaw() - gyroZBiasRaw) / GYRO_LSB_PER_DPS;
  headingDeg += lastGyroZDps * dt;
}

// ---------- comando normalizado ----------
struct Command {
  float throttle = 0;
  float steer    = 0;
  bool  armed    = false;
  unsigned long stamp = 0;
};

// ---------- actuadores ----------
class PaddleDrive {
public:
  void begin() { ledcAttach(PIN_DRIVE_A, MOT_FREQ, MOT_BITS);
                 ledcAttach(PIN_DRIVE_B, MOT_FREQ, MOT_BITS); stop(); }
  void set(float t) {
    t = constrain(t, -1.f, 1.f);
    uint32_t duty = (uint32_t)(fabs(t) * MOT_MAX);
    ledcWrite(PIN_DRIVE_A, t >= 0 ? duty : 0);
    ledcWrite(PIN_DRIVE_B, t <  0 ? duty : 0);
  }
  void stop() { ledcWrite(PIN_DRIVE_A, 0); ledcWrite(PIN_DRIVE_B, 0); }
};

class Steering {
public:
  void begin() { ledcAttach(PIN_STEER, SRV_FREQ, SRV_BITS); set(0); }
  void set(float s) {
    s = constrain(s, -1.f, 1.f);
    int us = STEER_CENTER_US + (int)(s * STEER_RANGE_US);
    ledcWrite(PIN_STEER, (uint32_t)((uint64_t)us * (1UL << SRV_BITS) / SRV_PERIOD_US));
  }
};

PaddleDrive drive;
Steering    steer;

// ---------- entrada: control Xbox ----------
ControllerPtr pad = nullptr;
void onConnect(ControllerPtr c)    { if (!pad) pad = c; }
void onDisconnect(ControllerPtr c) { if (pad == c) pad = nullptr; }
float deadzone(float v) { return fabs(v) < DEADZONE ? 0.f : v; }
bool armSwitchClosed()  { return digitalRead(PIN_ARM_SWITCH)  == LOW; }
bool autoSwitchClosed() { return digitalRead(PIN_AUTO_SWITCH) == LOW; }

bool readController(Command& cmd) {
  if (!pad || !pad->isConnected() || !pad->isGamepad()) return false;
  float fwd = pad->throttle() / 1023.0f;
  float rev = pad->brake()    / 1023.0f;
  cmd.throttle = deadzone(fwd - rev);
  cmd.steer    = deadzone(pad->axisX() / 512.0f);
  cmd.armed    = true;
  cmd.stamp    = millis();
  return true;
}

// ---------- maquina de estados del modo autonomo ----------
enum AutoState { AUTO_STRAIGHT1, AUTO_TURN, AUTO_STRAIGHT2, AUTO_DONE };
AutoState     autoState = AUTO_STRAIGHT1;
unsigned long autoStateStartMs = 0;
float         headingRef = 0.0f;   // rumbo objetivo del tramo recto actual

// TODO: calibrar estos 5 numeros con el log/video de la practica.
constexpr float STEER_KP = 0.02f;              // correccion proporcional del rumbo (recto)
constexpr float STEER_MAX_CORR = 0.35f;        // no dejar que la correccion tape el throttle
constexpr float TURN_TARGET_DEG = 70.0f;       // cuanto girar para rodear la boya
constexpr float TURN_STEER_CMD  = 0.6f;        // steer fijo mientras se ejecuta el giro
constexpr unsigned long STRAIGHT1_MS_CAP = 4000;  // salvaguarda de tiempo (por si falla el giro)
constexpr unsigned long TURN_MS_CAP      = 3000;
constexpr unsigned long STRAIGHT2_MS_CAP = 5000;

float headingHoldSteer(float target) {
  if (!gyroOK) return 0.0f;   // sin giroscopio no hay correccion -- va recto "a ciegas"
  float err = headingDeg - target;
  return constrain(-STEER_KP * err, -STEER_MAX_CORR, STEER_MAX_CORR);
}

Command autoStep(unsigned long now) {
  Command out; out.armed = true; out.stamp = now;
  unsigned long elapsed = now - autoStateStartMs;

  switch (autoState) {
    case AUTO_STRAIGHT1:
      out.throttle = 1.0f;   // 1.0 x DUTY_CAP(0.80) = 80% duty, el minimo que mueve el barco
      out.steer    = headingHoldSteer(headingRef);
      if (elapsed >= STRAIGHT1_MS_CAP) {          // TODO: idealmente disparar por distancia, no tiempo
        autoState = AUTO_TURN; autoStateStartMs = now; headingRef = headingDeg;
      }
      break;
    case AUTO_TURN:
      out.throttle = 1.0f;   // idem: con menos de 80% duty el barco no se mueve
      out.steer    = TURN_STEER_CMD;
      if (fabs(headingDeg - headingRef) >= TURN_TARGET_DEG || elapsed >= TURN_MS_CAP) {
        autoState = AUTO_STRAIGHT2; autoStateStartMs = now; headingRef = headingDeg;
      }
      break;
    case AUTO_STRAIGHT2:
      out.throttle = 1.0f;   // 1.0 x DUTY_CAP(0.80) = 80% duty, el minimo que mueve el barco
      out.steer    = headingHoldSteer(headingRef);
      if (elapsed >= STRAIGHT2_MS_CAP) { autoState = AUTO_DONE; autoStateStartMs = now; }
      break;
    case AUTO_DONE:
    default:
      out.throttle = 0.0f; out.steer = 0.0f;
      break;
  }
  return out;
}

Command cmd;
float applied = 0.0f;
unsigned long lastLoop = 0;
bool lastAutoSwitch = false;
bool autoActive = false;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_ARM_SWITCH,  INPUT_PULLUP);
  pinMode(PIN_AUTO_SWITCH, INPUT_PULLUP);

  delay(200);
  if (!LittleFS.begin(true)) {
    Console.println("[log] LittleFS no monto -- log deshabilitado");
  } else {
    dumpLogOverSerial();
    logFile = LittleFS.open(LOG_PATH, FILE_APPEND);
    if (logFile && logFile.size() == 0)
      logFile.println("t_ms,modo,throttle,steer,gyro_z_dps,heading_deg,duty");
  }

  drive.begin();
  steer.begin();
  gyroOK = mpuBegin();   // si falla, autoStep sigue funcionando pero sin correccion de rumbo
  BP32.setup(&onConnect, &onDisconnect);
  BP32.enableVirtualDevice(false);
  lastLoop = millis();
  Console.printf("\n[Fase 6] Modo autonomo (MPU-6050). gyroOK=%d\n", gyroOK);
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastLoop) / 1000.0f; lastLoop = now;

  updateHeading(dt);

  bool autoSw = autoSwitchClosed();
  if (autoSw && !lastAutoSwitch) {
    autoActive = true;
    autoState  = AUTO_STRAIGHT1;
    autoStateStartMs = now;
    headingDeg = 0.0f;      // el rumbo se mide RELATIVO al momento de encender el modo
    headingRef = 0.0f;
  }
  if (!autoSw) autoActive = false;
  lastAutoSwitch = autoSw;

  if (autoActive) {
    cmd = autoStep(now);
  } else {
    BP32.update();
    if (!readController(cmd)) cmd.armed = false;
  }

  if (!armSwitchClosed()) { cmd.armed = false; autoActive = false; }  // kill switch manda siempre

  float target  = cmd.armed ? cmd.throttle * DUTY_CAP : 0.0f;
  float maxStep = THROTTLE_SLEW * dt;
  applied += constrain(target - applied, -maxStep, maxStep);

  drive.set(applied);
  steer.set(cmd.armed ? cmd.steer : 0.0f);
  logRow(now, autoActive, cmd.armed ? cmd.throttle : 0.0f, cmd.armed ? cmd.steer : 0.0f,
         lastGyroZDps, headingDeg, applied);

  digitalWrite(PIN_LED, cmd.armed ? HIGH : (now / 300 % 2));
  delay(10);
}
