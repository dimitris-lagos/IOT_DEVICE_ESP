#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <Ticker.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>

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
void XMLcontent();
void returnOK();
void handlePageFromSD();
void setAPmode();
void setupWebServerOnRequests();
bool loadFromSdCard(String path);
void handleFileUpload();
void handleNotFound();
String file_size(int bytes);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void setRouterConnect();
void setAPmode();
DhtSensorData readDhtSensorData();
boolean intervalPassed(uint32_t* previousMillis, uint32_t interval);
void logDataToSD(String fileName, String logData);