#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// SH1106 128x64 I2C OLED
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  Serial.begin(9600);
  pinMode(23, OUTPUT); // for display 
  digitalWrite(23, HIGH);
  delay(100);
  dht.begin();

  u8g2.begin();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(15, 30, "Starting...");
  u8g2.sendBuffer();

  delay(1000);
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Celsius

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT11!");

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 30, "DHT11 Error!");
    u8g2.sendBuffer();

    delay(2000);
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C  Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Display on OLED
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(20, 15, "DHT11 Monitor");

  u8g2.setFont(u8g2_font_logisoso16_tr);

  char tempStr[20];
  sprintf(tempStr, "T: %.1f C", temperature);
  u8g2.drawStr(5, 40, tempStr);

  char humStr[20];
  sprintf(humStr, "H: %.1f %%", humidity);
  u8g2.drawStr(5, 62, humStr);

  u8g2.sendBuffer();

  delay(2000);
}