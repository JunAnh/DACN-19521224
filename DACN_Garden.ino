//---------Libraries--------
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <Wire.h>
#include <NTPtimeESP.h>

//---------PinOut---------
#define DHTPIN 12 // D6
#define DHTTYPE DHT11
#define MoisturePIN A0 
#define pumpPin 2 //D4
#define mistPin 16 // D0
#define fanPin 5 // D1
#define DEBUG_ON

//--------Wifi Information--------
const char* ssid = "Phuong Anh";//"Phuong Anh";
const char* password = "phuonganh2001";// "phuonganh2001";
 
//--------MQTT Information--------
const char* mqtt_server = "192.168.2.10";   
const uint16_t mqtt_port = 1883;

//--------Var--------
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
NTPtime NTPch("ch.pool.ntp.org");
strDateTime dateTime;

long lastMsg,lastRead,lastProcess,lastWater,lastMist,lastFan = 0;
char msgMoisture[50];
char msgHumidity[50];
char msgTemperature[50];
int value = 0;
int mt = 0;
float t;
float h;
int timer;

//--------Threshold----------
//----Default threshold--------
int moisThreshold = 50;
float humidThreshold = 50;
float minTempThreshold = 20;
float maxTempThreshold = 25;
int timeThreshold;

//-------------Setup----------------
void setup(){
  dht.begin();                 // Initialize DHT11 Sensor
  pinMode(MoisturePIN, INPUT); // Initialize Moisture Sensor
  pinMode(pumpPin, OUTPUT);    // Initialize Pump Motor
  pinMode(mistPin, OUTPUT);    // Initialize Mist Motor
  pinMode(fanPin, OUTPUT);     // Initialize Fan
  Serial.begin(115200); 
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

//------------Loop------------
void loop(){
  autoProcess();              // Auto get & process data from sensors
  
  // Reconnect if losing wifi
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  timer = getTime();          // Get time from NTP Server
  if (timer == timeThreshold ) // Processing timer
   {
      Serial.println("Time up!");
      itrWater();
   }
   
 // Send data to MQTT broker per 2 seconds
  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    publishData();
    client.subscribe("action");
    client.subscribe("event");
    client.subscribe("time");
  }
}

//------------Publishing Data----------
void publishData(){
    snprintf (msgMoisture, 75, "Moisture:%ld", mt);
    Serial.print("Publish message: ");
    Serial.println(msgMoisture);
    client.publish("event", msgMoisture);
    
    snprintf (msgHumidity, 75, "Humidity:%ld", (int) h);
    Serial.print("Publish message: ");
    Serial.println(msgHumidity);
    client.publish("event", msgHumidity);

    snprintf (msgTemperature, 75, "Temperature:%ld", (int) t);
    Serial.print("Publish message: ");
    Serial.println(msgTemperature);
    client.publish("event", msgTemperature);
}

//------------Processing---------------
void autoProcess(){
  readSensor();
  long procestTime = millis();
  if ( procestTime - lastProcess > 2000)
  {
    if (mt < moisThreshold) {
      Serial.println("Watering"); 
      itrWater();   // Start watering
    }
    else {
      Serial.println("Stop watering");
      digitalWrite(pumpPin, LOW);   
    }
    if (h < humidThreshold) {
      Serial.println("Misting"); 
      itrMist();   // Start misting
    }
    else {
      Serial.println("Stop misting");
      digitalWrite(mistPin, LOW);  
    }
    if (t >= (maxTempThreshold + 1)) {
      Serial.println("Turn on the fan"); 
      itrFan();   // Turn on the fan
    }
    else {
      Serial.println("Turn off the fan");
      digitalWrite(fanPin, LOW);
    }
    lastProcess = procestTime;
  }
}

// -----------Read Sensors-------------
int readMoisture(){
  int moisture = analogRead(MoisturePIN);
  float m = (moisture * 170) / 1023;  // Convert Analog Signal to Percent - Adjusted
  if ( m >= 100 )
    m = 100;
  return m;
}

float readTemperatureDHT(){
  float temp = dht.readTemperature();
    if (isnan(temp)) {
    Serial.println(F("Failed to read Temperature from DHT sensor!"));
    }
  return temp;
}

float readHumidityDHT(){
  float humid = dht.readHumidity();
    if (isnan(humid)) {
    Serial.println(F("Failed to read Humidity from DHT sensor!"));
    }
  return humid;
}

