#include <WiFi.h>
#include <Arduino_JSON.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//define rest and sda pin for RFID
#define SS_PIN 5
#define RST_PIN 0

//define sound speed in cm/uS
#define SOUND_SPEED 0.0343
//#define CM_TO_INCH 0.393701

//Assign Sensors pin
const int trigPin = 2;
const int echoPin = 4;

//Configure wifi network credential
//const char* ssid = "3Bredband-0390";
//const char* password = "8593R37J82N";

const char* ssid = "Pappy_Joe";
const char* password = "@12345678";

//Preparing NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

// Domain Name with full URL Path for HTTP POST Request
const char* serverName = "http://api.thingspeak.com/update";
const char* getServerName = "https://api.thingspeak.com/channels/2354652/fields/1.json?api_key=C6MMSHYUIJVDK29E&results=2";
// Service API Key
String apiKey = "3I4TOFFU4D84CKPM";
String getApiKey = "C6MMSHYUIJVDK29E";

MFRC522 rfid(SS_PIN, RST_PIN);  // Instance of the class
MFRC522::MIFARE_Key key;

//Variable declaration
long duration;
float distanceCm;
float distanceInch;
String formattedDate;
String dayStamp;
String timeStamp;
String httpRequestData;

//create ajson object to hold data keys and value
JSONVar DataObject;
JSONVar getDataObject;
volatile bool cardDetected = false;
// Init array that will store new NUID
byte nuidPICC[4];

void setup() {
  Serial.begin(115200);      // Starts the serial communication
  SPI.begin();               // Init SPI bus
  rfid.PCD_Init();           // Init MFRC522
  pinMode(trigPin, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);   // Sets the echoPin as an Input

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  attachInterrupt(digitalPinToInterrupt(SS_PIN), cardInterrupt, FALLING);
  delay(2000);
  wifi_connect();  //connect to wifi network
  delay(500);
  timeClient.begin();  //initialize the NTP client to get date and time from an NTP server
  delay(4000);

  // get distance from the sensor
  Serial.print("Distance (cm): ");
  Serial.println(String(measure_distance_cm()) + " cm");  //get measurement in cm

  DataObjectCreation();  //assign data value to data key

  //sendData();
  getData();
}

void loop() {
  if (cardDetected) {
    getRFIDNumber();
    cardDetected = false; // Reset the flag
  }
}

//Function to create wifi connection to the access point
void wifi_connect() {

  WiFi.mode(WIFI_AP_STA);  //Optional
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");#include <WiFi.h>
#include <Arduino_JSON.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SS_PIN 5
#define RST_PIN 0
#define SOUND_SPEED 0.0343
#define TRIG_PIN 2
#define ECHO_PIN 4
#define WIFI_SSID "Pappy_Joe"
#define WIFI_PASSWORD "@12345678"
#define API_KEY "3I4TOFFU4D84CKPM"
#define SERVER_NAME "http://api.thingspeak.com/update"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

float sharedVariable = 0.00;
String sharedrfidString = "";
bool runTask2 = false;

TaskHandle_t getRFIDHandle;
TaskHandle_t wasteLevelHandle;
TaskHandle_t sendDataHourlyHandle;
TaskHandle_t sendDataHandle;

SemaphoreHandle_t mutex;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  mutex = xSemaphoreCreateMutex();


  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  int taskId1 = 1, taskId2 = 2, taskId3 = 3, taskId4 = 4;
  xTaskCreatePinnedToCore(getRFID, "getRFID", 6144, &taskId1, 1, &getRFIDHandle, 0);
  xTaskCreatePinnedToCore(wasteLevel, "wasteLevel", 4096, &taskId2, 2, &wasteLevelHandle, 0);
  xTaskCreatePinnedToCore(sendDataHourly, "sendDataHourly", 5120, &taskId3, 3, &sendDataHourlyHandle, 0);
  xTaskCreatePinnedToCore(sendData, "sendData", 8192, &taskId4, 4, &sendDataHandle, 1);


  vTaskStartScheduler();
}

void loop() {
  // Nothing to be done here
}

void getRFID(void *parameter) {
  (void)parameter;
  byte nuidPICC[4];


  for (;;) {
    // if (xSemaphoreTake(mutex, portMAX_DELAY)) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      for (byte i = 0; i < 4; i++) {
        nuidPICC[i] = rfid.uid.uidByte[i];
      }

      String nuidString = "";
      for (byte i = 0; i < 4; i++) {
        nuidString += String(nuidPICC[i], HEX);
      }
      // xSemaphoreGive(mutex);
      if (xSemaphoreTake(mutex, portMAX_DELAY)) {
        // Set the shared variable
        sharedrfidString = nuidString;
        runTask2 = true;
        // Release the mutex
        xSemaphoreGive(mutex);
      }
      Serial.println("This is RFID: " + nuidString);
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();

      vTaskResume(sendDataHandle);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    // }
    // Optionally yield to other tasks
    // taskYIELD();
  }
}

