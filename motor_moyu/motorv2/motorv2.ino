#include <ESP8266WiFi.h>

#ifndef STASSID
#define STASSID "HUAWEI-J6WHTX"
#define STAPSK  "wuchang@888"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;

const char* host = "110.40.236.234";
const uint16_t port = 9999;

/****************************** 电机驱动引脚及状态定义 start ******************************/
#define IN_SIZE 4
#define FW_STATE 0b0110
// L298N输入引脚定义
int IN1 = 0;
int IN2 = 1;
int IN3 = 2;
int IN4 = 3;
int IN_LIST[IN_SIZE] = {IN1, IN2, IN3, IN4};
/****************************** 电机驱动引脚及状态定义 ed ******************************/

void setup() {
  for (int i = 0; i < IN_SIZE; i++) {
    pinMode(IN_LIST[i], OUTPUT);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}


void loop() {
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, port)) {
    delay(5000);
    return;
  }

  // This will send a string to the server
  client.print("ojbk");
  while(client.connected()){
    while (client.available()) {
      char ch = static_cast<char>(client.read());
      run_motor(ch);
    }
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
