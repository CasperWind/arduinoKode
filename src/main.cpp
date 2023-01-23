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
int portNumber = 80;
const char* resource = "/alarm/create.php";



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
  const size_t capacity = JSON_OBJECT_SIZE(5);
  DynamicJsonDocument jsonBuffer(capacity);
  

  jsonBuffer["date_time"] = "000000";
  jsonBuffer["isOn"] = "1";
  jsonBuffer["device_id"] = "1";
  jsonBuffer["value"] = "NULL";  
  jsonBuffer["measuringUnit_id"] = "5";

  //POST request
  Serial.println("Begin POST Request");

  client.println("POST /alarm/create.php HTTP/2.0");
    Serial.println("POST /alarm/create.php HTTP/2.0");

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
    //SendDataToAPI();
    lcd.setRGB(255,0,0);
    lcd.setCursor(0, 0);
    lcd.print(F("Door open"));
    lcd.setCursor(0, 1);
  } 

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
  
  pinMode(button, INPUT);
  timeClient.begin();
  timeClient.update();  
  Serial.println(timeClient.getFormattedTime()); 
  millis();  
}

void loop() {
  long RangeinCm;
  RangeinCm = ultrasonic.MeasureInCentimeters();
  if(RangeinCm <= 5){
    if (timeClient.getHours() <= 8 && timeClient.getMinutes() >= 0)
    {
      if (openOrclosed == 0)
      {
        openOrclosed = 1;
        normalAlarm();
      }     
      
    }
    if (timeClient.getHours() >= 16 && timeClient.getMinutes() >= 0)
    {
      if (openOrclosed == 0)
      {
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



