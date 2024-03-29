#include <SoftwareSerial.h> 

/****************************** ESP8266串口及状态定义 start ******************************/
#define BRUDRATE 9600
#define ESP8266_RX 2
#define ESP8266_TX 3
/****************************** ESP8266串口及状态定义 end ******************************/

/****************************** 电机驱动引脚及状态定义 start ******************************/
// 左边L298N输入引脚定义
int L_IN1 = 4;
int L_IN2 = 5;
int L_IN3 = 6;
int L_IN4 = 7;
// 右边L298N输入引脚定义
int R_IN1 = 8;
int R_IN2 = 9;
int R_IN3 = 10;
int R_IN4 = 11;

#define IN_SIZE 8
/****************************** 电机驱动引脚及状态定义 ed ******************************/

int IN_LIST[IN_SIZE] = {L_IN1, L_IN2, L_IN3, L_IN4, R_IN1, R_IN2, R_IN3, R_IN4 };
char FW_STATE = 0b10011010;
SoftwareSerial esp8266(2,3); 

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < IN_SIZE; i++) {
    pinMode(IN_LIST[i], OUTPUT);
  }
  
  Serial.begin(BRUDRATE);
}

void loop() {
  while(Serial.available()){
    char c = Serial.read();
    run_motor(c);
  }
}


/****************************** 小车运动函数定义 start ******************************/
void run_motor(char c) {
  switch (c) {
    case 'F':
      forward();
      break;
    case 'B':
      backward();
      break;
    case 'R':
      turn_right();
      break;
    case 'L':
      turn_left();
      break;
    default:
      _stop();
      return;
  }
  delay(100);
  _stop();
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

/****************************** 小车运动函数定义 end ******************************/
