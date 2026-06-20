#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <WiFiClient.h>   // 🔥 THÊM

// ================= WIFI =================
const char* ssid = "Hào Hùng Phát";
const char* password = "haohungle";

// const char* ssid = "Hùnn";
// const char* password = "11223301";

// ================= ESP32-CAM =================
// 🔥 THÊM MỚI: IP của ESP32-CAM
String ESP32CAM_IP = "192.168.16.108";  // <-- sửa theo IP thật

// ================= TELEGRAM =================
String BOT_TOKEN = "8163882798:AAH99a5iqdJ666oAPlRc-OJ1tBlQP8B60Jk";
String CHAT_ID = "5920860822";

// ================= NTP =================
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

// ================= GPIO =================
#define FP_RX 16
#define FP_TX 17

#define I2C_SDA 21
#define I2C_SCL 22

#define LED_OK 26
#define LED_FAIL 25
#define LED_BLUE 2

// ================= CHỐNG QUÉT =================
#define MAX_ID 50
unsigned long timeList[MAX_ID];
const unsigned long blockTime = 24000;

// ================= OBJECT =================
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= NAME =================
String getName(int id) {
  switch(id) {
    case 1: return "Le Nguyen Manh Hung";
    case 2: return "Le Quang Huan";
    case 3: return "Pham Minh Quan";
    case 4: return "Nguyen Thi Nhung";
    default: return "Khong xac dinh!";
  }
}

// ================= TIME =================
String getTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "No Time";

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

// ================= URL ENCODE (🔥 FIX) =================
String urlEncode(String str) {
  String encoded = "";
  char c;
  char buf[4];

  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);

    if (isalnum(c)) encoded += c;
    else if (c == ' ') encoded += "%20";
    else if (c == '/') encoded += "%2F";
    else if (c == ':') encoded += "%3A";
    else {
      sprintf(buf, "%%%02X", c);
      encoded += buf;
    }
  }
  return encoded;
}

// ================= TELEGRAM =================
void sendTelegram(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage";
    String data = "chat_id=" + CHAT_ID + "&text=" + message;

    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.POST(data);
    http.end();
  }
}

// ================= ESP32-CAM =================
// 🔥 THÊM MỚI: Gửi lệnh chụp ảnh sang ESP32-CAM
void sendToESP32CAM(String name, String timeStr) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;   // 🔥 THÊM
  HTTPClient http;

  String url = "http://" + ESP32CAM_IP + "/capture?name=" + urlEncode(name) + "&time=" + urlEncode(timeStr);

  Serial.println("CALL CAM: " + url); // debug

   http.begin(client, url);   // 🔥 SỬA DÒNG NÀY (QUAN TRỌNG)
  //http.GET();   // gửi request
  int httpCode = http.GET();

  Serial.print("HTTP CODE: ");
  Serial.println(httpCode);
  http.end();
}

// ================= GET ID =================
int getID() {
  uint8_t p = finger.getImage();

  if (p == FINGERPRINT_NOFINGER) return -2;

  // 🔥 SỬA: bỏ lỗi giả (tránh spam sai)
  if (p == FINGERPRINT_PACKETRECIEVEERR || p == FINGERPRINT_IMAGEFAIL) {
    return -2;
  }

  if (p != FINGERPRINT_OK) return -1;

  if (finger.image2Tz() != FINGERPRINT_OK) return -1;
  if (finger.fingerSearch() != FINGERPRINT_OK) return -1;

  return finger.fingerID;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected");

  // NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // UART + I2C
  mySerial.begin(57600, SERIAL_8N1, FP_RX, FP_TX);
  Wire.begin(I2C_SDA, I2C_SCL);

  pinMode(LED_OK, OUTPUT);
  pinMode(LED_FAIL, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  lcd.init();
  lcd.backlight();

  finger.verifyPassword();

  // INIT TIME LIST
  for (int i = 0; i < MAX_ID; i++) {
    timeList[i] = 0;
  }

  // Welcome
  lcd.setCursor(3,0);
  lcd.print("Welcome to");
  lcd.setCursor(3,1);
  lcd.print("My project");

  unsigned long start = millis();
  while (millis() - start < 3000) {
    digitalWrite(LED_BLUE, HIGH);
    delay(200);
    digitalWrite(LED_BLUE, LOW);
    delay(200);
  }

  lcd.clear();
  lcd.print("Hay cham van tay");
  lcd.setCursor(0,1);
  lcd.print("cua ban hihi ^_^");

  digitalWrite(LED_BLUE, HIGH);
}

// ================= LOOP =================
void loop() {
  int id = getID();

  if (id == -2) return;

  lcd.clear();

  if (id == -1) {
    lcd.print("Van tay");
    lcd.setCursor(0,1);
    lcd.print("khong hop le");

    digitalWrite(LED_FAIL, HIGH);
    delay(2000);
    digitalWrite(LED_FAIL, LOW);
  }
  else {
    unsigned long now = millis();

    if (id < MAX_ID && timeList[id] != 0 && (now - timeList[id] < blockTime)) {

      unsigned long remain = (blockTime - (now - timeList[id])) / 1000;

      lcd.print("Van tay da quet");
      lcd.setCursor(0,1);
      lcd.print("Thu lai sau ");
      lcd.print(remain);
      lcd.print("s");

      digitalWrite(LED_FAIL, HIGH);
      delay(2000);
      digitalWrite(LED_FAIL, LOW);
    }
    else {
      String name = getName(id);

      if (name == "Khong xac dinh!") {
        lcd.print("Khong xac dinh");
        lcd.setCursor(0,1);
        lcd.print("ID:");
        lcd.print(id);

        digitalWrite(LED_FAIL, HIGH);
        delay(2000);
        digitalWrite(LED_FAIL, LOW);
      }
      else {
        // ===== HIỂN THỊ =====
        String timeNow = getTimeString();   // 🔥 THÊM: lưu thời gian 1 lần

        lcd.print(name);
        lcd.setCursor(0,1);
        lcd.print(timeNow);

        digitalWrite(LED_OK, HIGH);

        //delay(2000);

        // ===== TELEGRAM TEXT =====
        String msg = "Da diem danh(ESP32):\n";
        msg += name;
        msg += "\nTime: ";
        msg += timeNow;

        sendTelegram(msg);

        // ===== ESP32-CAM =====
        // 🔥 THÊM MỚI: chụp ảnh + gửi Telegram ảnh
        sendToESP32CAM(name, timeNow);

        digitalWrite(LED_OK, LOW);

        // lưu thời gian
        if (id < MAX_ID) {
          timeList[id] = now;
        }
      }
    }
  }

  // quay lại chờ
  lcd.clear();
  lcd.print("Hay cham van tay");
  lcd.setCursor(0,1);
  lcd.print("cua ban hihi ^_^");
}