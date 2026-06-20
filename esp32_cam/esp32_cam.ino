#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>

// ================= WIFI =================
const char* ssid = "Hào Hùng Phát";
const char* password = "haohungle";

// const char* ssid = "Hùnn";
// const char* password = "11223301";

// TELEGRAM
String BOT_TOKEN = "8163882798:AAH99a5iqdJ666oAPlRc-OJ1tBlQP8B60Jk";
String CHAT_ID = "5920860822";

WebServer server(80);

// ===== CAMERA CONFIG =====
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

void sendToAI(String name, String timeStr) {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  WiFiClient client;
  client.setTimeout(5000);  // 🔥 FIX

  String server = "192.168.16.107";
  int port = 5000;

  if (!client.connect(server.c_str(), port)) {
    Serial.println("Connect AI Server FAILED");
    esp_camera_fb_return(fb);
    return;
  }

  String boundary = "----AI123456";

  String head = "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"file\"; filename=\"img.jpg\"\r\n";
  head += "Content-Type: image/jpeg\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";

  uint32_t contentLength = head.length() + fb->len + tail.length();

  client.println("POST /analyze HTTP/1.1");
  client.println("Host: " + server);
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println("Content-Length: " + String(contentLength));
  client.println();

  client.print(head);
  client.write(fb->buf, fb->len);
  client.print(tail);

  Serial.println("Sent image to AI");

  // ===== RECEIVE =====
  String response = "";
  unsigned long startTime = millis();

  while (client.connected() && millis() - startTime < 5000) {
    while (client.available()) {
      response += (char)client.read();
    }
  }

  Serial.println("AI RESPONSE:");
  Serial.println(response);

  // ===== PARSE =====
  String emotion = "unknown";

  int index = response.indexOf("emotion");
  if (index != -1) {
    int start = response.indexOf(":", index) + 2;
    int end = response.indexOf("\"", start);
    emotion = response.substring(start, end);
  }

  Serial.println("Emotion: " + emotion);

  // ===== TELEGRAM =====
  String msg = "AI Emotion: " + emotion;
  msg.replace(" ", "%20");

  WiFiClientSecure client2;
  client2.setInsecure();

  if (client2.connect("api.telegram.org", 443)) {
    String url = "/bot" + BOT_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + msg;

    client2.println("GET " + url + " HTTP/1.1");
    client2.println("Host: api.telegram.org");
    client2.println("Connection: close");
    client2.println();
  }

  client.stop();
  esp_camera_fb_return(fb);

  delay(300); // 🔥 chống treo
}

// ================= SEND PHOTO =================
void sendPhotoTelegram(String caption) {

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return;

  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect("api.telegram.org", 443)) {
    esp_camera_fb_return(fb);
    return;
  }

  String boundary = "----123456";

  String head = "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + CHAT_ID + "\r\n";

  String cap = "--" + boundary + "\r\n";
  cap += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n" + caption + "\r\n";

  String fileHead = "--" + boundary + "\r\n";
  fileHead += "Content-Disposition: form-data; name=\"photo\"; filename=\"img.jpg\"\r\n";
  fileHead += "Content-Type: image/jpeg\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";

  uint32_t contentLength = head.length() + cap.length() + fileHead.length() + fb->len + tail.length();

  client.println("POST /bot" + BOT_TOKEN + "/sendPhoto HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println("Content-Length: " + String(contentLength));
  client.println();

  client.print(head);
  client.print(cap);
  client.print(fileHead);
  client.write(fb->buf, fb->len);
  client.print(tail);

  delay(1000);
  client.stop();
  esp_camera_fb_return(fb);
}

// ================= HANDLE =================
void handleCapture() {
  Serial.println("CAPTURE TRIGGERED");

  String name = server.arg("name");
  String time = server.arg("time");

  String caption = "Da diem danh(ESP32-CAM):\n" + name + "\nTime: " + time;

  sendPhotoTelegram(caption);
  sendToAI(name, time);   // 🔥 Gửi AI trước

  server.send(200, "text/plain", "OK");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

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

  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_camera_init(&config);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  Serial.println(WiFi.localIP());

  server.on("/capture", handleCapture);
  server.begin();
}

void loop() {
  server.handleClient();
}