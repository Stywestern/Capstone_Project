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
const char* ssid = "VODAFONE_40EB";
const char* password = "K07022003";
WiFiServer server(80);

// ThingSpeak Server Settings
WiFiClient  client;

unsigned long myChannelNumber = 2887470;
const char * myWriteAPIKey = "C72BBAEET0UFASC6";

// Pin Definitions
const int relayPin = 26;
const int dhtPin = 4;
const int soilMoisturePin = 32;
const int ledPin = 2; 

// DHT Sensor Setup
#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Timing Variables
const long thingSpeakInterval = 60000; // 60 seconds in milliseconds
unsigned long previousThingSpeakMillis = 0;

void setup() {
  // Init monitor
  Serial.begin(115200);

  // Init time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Relay Setup
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

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
    float temperature = dht.readTemperature();
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

    ThingSpeak.setField(1, humidity);
    ThingSpeak.setField(2, temperature);
    ThingSpeak.setField(3, soilMoistureValue);
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.println("Channel update successful.");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }

    // Display Sensor Data on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(" C-");
    lcd.setCursor(0, 1);
    lcd.print("Soil: ");
    lcd.print(soilMoistureValue);
    lcd.display();

    // Serial Output
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Temperature: ");
    Serial.print(temperature);
    Serial.print(" *C, Soil Moisture: ");
    Serial.println(soilMoistureValue);
  }
}