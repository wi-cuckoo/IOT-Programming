#include <SoftwareSerial.h> 

/****************************** ESP8266串口及状态定义 start ******************************/
#define BRUDRATE 9600
#define DEBUG true
#define MAX_BUF_LEN 128

#define ESP8266_RX 2
#define ESP8266_TX 3

// SP8266 Station 接⼝的状态定义
#define CONNECTED_WIFI    0x32
#define ESTABLISH_TCP     0x33
#define CLOSED_TCP        0x34
#define NOT_CONNECT_WIFI  0x35
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
  esp8266.begin(BRUDRATE);

  init_esp8266();
}

void loop() {
  while(true)
  {
    debug("ask to subscribe");
    subscribe();
    debug("recv from server");
    read_until('+');
    char buf[16];
    int read_len = esp8266.readBytesUntil('\n', buf, 16);
    Serial.write(buf, read_len);
    
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print(" run motor with cmd:");
    Serial.println(buf[6]);
    run_motor(buf[6]);
    digitalWrite(LED_BUILTIN, LOW);
  }
}


/****************************** esp8266串口通讯操作 start ******************************/

// 初始化esp8266模块，包括连wifi与云服务器
void init_esp8266(){
  delay(1000);
  debug("1. check wifi connection");
  while(!connected_wifi())
  {
    debug("not connected wifi, trying connecting...");
    delay(1000);
  }

  delay(1000);
  debug("2. check cloud server connection");
  while(!connected_server())
  {
    debug("not connected server, trying connecting...");
    connecting_server();
    delay(2000);
  }
  
  debug("3. establish connection with server");
  delay(1000);
}

// 从串口读取数据直到读到字符 c 为止退出
void read_until(char c){
  while(esp8266.read() != c);
}

// 查询 Station 接⼝的状态
byte station_status(){
    const unsigned int buf_len = 25;
    const unsigned int stat_idx = 22;
    esp8266.print("AT+CIPSTATUS\r\n");
    esp8266.setTimeout(1000); //1s
    byte buf[MAX_BUF_LEN];
    esp8266.readBytes(buf, MAX_BUF_LEN);
    if (DEBUG)
    {
      Serial.write(buf, buf_len);
    }


    return buf[stat_idx];
}

// 查询是否连接了wifi
bool connected_wifi(){
    return station_status() != NOT_CONNECT_WIFI;
}

// 查询是否连接了服务器
bool connected_server(){
    return station_status() == ESTABLISH_TCP;
}

// 连接到云端服务器
void connecting_server(){
    String cmd = "AT+CIPSTART=\"TCP\",\"192.168.3.39\",9999\r\n";
    esp8266.print(cmd);

    long int now_ms = millis();
    while((now_ms+3000)>millis())
      while(esp8266.available())
        Serial.write(esp8266.read()); 
}

// 订阅一个命令
void subscribe(){
    String cmd = "AT+CIPSEND=4\r\n";
    esp8266.print(cmd);
    read_until('>');
    esp8266.print("fuck");

    read_until('\n');
    read_until('\n');
    read_until('\n');
    read_until('\n');
    read_until('\n');
}

// 输出调试日志到控制台
void debug(String str){
  if (DEBUG)
  {
    Serial.println(">>>>>>>>> "+str+" <<<<<<<<<");
  }
}

/****************************** esp8266串口通讯操作 end ******************************/


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
