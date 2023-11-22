#include <WiFi.h>
#include <Arduino_JSON.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "math.h"

//Assign Sensors pin
const int trigPin = 5;
const int echoPin = 4;

//Configure wifi network credential
//const char* ssid = "3Bredband-0390";
//const char* password = "8593R37J82N";

const char* ssid = "Pappy_Joe";
const char* password = "@12345678";

//Preparing NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

//define sound speed in cm/uS
#define SOUND_SPEED 0.0343
//#define CM_TO_INCH 0.393701

//Variable declaration
long duration;
float distanceCm;
float distanceInch;
String formattedDate;
String dayStamp;
String timeStamp;

//create ajson object to hold data keys and value
JSONVar DataObject;

void setup() {
  Serial.begin(115200);      // Starts the serial communication
  pinMode(trigPin, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);   // Sets the echoPin as an Input

  delay(2000);
  wifi_connect();  //connect to wifi network
  delay(500);
  timeClient.begin();  //initialize the NTP client to get date and time from an NTP server
  delay(4000);

  // get distance from the sensor
  Serial.print("Distance (cm): ");
  Serial.println(String(measure_distance_cm()) + " cm");  //get measurement in cm

  DataObjectCreation(); //assign data value to data key
}

void loop() {
  // Prints the distance in the Serial Monitor
}

//Function to create wifi connection to the access point
void wifi_connect() {

  WiFi.mode(WIFI_AP_STA);  //Optional
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

 //waiting for connection to be established
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("..");
    delay(100);
  }
  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
  delay(1000);
}

//function to measure the distance in cm
float measure_distance_cm() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculate the distance
  distanceCm = duration * SOUND_SPEED / 2;

  return distanceCm;
}

//Function that Prepare Data to send 
void DataObjectCreation() {
  //setting json key and value
  DataObject["Device_macAddress"] = "12:bc:3c:58";
  DataObject["TimeStamp"] =  getTimeDateStamp();
  DataObject["RFID_number"] = "0x7ce35f9";
  DataObject["BinStatus"] = (distanceCm);
  DataObject["BinID"] = "0001-AAA";

 // convert the Json object to string
  String jsonDataString = JSON.stringify(DataObject);
  Serial.println(jsonDataString);
}

//Function to get date and time
String getTimeDateStamp() {
  //update the timeclient
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // get time and date in this format 2023-11-22T01:38:14Z
  formattedDate = timeClient.getFormattedDate();

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.println(timeStamp);
  delay(1000);
  
  return formattedDate;
}