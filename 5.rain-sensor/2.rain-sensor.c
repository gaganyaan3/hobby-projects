#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>

// WIFI CONFIG
const char* ssid = "";
const char* password = "";

// PUSHGATEWAY CONFIG
const char* pushUrl =
  "https://pushgateway.example.com/metrics/job/rain_monitor/instance/node3-8266/sensor/rain";

// Basic Auth credentials
const char* pgUser = "";
const char* pgPass = "";

const int rainPin = A0;

unsigned long lastSampleTime = 0;
unsigned long lastPushTime = 0;
unsigned long lastWifiCheck = 0;

const unsigned long sampleInterval = 2000;      // 2 sec
const unsigned long pushInterval = 30000;       // 30 sec

// Reconnect if signal weaker than this
const int minAcceptableRSSI = -82;

float sumRain = 0;
float minRain = 0;
float maxRain = 0;
int sampleCount = 0;
float lastSentAvgRain = -1;

void connectWiFi() {
  Serial.println();
  Serial.print("[WIFI] Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int retries = 0;

  while (WiFi.status() != WL_CONNECTED && retries < 10) {
    delay(1000);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("[WIFI] Connected!");
    Serial.print("[WIFI] IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.print("[WIFI] RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println();
    Serial.println("[WIFI] Failed to connect.");
  }
}


void checkWiFiHealth() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Disconnected. Reconnecting...");
    WiFi.disconnect();
    connectWiFi();
    return;
  }

  int rssi = WiFi.RSSI();

  Serial.print("[WIFI] RSSI: ");
  Serial.print(rssi);
  Serial.println(" dBm");
  delay(2000);

  if (rssi < minAcceptableRSSI) {
    Serial.println("[WIFI] Weak signal detected. Reconnecting...");

    WiFi.disconnect();
    delay(1000);

    connectWiFi();
  }
}

void setup() {
  Serial.begin(9600);
  delay(100);

  connectWiFi();
}


void loop() {
  unsigned long now = millis();

  // WIFI HEALTH CHECK
  checkWiFiHealth();

  // SAMPLE SENSOR
  if (now - lastSampleTime >= sampleInterval) {
    lastSampleTime = now;

    float reading = analogRead(rainPin);

    sumRain += reading;
    sampleCount++;

    if (reading < minRain) minRain = reading;
    if (reading > maxRain) maxRain = reading;

    Serial.print("[SENSOR] Rain value: ");
    Serial.println(reading, 2);
  }

  // PUSH METRICS
  if (now - lastPushTime >= pushInterval && sampleCount > 0) {
    lastPushTime = now;

    float avgRain = sumRain / sampleCount;
    float diff = abs(avgRain - lastSentAvgRain);

    String payload = "";
    payload += "rain_sensor_avg " + String(avgRain, 2) + "\n";
    payload += "rain_sensor_min " + String(minRain, 2) + "\n";
    payload += "rain_sensor_max " + String(maxRain, 2) + "\n";

    sendToPushGateway(payload);
    
    // Reset stats
    lastSentAvgRain = avgRain;
    sumRain = 0;
    sampleCount = 0;
    minRain = 0;
    maxRain = 0;
  }
}

// SEND TO PUSHGATEWAY
void sendToPushGateway(String payload) {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi not connected. Cannot send.");
    connectWiFi();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[ERROR] Reconnect failed.");
      return;
    }
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  Serial.println("[INFO] Sending metrics:");
  Serial.println(payload);

  // BASIC AUTH
  String auth = String(pgUser) + ":" + String(pgPass);
  String encodedAuth = base64::encode(auth);
  String authHeader = "Basic " + encodedAuth;

  // HTTP REQUEST
  http.begin(client, pushUrl);

  http.addHeader("Content-Type", "text/plain");
  http.addHeader("Authorization", authHeader);

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.print("[HTTP] Response code: ");
    Serial.println(httpCode);

    String response = http.getString();
    Serial.println(response);

  } else {
    Serial.print("[HTTP] POST failed: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}