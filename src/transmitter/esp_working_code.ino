#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include "MPU6050.h"

MPU6050 mpu;

uint8_t receiverMac[] = {0x80,0xF3,0xDA,0x50,0xDC,0xA4};  // change

typedef struct struct_message {
  uint8_t x;
  uint8_t y;
  uint8_t z;
} struct_message;

struct_message sensorData;

void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// Mapping function
uint8_t convertToByte(int16_t value) {

  // constrain to safe range
  if (value < -16384) value = -16384;
  if (value >  16384) value =  16384;

  // shift range (-16384 → +16384) to (0 → 32768)
  value = value + 16384;

  // scale to 0 → 254
  return (uint8_t)((value * 254) / 32768);
}

void setup() {
  Serial.begin(115200);

  Wire.begin();
  mpu.initialize();

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  esp_now_init();
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {

  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  sensorData.x = convertToByte(ax);
  sensorData.y = convertToByte(ay);
  sensorData.z = convertToByte(az);

  esp_now_send(receiverMac, (uint8_t *)&sensorData, sizeof(sensorData));

  delay(20);
}