void readSensor(){
  long sNow = millis();
  if (sNow - lastRead > 1000) // read sensors per second
  {
    lastRead = sNow;
    mt = readMoisture();
    t = readTemperatureDHT();
    h = readHumidityDHT();
  }
}

//----------------CALLBACK-------------------
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ((char)payload[0] == '1') {
    Serial.println("ON");
    itrWater();
  } 
  if ((char)payload[0] == '4') {
    Serial.println("OFF");
    digitalWrite(pumpPin, LOW);
  }
  
  // Set Threshold while "Tomato" is chosen
  if ((char)payload[0] == '2') {
    Serial.println("Tomato");
    moisThreshold = 70;
    humidThreshold = 60;
    minTempThreshold = 26;
    maxTempThreshold = 29;
  }

  // Set Threshold while "Cucumber" is chosen
  if ((char)payload[0] == '3') {
    Serial.println("Cucumber");
    moisThreshold = 85;
    humidThreshold = 90;
    minTempThreshold = 25;
    maxTempThreshold = 30;
  }

  // Set Threshold while "Green Beans" is chosen
  if ((char)payload[0] == '5') {
    Serial.println("Green Beans");
    moisThreshold = 70;
    humidThreshold = 65;
    minTempThreshold = 20;
    maxTempThreshold = 25;
  }

  // Set Threshold while "Strawberry" is chosen
  if ((char)payload[0] == '6') {
    Serial.println("Strawberry");
    moisThreshold = 70;
    humidThreshold = 84;
    minTempThreshold = 18;
    maxTempThreshold = 22;
  }
  // Set Threshold while "Chili" is chosen
  if ((char)payload[0] == '7') {
    Serial.println("Chili");
    moisThreshold = 70;
    humidThreshold = 70;
    minTempThreshold = 25;
    maxTempThreshold = 28;
  }
  
  // Set Threshold while "Jicama" is chosen
  if ((char)payload[0] == '8') {
    Serial.println("Jicama");
    moisThreshold = 75;
    humidThreshold = 70;
    minTempThreshold = 23;
    maxTempThreshold = 27;
  }

  // Set Threshold while "Potato" is chosen
  if ((char)payload[0] == '9') {
    Serial.println("Potato");
    moisThreshold = 70;
    humidThreshold = 70;
    minTempThreshold = 20;
    maxTempThreshold = 22;
  }

  // Xu ly tin hieu thoi gian
   payload[length] = '\0'; // Make payload a string by NULL terminating it.
   int timeVal = atoi((char *)payload);
   if (timeVal >= 60000)
   {
      timeThreshold = timeVal;
      Serial.print("TimeThreshold: ");
      Serial.println(timeThreshold);
   }
}
//--------------GET TIME------------
int getTime(){
  dateTime = NTPch.getNTPtime(7.0, 0);
  if(dateTime.valid){
    //NTPch.printDateTime(dateTime);

    byte actualHour = dateTime.hour;
    byte actualMinute = dateTime.minute;
    byte actualsecond = dateTime.second;
    int actualyear = dateTime.year;
    byte actualMonth = dateTime.month;
    byte actualday = dateTime.day;
    byte actualdayofWeek = dateTime.dayofWeek;
    return (actualHour*60*60*1000 + actualMinute*60*1000 + actualsecond*1000);
  }
}
//--------------Start Action-----------
void itrWater()
{
  long thisTime = millis();
  if (lastWater - thisTime < 2000)
      digitalWrite(pumpPin, HIGH);
  thisTime = lastWater = 0;    
}

void itrMist()
{
  long thisTime = millis();
  if (lastMist - thisTime < 2000)
      digitalWrite(mistPin, HIGH);
  thisTime = lastMist = 0;  
}

void itrFan()
{
  long thisTime = millis();
  if (lastFan - thisTime < 2000)
      digitalWrite(fanPin, HIGH);
  thisTime = lastFan = 0;  
}

//------------RECONNECT-------------
void reconnect() {
  // Wait until connected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Send "Hello World" to topic "event"
      client.publish("event", "Hello World");
      // subscribe topic
      client.subscribe("event");
      client.subscribe("action");
      client.subscribe("time");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

//----------------WIFI SETUP----------------
void setup_wifi() {
  delay(10);
  // Wifi connecting 
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
