/************************************************
#  This file is to realize using 10086 iot platform
#  control W5100 module and make the light.
#   
#  Date: 2016/3/3
#  By  : Wi
**************************************************/

#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include "edp.c"          //import edp.c which define some methods to deal with EDP protocol.

//define two key value: device_id and api-key(master)
#define DEV_ID "773497"
#define API_KEY "1qHWLbTgkOUbQdpgE2zm26V0nc4="  //master key

typedef unsigned char uchar;

byte mac[] = {0x64, 0x51, 0x06, 0x5d, 0x4c, 0x76};
byte server[] = {183, 230, 40, 39};
edp_pkt *pkt;

EthernetClient client;

/*
 * method: connect2cloud()
 * describe: Using Ethernet lib provided by Arduino, control the W5100 to realize 
 * connect to OneNet via Internet. 
 * return value: return 1 if connected or 0
 */
int connect2cloud()
{
	Ethernet.begin(mac);
	Serial.print("Client is at ");
	Serial.println(Ethernet.localIP());

	delay(100);
	Serial.println("The device is connecting to OneNet ...");
	if(client.connect(server, 876))
	{
		Serial.println("Connected");
		return 1;
	}
	else
		Serial.println("Connection failed");
	return 0;
}

/*
 * method: devicelogin()
 * describe: Base on successful and steady Internet connection, device can login OneNet with unique
 * device_id and api-key. And it will judge if login operation done by analysing return data.
 * return value: return 1 if login ok or 0
 */
int devicelogin()
{ 
    uchar res[200];
    int i = 0;
    while(client.available()) client.read();     //clear serial buffer
    packetSend(packetConnect(DEV_ID, API_KEY));             //发送EPD连接包
    while(!client.available());       //wait for response 
    while(client.available())
    {
      res[i++] = (uchar)client.read();
    }         
    if (res[0] == 0x20 && res[2] == 0x00 && res[3] == 0x00)
    {
      Serial.println("The device login successfully via EDP protocol.");
      return 1;
    }
    else
      Serial.println("EDP connect error.");

    return 0;
}

void setup()
{

	pinMode(22, OUTPUT);  //for testing

	Serial.begin(9600);
	while(!Serial);
 // To connect till success
	while(!connect2cloud())
		Serial.println("Waiting a minute, Reconnecting ...");
 // To login till success
  while(!devicelogin())
    Serial.println("Login failed, Re-login...");
}

void loop()
{
	static int edp_connect = 0;
	edp_pkt rec_pkt;
	int i=0;

  // listen to Ethernet port if there is income message
	if (client.available())   
	{
  //read the income message and analyse it
		while(client.available())  
		{
      uchar ch = (uchar)client.read();
		  rec_pkt.data[i++] = ch;
		}
		parseCmdData(rec_pkt);
	}

	if (rec_pkt.len >= 0)
		packetClear(&rec_pkt);

  // Send heart beat to keep connection alive
  keepliving();
}

/*
 * parseCmdData(): human readable string
 * describe: income data should be encoded with json, So it will be decode reversely
 * return value: None
 */

void parseCmdData(edp_pkt rec_pkt)
{
  unsigned char pkt_type;
  if (rec_pkt.data)
    {
      pkt_type = rec_pkt.data[0];
      if (pkt_type == CMDREQ)
      {
        char edp_command[60];
        char edp_cmd_id[40];
        long id_len, cmd_len, rm_len;
        
        memset(edp_command, 0, sizeof(edp_command));
        memset(edp_cmd_id, 0, sizeof(edp_cmd_id));
        edpCommandReqParse(&rec_pkt, edp_cmd_id, edp_command, &rm_len, &id_len, &cmd_len);
        Serial.print("id_len: ");
        Serial.println(id_len, DEC);
        delay(10);
        Serial.print("cmd_len: ");
        Serial.println(cmd_len, DEC);
        delay(10);
        Serial.print("id: ");
        Serial.println(edp_cmd_id);
        delay(10);
        Serial.print("cmd: ");
        Serial.println(edp_command);

        StaticJsonBuffer<60> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(edp_command);
        if (!root.success())
          Serial.println("parseObject() failed");
        else
        {
            const char* dt_id = root["id"];
            const char* val = root["value"];

            if (atoi(val) == 0)
              digitalWrite(22, LOW);
            else
              digitalWrite(22, HIGH);

//        packetSend(packetDataSaveTrans(DEV_ID, datastr, value));  //将新数据值上传至数据流
        }
      }
   }
}

/*
 * keepliving(): to send ping data 
 * describe: According to EDP packet format, send correct PING_REQ data,
 * the interval time will less than 4 mins. So I set it to 220000ms
 * return value: None
 */
void keepliving()
{
  static unsigned long now = 0;
  if (millis() - now > 220000)
  {
    now = millis();
    packetSend(packetPing());
    delay(200);
    while(client.available()) Serial.println((uchar)client.read());      //clear serial buffer
  }
}
/*
 * packetSend
 * 将待发数据发送至串口，并释放到动态分配的内存
 */
void packetSend(edp_pkt* pkt)
{
	if (pkt != NULL)
	{
    		client.write(pkt->data, pkt->len);    //Send packet via Ethernet Port
    		client.flush();
    		free(pkt);              //free memory
  	}
}
