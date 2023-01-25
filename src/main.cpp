#include <Arduino.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include "Ultrasonic.h"
#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <EthernetUdp.h>
#include "pt.h"


// Ethernet setup
EthernetClient client;
EthernetUDP Udp;
byte mac[] = { 0XA8, 0X61, 0X0A, 0XAE, 0XA8, 0X56 };


const char* server = "192.168.1.105";
int portNumber = 8888;
const char* resource = "measurementconfig/1/config";

String http_response  = "";


//offset 1 houer 
const long utcOffsetInSeconds = 3600;

NTPClient timeClient(Udp, "0.dk.pool.ntp.org", utcOffsetInSeconds);



int totalCount = 0;

//LcdDisplay
rgb_lcd lcd;





int DoorOpencounter = 0;
const int button = 3;
Ultrasonic ultrasonic(7);
bool openOrclosed;
long cmrange = 5;
bool clear = 1;
bool IsEthernetOk = 0;

//timer set up
unsigned long startTime = 0;

//config Setup
const char* start_time;
const char* end_time;

bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  
  bool ok = client.find(endOfHeaders);
  

  if(!ok) {
    Serial.println("No response or invalid response!");
  }
  return ok;
}

void disconnect() {
  Serial.println("Disconnect");
  client.stop();
}


bool connect(const char* hostName, int portNumber) {
  Serial.print("Connect to ");
  Serial.println(hostName);

  bool ok = client.connect(hostName, portNumber);

  Serial.println(ok ? "Connected" : "Connection Failed!");
  return ok;
}

bool sendRequest(const char* host, const char* resource) {
  // Reserve memory space for your JSON data
  //StaticJsonDocument<200> jsonBuffer;
  // Build your own object tree in memory to store the data you want to send in the request
  const size_t capacity = JSON_OBJECT_SIZE(4);
  DynamicJsonDocument jsonBuffer(capacity);
  

  jsonBuffer["isOn"] = "1";
  jsonBuffer["device_id"] = "2";
  jsonBuffer["value"] = "1";  
  jsonBuffer["measuringUnit_id"] = "5";

  //POST request
  Serial.println("Begin POST Request");

  client.println("POST /alarm HTTP/2.0");
    Serial.println("POST /alarm HTTP/2.0");

  client.println("Host: 192.168.1.105");
    Serial.println("Host: 192.168.1.105");
  client.println("User-Agent: Arduino/1.0");
    Serial.println("User-Agent: Arduino/1.0");

  client.println("Content-Type: application/json");
    Serial.println("Content-Type: application/json");

  client.println("Connection: keep-alive");
    Serial.println("Connection: keep-alive");

  client.print("Content-Length: ");
    Serial.print("Content-Length: ");

  client.println(measureJson(jsonBuffer));
      Serial.println(measureJson(jsonBuffer));

  client.println();
  Serial.print(F("Sending: "));
  serializeJson(jsonBuffer, Serial);
  Serial.println();


  
  serializeJson(jsonBuffer, client);
  return true;
}

void SendDataToAPI()
{ 
  if(connect(server, portNumber)) {
    if(sendRequest(server, resource) && skipResponseHeaders()) {
      Serial.println("HTTP POST request finished.");
    }
  }
  disconnect(); 
}

void normalAlarm()
{  
  if(clear == 1){
    lcd.clear();
    lcd.setColorWhite();
    clear = 0;
    lcd.setRGB(255,0,0);
    lcd.setCursor(0, 0);
    lcd.print(F("Door open"));
    lcd.setCursor(0, 1);
    SendDataToAPI();
  } 

}

void GetConfig(){

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Getting"));
  lcd.setCursor(0,1);
  lcd.print(F("config"));

  if (!client.connect("192.168.1.105", 8888)) {
    Serial.println(F("Connection failed"));
    return;
  }

  // Send HTTP request
  client.println("GET /measurementconfig/1/config HTTP/2.0");
  client.println("Host: 192.168.1.105");
  client.println("Connection: keep-alive");
  client.println();
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    client.stop();
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  // It should be "HTTP/1.0 200 OK" or "HTTP/1.1 200 OK"
  if (strcmp(status + 9, "200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    client.stop();
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    client.stop();
    return;
  }

  // Allocate the JSON document
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  String payload = client.readString();
  DynamicJsonDocument doc(payload.length() * 2);

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    client.stop();
    return;
  }

  // Extract values
  start_time = doc["start_time"].as<const char*>();
  end_time = doc["end_time"].as<const char*>();

  // Disconnect
  client.stop();
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2); 
  lcd.print(F("Starting"));
  lcd.setCursor(0,1);
  lcd.print(F("ethernet"));
  Serial.print(F("Starting ethernet..."));
  while (!Ethernet.begin(mac))
  {
    Serial.println(F("failed"));
    if (IsEthernetOk == 0)
    {
      lcd.clear();
      lcd.setRGB(255,0,0);
      lcd.print(F("NO ETHERNET."));
      lcd.setCursor(0, 1);
      lcd.print(F("Reconnecting.."));
      IsEthernetOk = 1;
    }
    
    
  }  
  Serial.println(Ethernet.localIP());
  //starting up UDP for NTP.
  Udp.begin(8888);
  
  GetConfig();

  pinMode(button, INPUT_PULLUP);
  timeClient.begin();
  timeClient.update();  
  Serial.println(timeClient.getFormattedTime()); 
  millis();  
  attachInterrupt(digitalPinToInterrupt(button), GetConfig, FALLING);
}

void loop() {
  long RangeinCm;
  RangeinCm = ultrasonic.MeasureInCentimeters();
  if(RangeinCm <= 5){
    if (timeClient.getFormattedTime() < start_time)
    {
      
      if (openOrclosed == 0)
      {
        openOrclosed = 1;
        normalAlarm();
      }     
      
    }
    if (timeClient.getFormattedTime() > end_time)
    {
      if (openOrclosed == 0)
      {
        Serial.println(F("inde i efter 16"));
        openOrclosed = 1;
        normalAlarm();
      }
      
      
    }    
    else{
      if(openOrclosed == 0){
        DoorOpencounter++;
        openOrclosed = 1;
        lcd.setRGB(255,255,0);
        startTime = millis();
      }
      if(millis() - startTime >= 5000){
        normalAlarm();
      }
    }       
      
  }
  else{    
    openOrclosed = 0;
     lcd.setRGB(0,255,0);
     lcd.clear(); 
     startTime = 0;
     clear = 1;
   }
}



