#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = ""; //add wifi name
const char* password = ""; //add wifi password

// Prometheus Pushgateway
const char* pushUrl = "https://pushgateway.example.com/metrics/job/mq135_monitor/instance/node3-8266/sensor/mq135";


// ADC config
const int mq135Pin = A0;

unsigned long lastSampleTime = 0;
unsigned long lastPushTime = 0;

const unsigned long sampleInterval = 2000;     // 2 seconds
const unsigned long pushInterval = 30000;      // 30 seconds

float summq135 = 0;
float minmq135 = 1023;
float maxmq135 = 0;
int sampleCount = 0;
float lastSentAvgMq135Vaule = -1;

void setup() {
  Serial.begin(9600);
  delay(100);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void loop() {
unsigned long now = millis();

  // Sample every 2 seconds
  if (now - lastSampleTime >= sampleInterval) {
    lastSampleTime = now;
    float reading = analogRead(mq135Pin);
    summq135 += reading;
    sampleCount++;

    if (reading < minmq135) minmq135 = reading;
    if (reading > maxmq135) maxmq135 = reading;

    Serial.print("Sampled mq135 value: ");
    Serial.println(reading);
  }

  // Push to Prometheus every 30 seconds
  if (now - lastPushTime >= pushInterval && sampleCount > 0) {
    lastPushTime = now;

    float avgmq135 = summq135 / sampleCount;
    float diff = abs(avgmq135 - lastSentAvgMq135Vaule);
    if (lastSentAvgMq135Vaule < 0 || diff >= 5) {
    // Format as Prometheus text exposition format
    String payload = "";
    payload += "mq135_sensor_avg " + String(avgmq135, 2) + "\n";
    payload += "mq135_sensor_min " + String(minmq135, 2) + "\n";
    payload += "mq135_sensor_max " + String(maxmq135, 2) + "\n";

    sendToPushGateway(payload);

    // Reset stats
    lastSentAvgMq135Vaule = avgmq135;
    summq135 = 0;
    sampleCount = 0;
    minmq135 = 1023;
    maxmq135 = 0;
    } else {
          Serial.println("[INFO] No significant change in voltage, not sending.");
    }
  }
}


void sendToPushGateway(String payload) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nConnected!");
    return;
  }

  WiFiClientSecure client; // for https url
  client.setInsecure();
  // WiFiClient client; // for http url
  HTTPClient http;

  Serial.println("[INFO] Sending to Pushgateway:");
  Serial.println(payload);

  // Encode Basic Auth manually
  String base64Credentials = ""; //add creds here
  String authHeader = "Basic " + base64Credentials;
  http.begin(client, pushUrl);
  http.addHeader("Content-Type", "text/plain");
  http.addHeader("Authorization", authHeader);

  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    Serial.printf("Pushgateway Response: %d\n", httpCode);
  } else {
    Serial.printf("Failed to send data: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}
