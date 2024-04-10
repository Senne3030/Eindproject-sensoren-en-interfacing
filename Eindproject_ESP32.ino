//Eindproject Senne Van Dingenen 1IoT 15/04/2024

#define BLYNK_TEMPLATE_ID "user8"
#define BLYNK_TEMPLATE_NAME "user8@server.wyns.it"
#define BLYNK_PRINT Serial

//Include libraries
#include <LiquidCrystal.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

//Assign wifi SSID, password, IP of the
char auth[] = "JR4gsYX5TmtJ84ZL4hxn50Jh4NGS9sbJ";
char ssid[] = "telenet-A5C9B";            //(embed)
char pass[] = "nuN695U3fJQ3";             //(weareincontrol)
const char* server_ip = "192.168.0.180";  //MINI D1 IP

//Assign GPIO's
const int RedButton = 26;
const int BlueButton = 25;
const int CoolerLED = 27;
const int Heater = 14;
const int LightSensor = 32;
const int Cooler = 19;
const int LightLED = 18;

//make those values an integer
int BlueButtonValue;
int RedButtonValue;
int TotalValue;
int virtualButtonValue;
int ActualTemperature;
//Assign values to use for millis
unsigned long PreviousMillis1 = 0;
const long WaitTime1 = 5000;  //1000 = 1 second

void setup() {
  Serial.begin(9600);  //Begin Serial communication

  delay(10);  //Delay for 10 miliSeconds
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //Begin Wifi communication
  WiFi.begin(ssid, pass);
  int wifi_ctr = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  Blynk.begin(auth, ssid, pass, "server.wyns.it", 8081);  // Begin Blynk communication to a private server with gate 8081

  //Set PinModes
  pinMode(RedButton, INPUT_PULLUP);
  pinMode(BlueButton, INPUT_PULLUP);
  pinMode(LightSensor, INPUT);
  pinMode(CoolerLED, OUTPUT);
  pinMode(Heater, OUTPUT);
  pinMode(Cooler, OUTPUT);
  pinMode(LightLED, OUTPUT);
}

//Read Virtual 6 from the app
BLYNK_WRITE(V6) {
  virtualButtonValue = param.asInt();
}

void loop() {
  //Open Voids
  CheckLightSensor();
  WriteTemperature();
  CheckvirtualButtonForTemperature();
  heatingORcooling();

  // Use Millis to check every few seconds
  unsigned long currentMillis1 = millis();
  if (currentMillis1 - PreviousMillis1 >= WaitTime1) {
    CheckActualTemperatureFromMiniD1();
    PreviousMillis1 = currentMillis1;
  }

  Blynk.run();  //Ensure the connection with the Blynk app
}

void WriteTemperature() {  //Void used to physically change the wanted temperature value

  // Make those values 0 so there can be no mistakes
  int RedButtonState = 0;
  int BlueButtonState = 0;

  //If a button it pressed make its value 1, for further action
  if (digitalRead(RedButton) == 0) {
    RedButtonState = 1;
  }
  if (digitalRead(BlueButton) == 0) {
    BlueButtonState = 1;
  }

  //If the buttonValue is 1 do
  if (RedButtonState == 1) {
    Serial.println("RED, +1");
    RedButtonState = 0;

    if (TotalValue >= 30) {  // Max number is 30°C
      TotalValue = TotalValue;
    } else {
      TotalValue = TotalValue + 1;  // if it's not 30°C then do +1°C
    }
    //Print in serial and upload the value to the blynk app
    Serial.print("Wanted Temperature: ");
    Serial.println(TotalValue);
    Blynk.virtualWrite(V7, TotalValue);

    delay(200);  //Delay for 200 MiliSeconds
  }

  if (BlueButtonState == 1) {
    Serial.println("BLUE, -1");
    BlueButtonState = 0;

    if (TotalValue <= 0) {  //Lowest number is 0°C
      TotalValue = TotalValue;
    } else {
      TotalValue = TotalValue - 1;  //if it's not 0°C then do -1
    }
    //Print in serial and upload the value to the blynk app
    Serial.print("Wanted Temperature: ");
    Serial.println(TotalValue);
    Blynk.virtualWrite(V7, TotalValue);

    delay(200);  //Delay for 200 MiliSeconds
  }
}

