#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// WiFi credentials
const char* ssid = "SSID";
const char* password = "PASS";

// Prometheus Pushgateway
const char* pushgatewayHost = "https://pushgateway.example.com/metrics/job/voltage_monitor/instance/node-8266/sensor/voltage";



// Voltage divider resistors
const float R1 = 30000.0;
const float R2 = 7500.0;

// ADC config
const float ADC_RESOLUTION = 1023.0;
const float REF_VOLTAGE = 3.3;  // Adjust depending on your board
float previousVoltage = -1.0; 

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void loop() {
  float voltage = readVoltage();
  if (abs(voltage - previousVoltage) > 0.01) {  // send only if change > 10 mV
    sendToPrometheus(voltage);
    previousVoltage = voltage;
  } else {
    Serial.println("⚠️ Voltage unchanged. Skipping Pushgateway update.");
  }
  delay(20000);  // Send every 20 seconds
}

float readVoltage() {
  int adcValue = analogRead(A0);
  float vOut = (adcValue / ADC_RESOLUTION) * REF_VOLTAGE;
  float vIn = vOut * ((R1 + R2) / R2);
  Serial.print("Measured Voltage: ");
  Serial.println(vIn, 3);
  return vIn;
}

void sendToPrometheus(float voltage) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
  }
  //WiFiClient client; // code for http
  //HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String metrics = "voltage_sensor " + String(voltage, 3) + "\n";
  String url = String(pushgatewayHost) ;

  // Encode Basic Auth manually
  String base64Credentials = "$Base64encoded_Creds";
  String authHeader = "Basic " + base64Credentials;
  http.begin(client, url);
  http.addHeader("Content-Type", "text/plain");
  http.addHeader("Authorization", authHeader);

  int httpCode = http.POST(metrics);
  if (httpCode > 0) {
    Serial.printf("Pushgateway Response: %d\n", httpCode);
  } else {
    Serial.printf("Failed to send data: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}