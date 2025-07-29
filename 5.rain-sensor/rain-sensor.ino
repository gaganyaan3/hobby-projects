#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>


const char* ssid = ""; //add wifi name
const char* password = ""; //add wifi password

// Prometheus Pushgateway
const char* pushUrl = "https://pushgateway.example.com/metrics/job/rain_monitor/instance/node-8266/sensor/rain";


// ADC config
const int rainPin = A0;

unsigned long lastSampleTime = 0;
unsigned long lastPushTime = 0;

const unsigned long sampleInterval = 2000;     // 2 seconds
const unsigned long pushInterval = 30000;      // 30 seconds

float sumRain = 0;
float minRain = 3.30;
float maxRain = 0;
int sampleCount = 0;
float lastSentAvgVoltage = -1;

void setup() {
  Serial.begin(115200);
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
    float reading = analogRead(rainPin) * (3.3 / 1023.0);
    sumRain += reading;
    sampleCount++;

    if (reading < minRain) minRain = reading;
    if (reading > maxRain) maxRain = reading;

    Serial.print("Sampled rain value: ");
    Serial.println(reading);
  }

  // Push to Prometheus every 30 seconds
  if (now - lastPushTime >= pushInterval && sampleCount > 0) {
    lastPushTime = now;

    float avgRain = sumRain / sampleCount;
    float diff = abs(avgRain - lastSentAvgVoltage);
    if (lastSentAvgVoltage < 0 || diff >= 0.02) {
    // Format as Prometheus text exposition format
    String payload = "";
    payload += "rain_sensor_avg " + String(avgRain, 2) + "\n";
    payload += "rain_sensor_min " + String(minRain, 2) + "\n";
    payload += "rain_sensor_max " + String(maxRain, 2) + "\n";

    sendToPushGateway(payload);

    // Reset stats
    lastSentAvgVoltage = avgRain;
    sumRain = 0;
    sampleCount = 0;
    minRain = 3.30;
    maxRain = 0;
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