void heatingORcooling() {  //Void used to determene if we have to cool or heat the room

  if (ActualTemperature == 0) {  //If the temperature sent was 0, return.
    return;
  } else {
    if (TotalValue == ActualTemperature) {  // If the value is the same nothing should be on
      digitalWrite(CoolerLED, LOW);
      digitalWrite(Heater, LOW);
      Blynk.virtualWrite(V0, 0);  //Heating
      Blynk.virtualWrite(V1, 0);  //Cooling
      digitalWrite(Cooler, LOW);
    } else if (TotalValue < ActualTemperature) {  // If the value is lower, cool.
      digitalWrite(CoolerLED, HIGH);
      digitalWrite(Heater, LOW);
      Blynk.virtualWrite(V0, 0);    //Heating
      Blynk.virtualWrite(V1, 255);  //Cooling //255 is max. brightness for an led in Blynk
      digitalWrite(Cooler, HIGH);
    } else if (TotalValue > ActualTemperature) {  //If the value is higher, heat.
      digitalWrite(CoolerLED, LOW);
      digitalWrite(Heater, HIGH);
      Blynk.virtualWrite(V0, 255);  //Heating
      Blynk.virtualWrite(V1, 0);    //Cooling
      digitalWrite(Cooler, LOW);
    }
  }
}

void CheckvirtualButtonForTemperature() {  //Void used to virtually change the wanted temperature value

  Serial.print("virtualButtonValue: ");
  Serial.println(virtualButtonValue);

  if (virtualButtonValue == -1) {  // If the - is pressed do -1 only if it's bigger then 0
    if (TotalValue <= 0) {
      TotalValue = TotalValue;
    } else {
      TotalValue = TotalValue - 1;
    }
    virtualButtonValue = 0;
  }
  if (virtualButtonValue == 1) {  // If the + is pressed do +1 only if it's smaller then 30
    if (TotalValue >= 30) {
      TotalValue = TotalValue;
    } else {
      TotalValue = TotalValue + 1;
    }
    virtualButtonValue = 0;
  }
  Serial.println(TotalValue);  //Print in serial and upload the value to the blynk app
  Blynk.virtualWrite(V7, TotalValue);
}

void CheckActualTemperatureFromMiniD1() {  //Void used to recieve the Temperature of the DHT11sensor

  WiFiClient client;  // Create a WiFi client object

  if (client.connect(server_ip, 80)) {
    Serial.println("Connected to server");

    // Get the actualtemperature from the the Mini D1
    client.println("GET ActualTemperature");
    client.println("Host: " + String(server_ip));
    client.println("Connection: close");
    client.println();

    while (client.connected()) {
      if (client.available()) {
        String response = client.readStringUntil('\r');  // Reads the response from the server until carriage return ('\r')
        Serial.println("Response: " + response);         // Prints the response

        if (response.startsWith("ActualTemperature: ")) {
          String temperatureStr = response.substring(response.indexOf(':') + 2);  // Extracts the temperature value from the response

          if (temperatureStr) {  // Converts the temperature value to an integer
            ActualTemperature = temperatureStr.toInt();
            Serial.print("Echte temperatuur: ");
            Serial.println(ActualTemperature);
          }
        }

        // Read and print any remaining data from the server
        while (client.available()) {
          char c = client.read();
          Serial.print(c);
        }
        Serial.println();

        break;  // Exit the loop
      }
    }

    delay(1);
    client.stop();  // Stops the client connection
    Serial.println("Disconnected from server");
  } else {
    Serial.println("Connection failed");
  }
}

void CheckLightSensor() {  //Void used to determene if the light is on or off in the room

  int lightValue = analogRead(LightSensor);
  if (lightValue > 2000) {  // If the lightvalue is greater then 2000 then
    Blynk.virtualWrite(V3, 255);
    digitalWrite(LightLED, HIGH);
    Serial.println("ON");
  } else {
    Blynk.virtualWrite(V3, 0);
    digitalWrite(LightLED, LOW);
    Serial.println("OFF");
  }

  Serial.print("LightSensor: ");
  Serial.println(lightValue);
}
