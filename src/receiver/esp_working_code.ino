#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ========== MOTOR PINS ==========
#define rightMotorPin1  14
#define rightMotorPin2  27
#define leftMotorPin1   26
#define leftMotorPin2   25

#define enableRightMotor 33
#define enableLeftMotor  32
// =================================
int maxStep = 10;   // how fast speed can change per loop
typedef struct struct_message {
  uint8_t x;
  uint8_t y;
  uint8_t z;
} struct_message;

struct_message incomingData;

int currentRight = 0;
int currentLeft  = 0;

int normalize(uint8_t value)
{
  return (int)value - 127;
}

void rotateMotor(int rightMotorSpeed, int leftMotorSpeed)
{
  rightMotorSpeed = constrain(rightMotorSpeed, -255, 255);
  leftMotorSpeed  = constrain(leftMotorSpeed,  -255, 255);

  // 🔥 Reverse Boost Compensation
  if (rightMotorSpeed < 0)
    rightMotorSpeed *= 1.2;   // 20% boost in reverse

  if (leftMotorSpeed < 0)
    leftMotorSpeed *= 1.2;

  rightMotorSpeed = constrain(rightMotorSpeed, -255, 255);
  leftMotorSpeed  = constrain(leftMotorSpeed,  -255, 255);

  digitalWrite(rightMotorPin1, rightMotorSpeed > 0);
  digitalWrite(rightMotorPin2, rightMotorSpeed < 0);

  digitalWrite(leftMotorPin1, leftMotorSpeed > 0);
  digitalWrite(leftMotorPin2, leftMotorSpeed < 0);

  ledcWrite(enableRightMotor, abs(rightMotorSpeed));
  ledcWrite(enableLeftMotor,  abs(leftMotorSpeed));
}
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
  memcpy(&incomingData, data, sizeof(incomingData));

  int throttle = normalize(incomingData.x);
  int steering = normalize(incomingData.y);

  // Bigger deadzone
  if (abs(throttle) < 25) throttle = 0;
  if (abs(steering) < 35) steering = 0;

  // 🔥 Reduce steering sensitivity (prevents twitch)
  steering *= 0.6;

  int targetRight = throttle - steering;
  int targetLeft  = throttle + steering;

  targetRight *= 2;
  targetLeft  *= 2;

  // 🔥 Smoothing
  currentRight = currentRight * 0.75 + targetRight * 0.25;
  currentLeft  = currentLeft  * 0.75 + targetLeft  * 0.25;

  // 🔥 Minimum motor threshold (prevents stutter)
  if (currentRight != 0 && abs(currentRight) < 40)
      currentRight = (currentRight > 0) ? 40 : -40;

  if (currentLeft != 0 && abs(currentLeft) < 40)
      currentLeft = (currentLeft > 0) ? 40 : -40;

  rotateMotor(currentRight, currentLeft);
}

void setup()
{
  Serial.begin(115200);

  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);

  // 🔥 20kHz PWM (silent)
  ledcAttach(enableRightMotor, 20000, 8);
  ledcAttach(enableLeftMotor,  20000, 8);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("🚗 Smooth Gesture Car Ready!");
}

void loop() {}
