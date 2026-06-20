#include <Adafruit_Fingerprint.h>
HardwareSerial mySerial(2);

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, 16, 17);

  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("OK ket noi cam bien");
  } else {
    Serial.println("LOI ket noi");
    while (1);
  }
}

void loop() {}