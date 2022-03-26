#include <SoftwareSerial.h>                         //including the SoftwareSerial library will allow you to use the pin no. 2,3 as Rx, Tx.

SoftwareSerial esp8266(2,3);                        //set the Rx ==> Pin 2; TX ==> Pin3.

#define BRUDRATE 9600
#define DEBUG true
#define MAX_BUF_LEN 128

// SP8266 Station 接⼝的状态定义
#define CONNECTED_WIFI    0x32
#define ESTABLISH_TCP     0x33
#define CLOSED_TCP        0x34
#define NOT_CONNECT_WIFI  0x35

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(BRUDRATE);           //begin the Hardware serial communication (0, 1) at speed 9600.
  esp8266.begin(BRUDRATE);          //begin the software serial communication (2, 3) at speed 9600.

  InitESP8266();
}

void loop()
{
  while(true)
  {
    debug("ask to subscribe");
    subscribe();
    debug("recv from server");
    read_until('+');
    char buf[16];
    int read_len = esp8266.readBytesUntil('\n', buf, 16);
    Serial.write(buf, read_len);
    switch (buf[6])
    {
      case 'L':
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
    }
    delay(1000);
  }
  delay(60000);
}

// clear_esp8266_buffer 清空 esp8266 串口未读取的数据，避免下一次读到脏数据
void clear_esp8266_buffer()
{
  while(esp8266.available())
    esp8266.read();
}

void read_until(char c)
{
  while(esp8266.read() != c);
}

// 查询 Station 接⼝的状态
byte station_status()
{
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
bool connected_wifi()
{
    return station_status() != NOT_CONNECT_WIFI;
}

// 查询是否连接了服务器
bool connected_server()
{
    return station_status() == ESTABLISH_TCP;
}

// 连接到云端服务器
void connecting_server()
{
    String cmd = "AT+CIPSTART=\"TCP\",\"192.168.3.39\",9999\r\n";
    esp8266.print(cmd);

    long int now_ms = millis();
    while((now_ms+3000)>millis())
      while(esp8266.available())
        Serial.write(esp8266.read()); 
}

byte subscribe()
{
    String cmd = "AT+CIPSEND=4\r\n";
    esp8266.print(cmd);
    read_until('>');
    esp8266.print("fuck");
}

void InitESP8266()
{
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
  debug("4. ready to communciate with server");
}

void debug(String str)
{
  if (DEBUG)
  {
    Serial.println(">>>>>>>>> "+str+" <<<<<<<<<");
  }
}
