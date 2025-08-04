#include <Wire.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
//For OLED: SH1106 128x64 I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

//wifi credentials
const char* ssid = "";
const char* password = "";

//pushgateway
const char* pushUrl = "https://pushgateway.example.com/metrics/job/mq135_monitor/instance/node2-8266/sensor/mq135";
String base64Credentials = ""; //base64 encoded "username:password"

const int mq135Pin = A0;

//time
unsigned long lastSampleTime = 0;
unsigned long lastPushTime = 0;
const unsigned long sampleInterval = 2000;   // 2s
const unsigned long pushInterval = 30000;    // 30s

float summq135 = 0;
float minmq135 = 0;
float maxmq135 = 0;
int sampleCount = 0;
int mustSentCount = 300;
int lastSentAvgMq135Vaule = -1;
int latestAvg = 0;

String wifiStatus = "NA";
String pushStatus = "Unknown";

void setup() {
  Serial.begin(115200);
  delay(100);

  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  delay(5000);

  u8g2.begin();
  Wire.begin();

}

void loop() {
  int wifi_rssi = WiFi.RSSI();
  // Serial.println("Signal strength (RSSI): " + String(wifi_rssi) + " dBm");
  // delay(100);
  unsigned long now = millis();
  // Reconnect if wifi power is low (Took 2 days to fix this)
  if ( WiFi.RSSI() < -82) {
    WiFi.disconnect();
    Serial.println("Disconnected wifi at (RSSI): " + String(wifi_rssi) + " dBm");
    delay(1000);
    WiFi.begin(ssid, password);
    Serial.println("connecting back...");
    delay(10000);
  }
  if (WiFi.status() != WL_CONNECTED) {
    wifiStatus = "NA";
    pushStatus = "Unknown";
  } else {
    wifiStatus = "OK" + String(wifi_rssi);
  }

 
  if (now - lastSampleTime >= sampleInterval) {
    lastSampleTime = now;

    float reading = analogRead(mq135Pin);
    summq135 += reading;
    sampleCount++;
    Serial.print("reading : ");
    Serial.println(reading);
    if (reading < minmq135) minmq135 = reading;
    if (reading > maxmq135) maxmq135 = reading;

    latestAvg = summq135 / sampleCount;

    //Display
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_7x14_tr);
    char airStr[32];
    sprintf(airStr, "Air Quality: %.1f", reading);
    u8g2.drawStr(0, 20, airStr);

    u8g2.setFont(u8g2_font_6x10_tf);
    char wifiStr[32];
    sprintf(wifiStr, "WiFi: %s", wifiStatus.c_str());
    u8g2.drawStr(0, 44, wifiStr);

    char pushStr[32];
    sprintf(pushStr, "Push: %s", pushStatus.c_str());
    u8g2.drawStr(0, 60, pushStr);

    u8g2.sendBuffer();
  }

  if ((now - lastPushTime >= pushInterval) && sampleCount > 0) {
    lastPushTime = now;

    float avgmq135 = summq135 / sampleCount;
    float diff = abs(avgmq135 - lastSentAvgMq135Vaule);
    if (lastSentAvgMq135Vaule < 0 || diff >= 0.5) {
      String payload = "";
      payload += "mq135_sensor_avg " + String(avgmq135, 2) + "\n";
      payload += "mq135_sensor_min " + String(minmq135, 2) + "\n";
      payload += "mq135_sensor_max " + String(maxmq135, 2) + "\n";

      //push Prometheus
      if (WiFi.status() == WL_CONNECTED) {
      bool success = sendToPushGateway(payload);
      pushStatus = success ? "OK-"+String(avgmq135) : "Fail-"+String(avgmq135);
      }

      lastSentAvgMq135Vaule = avgmq135;
      summq135 = 0;
      sampleCount = 0;
      minmq135 = 0;
      maxmq135 = 0;
    } else {
      Serial.println("No significant change, not sending.");
      pushStatus = "NoChange-" + String(avgmq135);
    }
  }
}

bool sendToPushGateway(String payload) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(5000);

  Serial.println("Sending to Pushgateway:");
  Serial.println(payload);

  http.begin(client, pushUrl);
  http.addHeader("Content-Type", "text/plain");
  http.addHeader("Authorization", "Basic " + base64Credentials);
  http.setTimeout(5000);

  int httpCode = http.POST(payload);
  if (httpCode == 200) {
    Serial.printf("success: %d %s\n", httpCode, http.errorToString(httpCode).c_str());
    http.end();
    return true;
  } else {
    Serial.printf("failed: %d %s\n", httpCode, http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
}
