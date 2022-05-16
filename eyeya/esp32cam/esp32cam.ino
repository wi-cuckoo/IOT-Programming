#include "esp_camera.h"
#include <WiFi.h>
#include "camera_pins.h"


/****************************** 电机驱动引脚及状态定义 start ******************************/
#define IN_SIZE 4
#define FW_STATE 0b0110
// L298N输入引脚定义
int IN1 = 12;
int IN2 = 13;
int IN3 = 14;
int IN4 = 15;
int IN_LIST[IN_SIZE] = {IN1, IN2, IN3, IN4};
/****************************** 电机驱动引脚及状态定义 end ******************************/


/****************************** wifi&server start ******************************/
const char* ssid = "HUAWEI-B91W7Y";
const char* password = "liangzhu@888";
const char* host = "110.40.236.234";
const uint16_t port = 9999;
/****************************** wifi&server end ******************************/

void setup() {
  for (int i = 0; i < IN_SIZE; i++) {
    pinMode(IN_LIST[i], OUTPUT);
  }

  Serial.begin(115200);
  Serial.println("begin to initialize camera...");
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("got ip: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  WiFiClient client;
  if (!client.connect(host, port)) {
    delay(5000);
    return;
  }

  Serial.println("connected to remote host");
  // say ojbk!
  client.print("ojbk");

  bool opened = false;
  while(client.connected()){
    if (opened) {
      // capture camera frame
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
            Serial.println("esp_camera_fb_get failed");
            delay(1000);
            continue;
      }
      Serial.printf("send one camera frame with len %d\r\n", fb->len);
      // while(!client.availableForWrite());
      // write frame len
      client.write((fb->len >> 24)&0xff);
      client.write((fb->len >> 16)&0xff);
      client.write((fb->len >> 8)&0xff);
      client.write(fb->len&0xff);
      // write frame buffer
      client.write((const char *)fb->buf, fb->len);
      if(fb){
        esp_camera_fb_return(fb);
        fb = NULL;
      }
    }
    // wait for command
    if (client.available()) {
      char c = client.read();
      Serial.printf("got command: %c\n", c);
      if (c == 'O') {
        opened = true;
      } else if (c == 'C') {
        opened = false;
      } else {
        run_motor(c);
      }
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
