#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <BlynkSimpleEsp8266.h>
#include <Timer.h>
#include <DHT.h>
#include <ThingSpeak.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>

// WiFi settings
#define ssid "P1602"
#define pass "0913273839"

// Thingspeak settings
#define WriteAPIKey "03ITLO7XCGSD2M08"  
#define channelID 1631256                 
#define apiServer "api.thingspeak.com"    

// Blynk setting
#define auth "c-TRSgfacuiPvwm49JfSUnjmBY6SqdJ3"

// DHTxx sensor type and digital pin
#define DHTTYPE DHT11
#define DHTPIN 10

// Rain sensor setting
#define rainAnalog A0
#define rainDigital D4
LiquidCrystal_I2C lcd(0x27,16,2); 

byte thermometro[8] = //icon for termometer
{
  B00100,
  B01010,
  B01010,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110
};

byte igrasia[8] = //icon for water droplet
{
  B00100,
  B00100,
  B01010,
  B01010,
  B10001,
  B10001,
  B10001,
  B01110,
};

DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;
// initialize timers
Timer timer;
BlynkTimer blynkTimer;
// initialize global variables to store sensors readings
float humidity, temperature;
int rainAnalogVal, rainDigitalVal;
// indicates state of Blynk connection 
bool connect = false;
// indicates whether to display Rain sensor reading or DHT11 reading on LCD
bool lcdDHT = true;

void connectWiFi(){
  Serial.println("Connecting");
  WiFi.mode(WIFI_STA);
  // if NodeMCU is not connected to WiFi
  if (WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect");
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nWiFi Connected.");
    Serial.println(WiFi.localIP());
  }
}

void readSensor(){
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  rainAnalogVal = analogRead(rainAnalog);
  rainDigitalVal = digitalRead(rainDigital);
}

void setupBlynk(){
  Serial.println("Setup Blynk");
  // if NodeMCU is connected to WiFi and not connected to Blynk server
  if ((WiFi.status() == WL_CONNECTED) && (!connect)){
    Blynk.begin(auth, ssid, pass, "blynk-server.com", 8080);
    Serial.println("Connected to Blynk server");
    connect = true;
  }
}


void writeToLCD(){
  // LCD's SDA is connected to GPIO5 and SCL to GPIO4
  lcd.init(); 
  lcd.backlight();
  if (lcdDHT){
    lcd.setCursor(0, 0);
    lcd.print("BTL VXL Nhom 2");
    lcd.createChar(1, thermometro);
    lcd.createChar(2, igrasia);
    lcd.setCursor(0, 1);
    lcd.write(1);
    lcd.print(temperature);
    lcd.print((char)223);
    lcd.print("C");
    lcd.print(" ");
    lcd.write(2);
    lcd.print(humidity);
    lcd.print("%");
    lcdDHT = false;
  } else {
    lcd.setCursor(0, 0);
    lcd.print("RainAnalog: ");
    lcd.print(rainAnalogVal);
    lcd.setCursor(0, 1);
    if (rainDigitalVal)
    {
      lcd.print("It isnt raining");
    } 
    else lcd.print("It is raining");
    lcdDHT = true;
  }
}

void writeToBlynk(){
  Serial.println("BLYNK WRITE");
  if (Blynk.connected()){
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, humidity);
    Blynk.virtualWrite(V2, rainAnalogVal);
    if (rainDigitalVal == 0) Blynk.notify("It is raining");
  }
}
void sendToThingspeak(){
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, rainAnalogVal);
  int writeSuccess = ThingSpeak.writeFields(channelID, WriteAPIKey);
  if (writeSuccess) Serial.println("THINGSPEAK");  
}
void setup(){ 
  Serial.begin(9600);
  ThingSpeak.begin(client);
  pinMode(rainDigital,INPUT);
  dht.begin();
  // every 5 seconds, check WiFi connection and try to connect if not connected 
  timer.every(5000, connectWiFi);
  // every 7 seconds, check Blynk connection and try to connect if not connected
  blynkTimer.setInterval(7000, setupBlynk);
  // every 5 seconds, display sensors readings on LCD
  timer.every(5000, writeToLCD);
  // every 5 seconds, send sensors readings to Blynk server 
  blynkTimer.setInterval(5000, writeToBlynk);
  // every 20 seconds, send sensors readings to Thingspeak server 
  // Thingspeak needs 15-second invervals between requests
  timer.every(20000, sendToThingspeak);
}

void loop() {
  timer.update();
  blynkTimer.run();
  readSensor();
  client.stop();
}