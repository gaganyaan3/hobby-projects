#include <Wire.h>
#include <U8g2lib.h>
// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <WiFiClientSecure.h>
// For OLED: SH1106 128x64 I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// sensor pin
const int mq135Pin = 34;
String air_quality = "NA";

void setup() {
  Serial.begin(9600);
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH); //for display power
  
  delay(100);
  u8g2.begin();
  Wire.begin();

}

void loop() {

    float mq135_reading = analogRead(mq135Pin)/4;
    Serial.println("mq135 reading: " + String(mq135_reading));
    delay(1000);

    if (mq135_reading < 100) {
      air_quality = "Very Good ;) ";
    } 
    else if (mq135_reading > 100 && mq135_reading <= 200) {
      air_quality = "Good :)";
    } 
    else if (mq135_reading > 200 && mq135_reading <= 400) {
      air_quality = "Modrate";
    } 
    else if (mq135_reading > 400 && mq135_reading <= 500) {
      air_quality = "Unhealthy :(";
    } 
    else if (mq135_reading > 500 && mq135_reading <= 600) {
      air_quality = "Unhealthy :(";
    } 
    else if (mq135_reading > 600) {
      air_quality = "Hazardous :(";
    }
    Serial.println("air_quality reading: " + String(air_quality));

    //display block
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_7x14_tr);  // large font for AQ
    char airStr[32];
    sprintf(airStr, "Air Quality: %.1f", mq135_reading);
    u8g2.drawStr(0, 20, airStr);

    u8g2.setFont(u8g2_font_6x10_tf);  // small font for status
    char statusStr[32];
    sprintf(statusStr, "Status: %s", air_quality);
    u8g2.drawStr(0, 44, statusStr);

    u8g2.sendBuffer();



}


