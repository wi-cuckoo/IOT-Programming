//#include <ESP8266WiFi.h>
#define IN_SIZE 8

// 左边L298N输入引脚定义
int L_IN1 = 3;
int L_IN2 = 4;
int L_IN3 = 5;
int L_IN4 = 6;
// 右边L298N输入引脚定义
int R_IN1 = 7;
int R_IN2 = 8;
int R_IN3 = 9;
int R_IN4 = 10;

int IN_LIST[IN_SIZE] = {L_IN1, L_IN2, L_IN3, L_IN4, R_IN1, R_IN2, R_IN3, R_IN4 };
char FW_STATE = 0b10011010;

void setup() {
  for (int i = 0; i < IN_SIZE; i++) {
    pinMode(IN_LIST[i], OUTPUT);
  }

  Serial.begin(115200);
  while(!Serial);
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'f':
        forward();
        break;
      case 'b':
        backward();
        break;
      case 'r':
        turn_right();
        break;
      case 'l':
        turn_left();
        break;
    }
    delay(100);
    _stop();
  }
}

void forward() {
  for (int i = 0; i < IN_SIZE; i++) {
    int state = FW_STATE >> (IN_SIZE - i - 1) & 0x01;
    digitalWrite(IN_LIST[i], state);
  }
}

void _stop() {
  for (int i = 0; i < IN_SIZE; i++) {
    digitalWrite(IN_LIST[i], LOW);
  }
}

void backward() {
  for (int i = 0; i < IN_SIZE; i++) {
    int state = FW_STATE >> (IN_SIZE - i - 1) & 0x01;
    digitalWrite(IN_LIST[i], HIGH - state);
  }
}

void turn_left() {
  for (int i = 0; i < IN_SIZE; i++) {
    int state = FW_STATE >> (IN_SIZE - i - 1) & 0x01;
    if (i < IN_SIZE / 2) {
      digitalWrite(IN_LIST[i], HIGH - state);
    } else {
      digitalWrite(IN_LIST[i], state);
    }
  }
}

void turn_right() {
  for (int i = 0; i < IN_SIZE; i++) {
    int state = FW_STATE >> (IN_SIZE - i - 1) & 0x01;
    if (i < IN_SIZE / 2) {
      digitalWrite(IN_LIST[i], state);
    } else {
      digitalWrite(IN_LIST[i], HIGH - state);
    }
  }
}
