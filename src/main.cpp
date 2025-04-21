#include <Arduino.h>
#include <time.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include "ThingSpeak.h"

// Time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 3; // Adjust for your timezone (Istanbul is GMT+3)
const int   daylightOffset_sec = 0;

// Wi-Fi Settings
const char* ssid = "YavasIntBurda";
const char* password = "RikiDikiveCucpo";
WiFiServer server(80);

// ThingSpeak Server Settings
WiFiClient  client;

unsigned long myChannelNumber = 2887470;
const char * myWriteAPIKey = "C72BBAEET0UFASC6";

// Pin Definitions
const int relayPin = 26;
const int dhtPin = 23;
const int soilMoisturePin = 35;
const int ledPin = 2; 

// DHT Sensor Setup
#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Timing Variables
const long thingSpeakInterval = 28000; // 60 seconds in milliseconds
unsigned long previousThingSpeakMillis = 0;

// Led Variables
unsigned long previousLcdSwitchMillis = 0;
const long lcdSwitchInterval = 4000;  // 4 seconds
int screenState = 0;

// Sensor data cache (to reuse values between LCD updates)
float cachedHumidity = 0;
float cachedTemperature = 0;
int cachedSoilMoisture = 0;

void setup() {
  // Init monitor
  Serial.begin(115200);

  // Init time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Relay Setup
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // LOW means it is working by the way

  // DHT Setup
  dht.begin();

  // LCD Setup
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  lcd.display();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);

  // Start Server
  server.begin();
  pinMode(ledPin, OUTPUT); // init LED pin
  digitalWrite(ledPin, LOW); // LED off
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    if (request.indexOf("/command?") != -1) {
      int startIndex = request.indexOf("command=") + 8;
      int endIndex = request.indexOf(" HTTP/");
      String command = request.substring(startIndex, endIndex);

      if (command == "relay_on") {
        digitalWrite(relayPin, HIGH);
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Relay ON");
      } else if (command == "relay_off") {
        digitalWrite(relayPin, LOW);
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Relay OFF");
      } else if (command == "led_on") {
        digitalWrite(ledPin, HIGH);
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("LED ON");
      } else if (command == "led_off") {
        digitalWrite(ledPin, LOW);
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("LED OFF");
      } else {
        client.println("HTTP/1.1 400 Bad Request");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Unknown command");
      }
    }
    client.stop();
  }

  // Non-Blocking ThingSpeak Upload
  unsigned long currentMillis = millis();
  if (currentMillis - previousThingSpeakMillis >= thingSpeakInterval) {
    previousThingSpeakMillis = currentMillis;

    // Read Sensors and Send Data to ThingSpeak
    float humidity = dht.readHumidity();
    delay(1000);
    float temperature = dht.readTemperature();
    delay(1000);
    int soilMoistureValue = analogRead(soilMoisturePin);
    

    if (isnan(humidity)) {
      humidity = 0;
    }
    if (isnan(temperature)) {
      temperature = 0;
    }
    if (isnan(soilMoistureValue)) {
      soilMoistureValue = 0;
    }

    // Cache values for LCD use
    cachedHumidity = isnan(humidity) ? 0 : humidity;
    cachedTemperature = isnan(temperature) ? 0 : temperature;
    cachedSoilMoisture = soilMoistureValue;

    // Thingspeak send off
    String ipStr = WiFi.localIP().toString();

    ThingSpeak.setField(1, humidity);
    ThingSpeak.setField(2, temperature);
    ThingSpeak.setField(3, soilMoistureValue);
    ThingSpeak.setField(4, ipStr);
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.println("Channel update successful.");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
      
    // Serial Output
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Temperature: ");
    Serial.print(temperature);
    Serial.print(" *C, Soil Moisture: ");
    Serial.println(soilMoistureValue);

  }

  // Display Sensor Data on LCD
  if (millis() - previousLcdSwitchMillis >= lcdSwitchInterval) {
    previousLcdSwitchMillis = millis();
    screenState = (screenState + 1) % 2;  // Toggle 0 ↔ 1
  
    lcd.clear();
    if (screenState == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(cachedTemperature);
      lcd.print(" C");
  
      lcd.setCursor(0, 1);
      lcd.print("Hum: ");
      lcd.print(cachedHumidity);
      lcd.print(" %");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Soil Moisture:");
      lcd.setCursor(0, 1);
      lcd.print(cachedSoilMoisture);
    }
    lcd.display();
  }
}