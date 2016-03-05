/************************************************
#  This file is to realize using 10086 iot platform
#  control W5100 module and make the light.
#  for HttpPacket.h and ArduinoJson.h you can find it
#  in OneNet website. Good luck for you.
#   
#  Date: 2016/3/3
#  By  : Wi
**************************************************/

// you can define something like ID here, or use them directly in method
//#define DEV_ID 773497
//#define API-KEY dAjZ9gqANMuTdQI=Q27mCkTmCqs=  //master key


#include <SPI.h>
#include <Ethernet.h>
#include <HttpPacket.h>
#include <ArduinoJson.h>

byte mac[] = {0x64, 0x51, 0x06, 0x5d, 0x4c, 0x76};
char server[] = "api.heclouds.com";

byte ip[] = {192, 168, 205, 81};

//String returnValue = ""; 
//boolean ResponseBegin = false;

EthernetClient client;
HttpPacketHead packet;

int connecttocloud()
{
	Ethernet.begin(mac);
	Serial.print("client is at ");
	Serial.println(Ethernet.localIP());

	delay(1000);
	Serial.println("connecting...");
	
	client.connect(server, 80);
  delay(1000);
	if(client.connected())
		return 1;
	else 
		return 0;
}

void createhttppacket()
{
	//make a http request to check the value of button
  packet.setHostAddress("api.heclouds.com");
	packet.setDevId("773039");
	packet.setAccessKey("c3KGtwqLBzQpze3hAOD9xxllSkU=");
 	packet.addUrlParameter("datastream_ids", "lamp0");
	packet.addUrlParameter("datastream_ids", "lamp1");
	packet.addUrlParameter("datastream_ids", "lamp2");
	packet.createCmdPacket(GET, TYPE_DATASTREAM);	
	if (strlen(packet.content))
	{
		Serial.println(packet.content);
	}
}

void setup()
{
  pinMode(13, OUTPUT);  //for testing
  Serial.begin(9600);
  while(!Serial);
   
	while(!connecttocloud())    //wait until connection succ
  {
    Serial.println("The connection failed, reconnecting ...");
  }
	createhttppacket();       //create a http request packet
}

void loop()
{
	char json_response[1000];
  boolean beginread = false;
  int i = 0;
	if (client.connected())
	{
		Serial.println("Connected to OneNet");
		
		client.print(packet.content);
		delay(500);
		while(client.available())
		{ 
      char ch = client.read();
      if (ch == '{') 
        beginread = true;
      if (beginread) json_response[i++] = ch;
		}			
    json_response[i] = '\0';
    Serial.println("Return Value:");
    Serial.println(json_response);
      
    StaticJsonBuffer<1000> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(json_response);
    if (!root.success())
      Serial.println("parseObject() failed");		
    else
    {
      for(int i=0;i<3; i++)
      {
         const char* id = root["data"][i]["id"];
         int id_value = root["data"][i]["current_value"];
    
         Serial.print(id);
         Serial.print("\t");
         Serial.println(id_value);
      }
      Serial.println("\n");
    }
	}
	else
	{
	    // if you didn't get a connection to the server:
	    Serial.println("connection closed");
	    Serial.println("disconnecting...");
	    client.stop();
	}
	delay(1994);
  while(1);
}