void wasteLevel(void *parameter) {
  (void)parameter;
  long duration;
  float distanceCm;
  pinMode(TRIG_PIN, OUTPUT);  // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT);   // Sets the echoPin as an Input
  for (;;) {
    //if (xSemaphoreTake(mutex, portMAX_DELAY)) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH);
    distanceCm = duration * SOUND_SPEED / 2;
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
      // Set the shared variable
      sharedVariable = distanceCm;

      // Release the mutex
      xSemaphoreGive(mutex);
    }

    Serial.println("This is distance: " + String(distanceCm));
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
  // Optionally yield to other tasks
  //taskYIELD();
  //}
}

void sendDataHourly(void *parameter) {
  (void)parameter;
  const uint32_t oneHourInSeconds = 720;
  uint32_t count = 0;

  for (;;) {
    count++;

    if (count >= oneHourInSeconds) {
      vTaskResume(sendDataHandle);
      count = 0;
    }
    Serial.println("This is count: " + String(count));
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void sendData(void *parameter) {
  (void)parameter;
  WiFi.mode(WIFI_AP_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  for (;;) {
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print("..");
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    Serial.println("\nConnected to the WiFi network");
    while (!timeClient.update()) {
      timeClient.forceUpdate();
    }

    String formattedDate = timeClient.getFormattedDate();

    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
      // Get the shared variable
      JSONVar sendDataObject;
      float measureValue = sharedVariable;
      String rfidSharedString = sharedrfidString;
      // Release the mutex
      xSemaphoreGive(mutex);
      Serial.println("This is the Shared measured value: " + String(measureValue));
      Serial.println("This is the shared RFID number: " + rfidSharedString);
      Serial.println("This is the time: " + formattedDate);

      sendDataObject["api_key"] = API_KEY;
      sendDataObject["Device_macAddress"] = "12:bc:3c:58";
      sendDataObject["TimeStamp"] = formattedDate;
      sendDataObject["RFID_number"] = rfidSharedString;  // Replace with the actual RFID value
      sendDataObject["BinStatus"] = measureValue;
      sendDataObject["BinID"] = "0001-AAA";

      String jsonDataString = JSON.stringify(sendDataObject);
      Serial.println(jsonDataString);

      if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, SERVER_NAME);
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(jsonDataString);
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        http.end();
      }
      vTaskDelay(pdMS_TO_TICKS(1000));

      vTaskSuspend(NULL);


      //     // Optionally yield to other tasks
      //     taskYIELD();
    }
    ///}
  }
}

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
  DataObject["api_key"] = (apiKey);
  DataObject["Device_macAddress"] = "12:bc:3c:58";
  DataObject["TimeStamp"] = getTimeDateStamp();
  DataObject["RFID_number"] = "0x7ce35f9";
  DataObject["BinStatus"] = (distanceCm);
  DataObject["BinID"] = "0001-AAA";

  // DataObject["api_key"] = (apiKey);
  // DataObject["field1"] = "12:bc:3c:58";
  // DataObject["field2"] = "0x7ce35f9";
  // DataObject["field3"] = (distanceCm);
  // DataObject["field4"] = "0001-AAA";
  // DataObject["field5"] = getTimeDateStamp();
  // convert the Json object to string
  String jsonDataString = JSON.stringify(DataObject);
  Serial.println(jsonDataString);
  httpRequestData = jsonDataString;
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
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  Serial.println(timeStamp);
  delay(1000);

  return formattedDate;
}

void sendData() {
  //Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);

    http.addHeader("Content-Type", "application/json");
    // JSON data to send with HTTP POST

    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    // Free resources
    http.end();
  }
}

void getData() {
  //Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    // Your Domain name with URL path or IP address with path
    http.begin(getServerName);
    getDataObject["api_key"] = (getApiKey);
    getDataObject["results"] = 2;
    http.addHeader("Content-Type", "application/json");
    // JSON data to send with HTTP POST
    String getJsonDataString = JSON.stringify(getDataObject);
    Serial.println(getJsonDataString);
    // Send HTTP POST request
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
}

void getRFIDNumber() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if (!rfid.PICC_ReadCardSerial())
    return;

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }

  // Convert byte array to String
  String nuidString = "";
  for (byte i = 0; i < 4; i++) {
    nuidString += String(nuidPICC[i], HEX);
  }

  Serial.print(F("In hex: "));
  Serial.print(nuidString);
  Serial.println();

  rfid.PICC_HaltA();       // Halt PICC
  rfid.PCD_StopCrypto1();  // Stop encryption on PCD
}
void cardInterrupt() {
  // Function to handle the RFID card detection interrupt
  cardDetected = true;
}
