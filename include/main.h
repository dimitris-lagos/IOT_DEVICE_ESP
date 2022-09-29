#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <Ticker.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
// #include "esp_log.h"


#define DBG_OUTPUT_PORT Serial
#define DHTTYPE DHT11
#define MIN_INTERVAL 60000


#define SD_CS_PIN D8
#define SD_MOSI_PIN D7
#define SD_MISO_PIN D6
#define SD_SCK_PIN D5
#define DHTPIN D3 


typedef struct DhtSensorData{
  float temperature;
  float humidity;
}DhtSensorData;


DHT dht(DHTPIN, DHTTYPE);
boolean initSD();
void handleXMLRequest(AsyncWebServerRequest *request);
void returnOK(AsyncWebServerRequest *request);
void handlePageFromSD();
void setAPmode();
void setupWebServerOnRequests();
bool loadFromSdCard(AsyncWebServerRequest *request);
void handleFileUpload();
void handleNotFound();
String file_size(int bytes);
void setRouterConnect();
void setAPmode();
DhtSensorData readDhtSensorData();
boolean intervalPassed(uint32_t* previousMillis, uint32_t interval);
void logDataToSD(String fileName, String logData);
void wakeMyPC();
void handleListSDCardFiles(AsyncWebServerRequest *request);
String getSDCardList(File dir);
void handleGetRequest(AsyncWebServerRequest *request);
size_t load_data(File f, uint8_t* buffer, size_t maxLen, size_t index);
void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t length);
void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void onRequest(AsyncWebServerRequest *request);