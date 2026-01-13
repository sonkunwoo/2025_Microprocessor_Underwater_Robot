#include <ESP32Servo.h>

const int servoPin1 = 25;
const int servoPin2 = 26;
const int servoPin3 = 27;
const int servoPin4 = 13;

const int relayPin1 = 21;
const int relayPin2 = 22;

Servo servo1, servo2, servo3, servo4;

bool relayState1 = false;
bool relayState2 = false;

bool moveW = false, moveX = false, moveA = false, moveD = false;
bool moveQ = false, moveE = false, moveR = false, moveG = false;

float timeTick = 0.0;
float freqGlobal = 0.5;
float freqLeftA = 0.5;
float freqRightA = 0.5;
float freqLeftD = 0.5;
float freqRightD = 0.5;
float amp = 45.0;
float phaseShift = PI / 2.0;

const float freqMin = 0.1;
const float freqMax = 3.0;

volatile int servoSpeedDelay = 20;
const int speedMin = 5;
const int speedMax = 50;
const int forbiddenDelay = 40;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void setup() {
  Serial.begin(115200);

  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  digitalWrite(relayPin1, LOW);
  digitalWrite(relayPin2, LOW);

  servo1.attach(servoPin1, 500, 2400);
  servo2.attach(servoPin2, 500, 2400);
  servo3.attach(servoPin3, 500, 2400);
  servo4.attach(servoPin4, 500, 2400);

  xTaskCreatePinnedToCore(servoTask, "ServoControl", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(relayTask, "RelayControl", 4096, NULL, 1, NULL, 1);

  Serial.println("-- 마이크로프로세서 수중로봇 V2.8.1 (최종 시연본) --");
  Serial.println("1/2: 릴레이 토글 | W/X/A/D/Z/C/R: 모터 | S: 정지 | G: 로봇 접기(사이즈)");
  Serial.println("+/- : 전체 동작 속도 증가/감소");
  Serial.println("++/-- : 좌측 모터 속도 증가/감소 (A, D 시 개별)");
  Serial.println("+++/--- : 우측 모터 속도 증가/감소 (A, D 시 개별)");
}

void loop() {
  delay(1000);
}

void servoTask(void *parameter) {
  for (;;) {
    portENTER_CRITICAL(&mux);
    timeTick += 0.05;
    if (timeTick >= 10000.0) timeTick = 0;
    portEXIT_CRITICAL(&mux);

    if (moveG) {
      servo1.write(20);
      servo2.write(160);
      servo3.write(160);
      servo4.write(20);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }

    float omegaL, omegaR;
    if (moveA) {
      omegaL = 2 * PI * freqLeftA;
      omegaR = 2 * PI * freqRightA;
    } else if (moveD) {
      omegaL = 2 * PI * freqLeftD;
      omegaR = 2 * PI * freqRightD;
    } else {
      omegaL = 2 * PI * freqGlobal;
      omegaR = 2 * PI * freqGlobal;
    }

    float qL1 = amp * sin(omegaL * timeTick);
    float qL2 = amp * sin(omegaL * timeTick + phaseShift);
    float qR1 = amp * sin(omegaR * timeTick);
    float qR2 = amp * sin(omegaR * timeTick + phaseShift);

    int a1 = 90, a2 = 90, a3 = 90, a4 = 90;

    if (moveW) { a1 -= qL1; a2 -= qL2; a3 += qR1; a4 += qR2; }
    else if (moveX) { a1 += qL1; a2 += qL2; a3 -= qR1; a4 -= qR2; }
    else if (moveA) { a1 -= qL1; a2 -= qL2; a3 -= qR1; a4 -= qR2; }
    else if (moveD) { a1 += qL1; a2 += qL2; a3 += qR1; a4 += qR2; }
    else if (moveQ) { a1 += qL1; a2 += qL2; }
    else if (moveE) { a3 += qR1; a4 += qR2; }
    else if (moveR) { a1 += qL1; a2 -= qL2; a3 += qR1; a4 -= qR2; }

    servo1.write(constrain(a1, 0, 180));
    servo2.write(constrain(a2, 0, 180));
    servo3.write(constrain(a3, 0, 180));
    servo4.write(constrain(a4, 0, 180));

    vTaskDelay(servoSpeedDelay / portTICK_PERIOD_MS);
  }
}

void relayTask(void *parameter) {
  for (;;) {
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();

      if (cmd == "+") {
        freqGlobal = constrain(freqGlobal + 0.1, freqMin, freqMax);
        Serial.println("GLOBAL_FREQ: " + String(freqGlobal, 2));
      }
      else if (cmd == "-") {
        freqGlobal = constrain(freqGlobal - 0.1, freqMin, freqMax);
        Serial.println("GLOBAL_FREQ: " + String(freqGlobal, 2));
      }
      else if (cmd == "++") {
        if (moveA)
          freqLeftA = constrain(freqLeftA + 0.1, freqMin, freqMax);
        else if (moveD)
          freqLeftD = constrain(freqLeftD + 0.1, freqMin, freqMax);
        else
          freqGlobal = constrain(freqGlobal + 0.1, freqMin, freqMax);
        Serial.println("FREQ_LEFT:" + String(freqGlobal, 2));
      }
      else if (cmd == "+++") {
        if (moveA)
          freqRightA = constrain(freqRightA + 0.1, freqMin, freqMax);
        else if (moveD)
          freqRightD = constrain(freqRightD + 0.1, freqMin, freqMax);
        else
          freqGlobal = constrain(freqGlobal + 0.1, freqMin, freqMax);
        Serial.println("FREQ_RIGHT:" + String(freqGlobal, 2));
      }
      else if (cmd == "--") {
        if (moveA)
          freqLeftA = constrain(freqLeftA - 0.1, freqMin, freqMax);
        else if (moveD)
          freqLeftD = constrain(freqLeftD - 0.1, freqMin, freqMax);
        else
          freqGlobal = constrain(freqGlobal - 0.1, freqMin, freqMax);
        Serial.println("FREQ_LEFT:" + String(freqGlobal, 2));
      }
      else if (cmd == "---") {
        if (moveA)
          freqRightA = constrain(freqRightA - 0.1, freqMin, freqMax);
        else if (moveD)
          freqRightD = constrain(freqRightD - 0.1, freqMin, freqMax);
        else
          freqGlobal = constrain(freqGlobal - 0.1, freqMin, freqMax);
        Serial.println("FREQ_RIGHT:" + String(freqGlobal, 2));
      }
      else {
        moveW = moveX = moveA = moveD = moveQ = moveE = moveR = moveG = false;

        if (cmd.equalsIgnoreCase("W")) moveW = true;
        else if (cmd.equalsIgnoreCase("X")) moveX = true;
        else if (cmd.equalsIgnoreCase("A")) moveA = true;
        else if (cmd.equalsIgnoreCase("D")) moveD = true;
        else if (cmd.equalsIgnoreCase("Q")) moveQ = true;
        else if (cmd.equalsIgnoreCase("E")) moveE = true;
        else if (cmd.equalsIgnoreCase("R")) moveR = true;
        else if (cmd.equalsIgnoreCase("G")) moveG = true;
        else if (cmd.equalsIgnoreCase("S")) { /* 정지 */ }
        else if (cmd == "1") {
          relayState1 = !relayState1;
          digitalWrite(relayPin1, relayState1 ? HIGH : LOW);
        }
        else if (cmd == "2") {
          relayState2 = !relayState2;
          digitalWrite(relayPin2, relayState2 ? HIGH : LOW);
        }

        Serial.println("OK");
      }
    }

    delay(10);
  }
}
