#include <ESP8266WiFi.h>

#ifndef STASSID
#define STASSID "HUAWEI-J6WHTX"
#define STAPSK  "wuchang@888"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;

const char* host = "110.40.236.234";
const uint16_t port = 9999;

void setup() {
  Serial.begin(9600);

  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {
  digitalWrite(0, HIGH);
  digitalWrite(2, HIGH);
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    delay(5000);
    return;
  }

  // This will send a string to the server
  client.print("ojbk");
  while(client.connected()){
    while (client.available()) {
      char ch = static_cast<char>(client.read());
      Serial.print(ch);
    }
  }
}
