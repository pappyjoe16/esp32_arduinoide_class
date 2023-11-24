#include <WiFi.h>
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
