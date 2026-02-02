// =====================================================
//   K1BOT ULTIMATE FIRMWARE
//   Features: Hybrid Control, Live Tuning, Emotes
// =====================================================

#include <AFMotor.h>
#include <SoftwareSerial.h>

// =====================
// BLUETOOTH & HARDWARE
// =====================
// RX Arduino (Pin 2) -> TX Bluetooth
// TX Arduino (Pin 13) -> RX Bluetooth
SoftwareSerial BT(2, 13); 

// Sensor Pins
#define S1 A0  // kiri luar
#define S2 A1  // kiri
#define S3 A2  // tengah
#define S4 A3  // kanan
#define S5 A4  // kanan luar
#define USE_PULLUP true

// Motor Setup
AF_DCMotor M1(1); // Kiri
AF_DCMotor M2(2); // Kanan

// =====================
// TUNING PARAMETERS (Global Variables)
// =====================
// Hapus 'const' agar bisa diubah lewat Bluetooth

// 1. Motor Speed Base
int baseSpeed   = 235;   
int maxSpeed    = 255;   
int minSpeed    = 100;
int speedBack   = 165;   
int speedPivot  = 185; 

// 2. PID Constants (Default dari tuningan kamu)
float Kp = 0.28;         
float Ki = 0.0004;       
float Kd = 0.35;         

float Kp_small = 0.10;   
float Kd_small = 0.30;   

float Kp_mid = 0.18;     
float Kd_mid = 0.35;     

// 3. Correction Zones
int DEAD_BAND      = 250;   
int MID_BAND       = 700;   
int HARD_BAND      = 1300;  

float OUT_LIM_SOFT = 45.0f;  
float OUT_LIM_MID  = 85.0f;  
float OUT_LIM_HARD = 140.0f; 

// 4. Pivot Confirm
int OUTER_CONFIRM = 2; // Pakai int agar mudah diparsing

// =====================
// INTERNAL VARIABLES
// =====================
bool isLineFollowerMode = true; // Default ON saat nyala

// Sensor Weights
const int w1 = -2000; const int w2 = -1000; const int w3 = 0; const int w4 = 1000; const int w5 = 2000;

// PID Internal
float integral = 0;
float lastError = 0;
unsigned long lastPIDTime = 0;
int lastErrorSign = 0;

// Lost Line Logic
bool lostActive = false;
unsigned long lostStartMs = 0;
int searchMin = 120; int searchMax = 200; int searchRampMs = 400;

// Pivot Logic
unsigned long lastAction = 0;
int minAction = 30;
uint8_t outerL = 0, outerR = 0;


// =====================================================
//   PROTOTYPES
// =====================================================
void sendConfigToAndroid();
void parseConfigString(String data);

// =====================================================
//   SETUP
// =====================================================
void setup() {
  Serial.begin(9600);
  BT.begin(9600);
  BT.setTimeout(20); // Timeout pendek agar responsif

  #if USE_PULLUP
    pinMode(S1, INPUT_PULLUP); pinMode(S2, INPUT_PULLUP); pinMode(S3, INPUT_PULLUP);
    pinMode(S4, INPUT_PULLUP); pinMode(S5, INPUT_PULLUP);
  #else
    pinMode(S1, INPUT); pinMode(S2, INPUT); pinMode(S3, INPUT);
    pinMode(S4, INPUT); pinMode(S5, INPUT);
  #endif
  
  M1.run(RELEASE); M2.run(RELEASE);
  Serial.println("K1bot Ultimate Ready!");
}

// =====================================================
//   MAIN LOOP
// =====================================================
void loop() {
  // 1. Cek Bluetooth
  if (BT.available() > 0) {
    char c = BT.peek(); // Intip karakter pertama

    // --- TEXT COMMAND (Huruf) ---
    if (isAlpha(c)) {
       String cmd = BT.readStringUntil('\n');
       if(cmd == "") cmd = BT.readString(); // Fallback
       cmd.trim();

       // SYSTEM COMMANDS
       if (cmd == "GET_CONFIG") {
         sendConfigToAndroid();
       }
       else if (cmd.startsWith("SET:")) {
         parseConfigString(cmd);
       }
       else if (cmd == "LINE_ON") {
         isLineFollowerMode = true;
         resetPID();
         Serial.println("Mode: Auto");
       }
       else if (cmd == "LINE_OFF") {
         isLineFollowerMode = false;
         stopMotors();
         Serial.println("Mode: Manual");
       }
       
       // MANUAL COMMANDS (Hanya jika manual mode)
       else if (!isLineFollowerMode) {
         processActionButtons(cmd); // Handle RUN, SPIN, dll
       }
    }
    // --- JOYSTICK ANALOG (Angka) ---
    else if (!isLineFollowerMode) {
       int pwmLeft = BT.parseInt();
       int pwmRight = BT.parseInt();
       // Bersihkan sisa buffer
       if (BT.available()) BT.read(); 
       
       setManualSpeed(pwmLeft, pwmRight);
    }
    else {
      // Jika mode Line Follower tapi ada sampah data masuk, bersihkan
      while(BT.available() > 0) BT.read();
    }
  }

  // 2. Jalankan Logika Line Follower (Jika Aktif)
  if (isLineFollowerMode) {
    runLineFollowerLogic();
  }
}

