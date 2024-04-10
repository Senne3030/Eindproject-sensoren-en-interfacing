#define BLYNK_TEMPLATE_ID "user8"                   // Your Template ID for Blynk
#define BLYNK_TEMPLATE_NAME "user8@server.wyns.it"  // Your Template name for Blynk
#define BLYNK_PRINT Serial                          // Define Blynk print output to Serial

//Include libraries
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT11.h>

char auth[] = "p9blFSvobraj-7ZzvgN1Puz7jGm8_xpA";  // Your Blynk authentication token
char ssid[] = "telenet-A5C9B";                     //embed // Your Wifi SSID
char pass[] = "nuN695U3fJQ3";                      //weareincontrol // Your Wifi password

DHT11 dht11(2);  // The DHT11 sensor is connected to pin D4, or GPIO 2.


unsigned long previousMillis = 0;  // For millis make it a unsigned long
const long WaitTime = 5000;        // Wait time in milliseconds

int ActualTemperature = 0;  // Make the value an integer

WiFiServer server(80);  // Wifi server on port 80

void setup() {
  Serial.begin(9600);  // Begin Serial communication

  // Begin connection to your wifi network
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);
  int wifi_ctr = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  // Initialize Blynk connection to a private server with gate 8081
  Blynk.begin(auth, ssid, pass, "server.wyns.it", 8081);
}
void loop() {

  CheckMiniD1Info();  // Check for incoming client connections requests

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= WaitTime) {
    CheckTemperatureAndHumidity();  //when the DHT11 sensor reads its time it takes a full second, so that's why.
    previousMillis = currentMillis;
  }

  Blynk.run();
}

void CheckTemperatureAndHumidity() {  // Read temperature and humidity

  int temperature = dht11.readTemperature();
  int Humidity = dht11.readHumidity();

  // Check if readings are valid
  if (temperature != DHT11::ERROR_CHECKSUM && temperature != DHT11::ERROR_TIMEOUT) {
    if (Humidity != DHT11::ERROR_CHECKSUM && Humidity != DHT11::ERROR_TIMEOUT) {

      // Print temperature and humidity to Serial
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" °C");
      Serial.print("Humidity: ");
      Serial.print(Humidity);
      Serial.println(" %");

      // Send temperature and humidity data to Blynk
      Blynk.virtualWrite(V4, temperature);
      Blynk.virtualWrite(V5, Humidity);
      ActualTemperature = temperature;  // Globally save the temperature so it can be sent to the ESP32
    }
  }
}

void CheckMiniD1Info() {

  WiFiClient client = server.available();  // Check for incoming client connections

  if (client) {
    Serial.println("New client connected");

    while (client.connected()) {
      if (client.available()) {
        // Read client request
        String request = client.readStringUntil('\r');
        Serial.println("Request: " + request);

        // Send actual temperature to client
        client.print("ActualTemperature: ");
        client.println(ActualTemperature);
        break;  //Break the loop
      }
    }

    // Stop connection to the client
    delay(1);
    client.stop();
    Serial.println("Client disconnected");
  }
}