// =====================================================
//   MOTOR FUNCTIONS
// =====================================================
void leftMotor(int pwm, uint8_t dir) { M1.setSpeed(constrain(pwm, 0, 255)); M1.run(dir); }
void rightMotor(int pwm, uint8_t dir) { M2.setSpeed(constrain(pwm, 0, 255)); M2.run(dir); }

void stopMotors() { M1.run(RELEASE); M2.run(RELEASE); }

// Digunakan oleh Line Follower (PID Output)
void forward_drive(int leftPWM, int rightPWM) {
  leftPWM  = constrain(leftPWM,  minSpeed, maxSpeed);
  rightPWM = constrain(rightPWM, minSpeed, maxSpeed);
  leftMotor(leftPWM, FORWARD);
  rightMotor(rightPWM, FORWARD);
}

// Digunakan oleh Joystick Bluetooth
void setManualSpeed(int left, int right) {
  if (left > 0) leftMotor(left, FORWARD);
  else if (left < 0) leftMotor(abs(left), BACKWARD);
  else { M1.run(RELEASE); M1.setSpeed(0); }

  if (right > 0) rightMotor(right, FORWARD);
  else if (right < 0) rightMotor(abs(right), BACKWARD);
  else { M2.run(RELEASE); M2.setSpeed(0); }
}

void pivot_left() { leftMotor(speedBack, BACKWARD); rightMotor(speedPivot, FORWARD); }
void pivot_right() { leftMotor(speedPivot, FORWARD); rightMotor(speedBack, BACKWARD); }
void spin_left_pwm(int sp) { leftMotor(sp, BACKWARD); rightMotor(sp, FORWARD); }
void spin_right_pwm(int sp) { leftMotor(sp, FORWARD); rightMotor(sp, BACKWARD); }

void resetPID() {
  integral = 0; lastError = 0; lastPIDTime = 0; lostActive = false;
}

// =====================================================
//   TUNING COMMUNICATION
// =====================================================
void sendConfigToAndroid() {
  // Format urutan harus SAMA PERSIS dengan di Android (split koma)
  BT.print("CONF:");
  BT.print(Kp, 4); BT.print(","); BT.print(Ki, 5); BT.print(","); BT.print(Kd, 3); BT.print(",");
  BT.print(Kp_small, 3); BT.print(","); BT.print(Kd_small, 3); BT.print(",");
  BT.print(Kp_mid, 3); BT.print(","); BT.print(Kd_mid, 3); BT.print(",");
  BT.print(DEAD_BAND); BT.print(","); BT.print(MID_BAND); BT.print(","); BT.print(HARD_BAND); BT.print(",");
  BT.print(OUT_LIM_SOFT); BT.print(","); BT.print(OUT_LIM_MID); BT.print(","); BT.print(OUT_LIM_HARD); BT.print(",");
  BT.println(OUTER_CONFIRM);
}

void parseConfigString(String data) {
  // Data: "SET:0.28,0.0004,..."
  String params = data.substring(4); // Hapus "SET:"
  char buf[120];
  params.toCharArray(buf, 120);
  
  char* ptr = strtok(buf, ","); if(ptr) Kp = atof(ptr);
  ptr = strtok(NULL, ","); if(ptr) Ki = atof(ptr);
  ptr = strtok(NULL, ","); if(ptr) Kd = atof(ptr);
  
  ptr = strtok(NULL, ","); if(ptr) Kp_small = atof(ptr);
  ptr = strtok(NULL, ","); if(ptr) Kd_small = atof(ptr);
  
  ptr = strtok(NULL, ","); if(ptr) Kp_mid = atof(ptr);
  ptr = strtok(NULL, ","); if(ptr) Kd_mid = atof(ptr);
  
  ptr = strtok(NULL, ","); if(ptr) DEAD_BAND = atoi(ptr);
  ptr = strtok(NULL, ","); if(ptr) MID_BAND = atoi(ptr);
  ptr = strtok(NULL, ","); if(ptr) HARD_BAND = atoi(ptr);
  
  ptr = strtok(NULL, ","); if(ptr) OUT_LIM_SOFT = atof(ptr);
  ptr = strtok(NULL, ","); if(ptr) OUT_LIM_MID = atof(ptr);
  ptr = strtok(NULL, ","); if(ptr) OUT_LIM_HARD = atof(ptr);
  
  ptr = strtok(NULL, ","); if(ptr) OUTER_CONFIRM = atoi(ptr);
  
  Serial.println("Config Updated via Bluetooth!");
  resetPID();
}

// =====================================================
//   LINE FOLLOWER LOGIC (Original Tuned)
// =====================================================
bool readLinePosition(int &position, int &b1, int &b2, int &b3, int &b4, int &b5) {
  b1 = digitalRead(S1); b2 = digitalRead(S2); b3 = digitalRead(S3); b4 = digitalRead(S4); b5 = digitalRead(S5);
  long sum = 0; int count = 0;
  if (b1 == LOW) { sum += w1; count++; } if (b2 == LOW) { sum += w2; count++; }
  if (b3 == LOW) { sum += w3; count++; } if (b4 == LOW) { sum += w4; count++; }
  if (b5 == LOW) { sum += w5; count++; }
  if (count == 0) return false;
  position = (int)(sum / count);
  return true;
}

void PID_control_with_gain_limit(int position, float kp, float ki, float kd, float outLim) {
  float error = position;
  unsigned long now = millis();
  float dt = (lastPIDTime == 0) ? 0.01f : (now - lastPIDTime) / 1000.0f;
  if (dt <= 0 || dt > 0.1f) dt = 0.01f;
  lastPIDTime = now;

  integral += error * dt;
  integral = constrain(integral, -5000.0f, 5000.0f);

  float derivative = (error - lastError) / dt;
  static float lastDerivative = 0;
  derivative = 0.7f * derivative + 0.3f * lastDerivative;
  lastDerivative = derivative;
  lastError = error;

  float output = (kp * error) + (ki * integral) + (kd * derivative);
  output = constrain(output, -outLim, outLim);

  if (error < 0) lastErrorSign = -1; else if (error > 0) lastErrorSign = +1;

  int leftPWM  = (int)(baseSpeed + output);
  int rightPWM = (int)(baseSpeed - output);
  forward_drive(leftPWM, rightPWM);
}

void PID_adaptive(int position) {
  int ap = abs(position);
  if (ap <= DEAD_BAND) PID_control_with_gain_limit(position, Kp_small, Ki, Kd_small, OUT_LIM_SOFT);
  else if (ap <= MID_BAND) PID_control_with_gain_limit(position, Kp_mid, Ki, Kd_mid, OUT_LIM_MID);
  else PID_control_with_gain_limit(position, Kp, Ki, Kd, OUT_LIM_HARD);
}

void runLineFollowerLogic() {
  unsigned long now = millis();
  int b1, b2, b3, b4, b5; int position = 0;
  bool detected = readLinePosition(position, b1, b2, b3, b4, b5);

  if (!detected) {
    if (!lostActive) { lostActive = true; lostStartMs = now; }
    unsigned long t = now - lostStartMs;
    if (t > (unsigned long)searchRampMs) t = searchRampMs;
    int sp = searchMin + (int)((searchMax - searchMin) * (t / (float)searchRampMs));
    sp = constrain(sp, searchMin, searchMax);

    resetPID();
    if (lastErrorSign <= 0) spin_left_pwm(sp); else spin_right_pwm(sp);
    return;
  } else { lostActive = false; }

  if (b1 == LOW) { outerL++; outerR = 0; } else if (b5 == LOW) { outerR++; outerL = 0; } else { outerL = 0; outerR = 0; }

  if (outerL >= OUTER_CONFIRM) {
    resetPID();
    if (now - lastAction > (unsigned long)minAction) { pivot_left(); lastAction = now; }
    return;
  }
  if (outerR >= OUTER_CONFIRM) {
    resetPID();
    if (now - lastAction > (unsigned long)minAction) { pivot_right(); lastAction = now; }
    return;
  }
  PID_adaptive(position);
}

// =====================================================
//   MANUAL ACTION (Emotes & Arrow)
// =====================================================
void processActionButtons(String cmd) {
  // Arrow Keys
  if(cmd == "F") setManualSpeed(230, 230);
  else if(cmd == "B") setManualSpeed(-230, -230);
  else if(cmd == "L") setManualSpeed(-180, 180);
  else if(cmd == "R") setManualSpeed(180, -180);
  else if(cmd == "S") stopMotors();
  
  // Emotes
  else if (cmd == "RUN") { setManualSpeed(255, 255); delay(1000); stopMotors(); }
  else if (cmd == "SPIN") { setManualSpeed(200, -200); delay(2000); stopMotors(); }
  else if (cmd == "SHOCK") { 
    setManualSpeed(-255, -255); delay(200); 
    setManualSpeed(255, 255); delay(200); 
    stopMotors(); 
  }
  else if (cmd == "HAPPY") {
    for(int i=0; i<3; i++) {
       setManualSpeed(180, -180); delay(200);
       setManualSpeed(-180, 180); delay(200);
    }
    stopMotors();
  }
  else if (cmd == "ANGRY") {
    for(int i=0; i<3; i++) {
       setManualSpeed(255, 255); delay(100);
       stopMotors(); delay(50);
    }
  }
  else if (cmd == "THINK") stopMotors();
}