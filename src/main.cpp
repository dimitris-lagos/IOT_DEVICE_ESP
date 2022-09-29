#include "main.h"
#include "page1.h"

const long utcOffsetInSeconds = 0; // 7200 for greece(UTC+2)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
File uploadFile;
File webFile;
bool hasSD = false;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81); // create a websocket server on port 81
MDNSResponder mdns;
short cnt = 0;
char ssid[] = "MY_SSID";
char password[] = "MY_PASSWORD";
Ticker tick;
IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
uint32_t currentMills = 0;
uint32_t previousMillis = 0;
String jsonSerial;

uint32_t time_lap = 0;
float secs = 0.00;
//************************************************************************************//
//             Setup Function                                                         //
//************************************************************************************//
void setup()
{
  analogWriteFreq(100);
  setRouterConnect();
  pinMode(LED_BUILTIN, OUTPUT);
  analogWrite(LED_BUILTIN, 364);
  dht.begin();
  /* SD Card Init  */
  hasSD = initSD();
  if (!MDNS.begin("esp8266"))
  {
    DBG_OUTPUT_PORT.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  DBG_OUTPUT_PORT.println("mDNS responder started. Visit:");
  DBG_OUTPUT_PORT.println("http://esp8266.local/");

  setupWebServerOnRequests();
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent); // handle websocket
//  Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  timeClient.update();
  unsigned long time = timeClient.getEpochTime();
  logDataToSD("datalog.txt", "time:" + String(time) + "\n");
  DBG_OUTPUT_PORT.println(time);
}

//************************************************************************************//
//             Loop Function                                                          //
//************************************************************************************//
void loop()
{
  MDNS.update();
  webSocket.loop();
  server.handleClient(); // Handling incoming client requests
  if (DBG_OUTPUT_PORT.available() > 0)
  {
    char c[] = {(char)DBG_OUTPUT_PORT.read()};

    webSocket.broadcastTXT(c, sizeof(c));
  }
  if (intervalPassed(&previousMillis, MIN_INTERVAL))
  {
    DhtSensorData tempData = readDhtSensorData();
    String jsonString = "{\"type\": \"sensor\",\"temperature\":" + String(tempData.temperature) + ",\"humidity\":" + String(tempData.humidity) + "}";
    String dataLog = String(tempData.temperature) + "," + String(tempData.humidity) + "\n";
    webSocket.broadcastTXT(jsonString);
    logDataToSD("datalog.txt", dataLog);
    DBG_OUTPUT_PORT.println(jsonString);
  }
}

//************************************************************************************//
//             Setup Webserver "on" requests                                          //
//************************************************************************************//
void setupWebServerOnRequests()
{
  server.on("/cached", []()
            { server.send_P(200, "text/html", webpage); });

  server.on("/get", HTTP_GET, []()
            {
          String state = server.arg("input1");
          server.send(200, "text/plain", "You sent: " + state); });
  server.on("/xml", XMLcontent); // handle ajax

  server.on(
      "/edit", HTTP_POST, []()
      { returnOK(); },
      handleFileUpload);
  server.onNotFound(handleNotFound);
}

//************************************************************************************//
//            Init SD Card: Returs true if SD Card is present, else false             //
//************************************************************************************//
boolean initSD()
{
  boolean sdInit = false;
  if (SD.begin(SD_CS_PIN))
  {
    DBG_OUTPUT_PORT.println("SD Card initialized.");
    sdInit = true;
    if (!SD.exists("index.htm"))
    {
      DBG_OUTPUT_PORT.println("ERROR - Can't find index.htm file!");
    }
  }
  else
    DBG_OUTPUT_PORT.println("No SD Card found!");
  return sdInit;
}

//************************************************************************************//
//    xxxNOT_USEDxxx  Handle loading and sending index.htm from the SD                //
//************************************************************************************//
void handlePageFromSD()
{
  if (hasSD)
  {
    if (!SD.exists("index.htm"))
    {
      server.send(404, "text/plain", "Page Not Found");
      DBG_OUTPUT_PORT.println("ERROR - Can't find index.htm file!");
      return;
    }
    webFile = SD.open("index.htm");
    String page;
    while (webFile.available())
    {
      char ltr = webFile.read();
      page += ltr;
    }
    server.send(200, "text/html", page);
    webFile.close();
  }
  else
  {
    server.send(404, "text/plain", "SD Card Not Found");
  }
}

//************************************************************************************//
//                Handle creation and sending of the XML Content                      //
//************************************************************************************//
void XMLcontent()
{

  String XML;
  if (cnt < 20)
  {
    XML = "<?xml version='1.0'?>";
    XML += "<status>";
    XML += "<mode>";
    XML += "WIFI STATION";
    XML += "</mode>";
    XML += "<ip>";
    XML += WiFi.localIP().toString();
    XML += "</ip>";
    // XML+="<ssid>";
    // XML+=WiFi.SSID().c_str();
    // XML+="</ssid>";
    XML += "</status>";
    Serial.print("Wireless Client IP: ");
    Serial.println(XML);
  }
  else
  {
    XML = WiFi.softAPIP().toString();
    Serial.print("Access Point IP: ");
    Serial.println(XML);
    XML = "<?xml version='1.0'?>";
    XML += "<status>";
    XML += "<mode>";
    XML += "WiFi Access Point";
    XML += "</mode>";
    XML += "<ip>";
    XML += WiFi.softAPIP().toString();
    XML += "</ip>";
    XML += "</status>";
  }
  DBG_OUTPUT_PORT.print("Client connected: ");
  DBG_OUTPUT_PORT.println(server.client().remoteIP());
  server.send(200, "text/xml", XML);
}

//************************************************************************************//
//            Set the ESP to act as an Access Point                                   //
//************************************************************************************//
void setAPmode()
{
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_AP);
  /*WiFi.softAP(ssid, password, channel, hidden, max_connection)*/
  WiFi.softAP("ESP8266", "012345678", 6, false, 4);
  /*WiFi.softAPConfig (local_ip, gateway, subnet)*/
  WiFi.softAPConfig(local_ip, gateway, subnet);
}

//************************************************************************************//
//            Set the ESP to act as wifi client and connect to my router              //
//************************************************************************************//
void setRouterConnect()
{
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.hostname("ESP8266 Server");
  WiFi.begin(ssid, password);
  Serial.begin(115200);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
    cnt++;
    if (cnt > 20)
      break;
  }
  if (cnt > 20)
  {
    setAPmode();
    DBG_OUTPUT_PORT.println("");
    DBG_OUTPUT_PORT.print("IP Address: ");
    DBG_OUTPUT_PORT.println(WiFi.softAPIP());
    DBG_OUTPUT_PORT.println("Pass: 012345678");
  }
  else
  {
    DBG_OUTPUT_PORT.println("");
    DBG_OUTPUT_PORT.print("IP Address: ");
    DBG_OUTPUT_PORT.println(WiFi.localIP());
    DBG_OUTPUT_PORT.print("Connected to: ");
    DBG_OUTPUT_PORT.println(WiFi.SSID());
  }
}

void logDataToSD(String fileName, String logData)
{
  if (hasSD)
  {
    File dataLog = SD.open(fileName.c_str(), FILE_WRITE);
    if (dataLog)
    {
      dataLog.print(logData);
    }
    else
    {
      DBG_OUTPUT_PORT.println(fileName + "not found on SD!");
    }
    dataLog.close();
  }
  else
  {
    DBG_OUTPUT_PORT.println("(Trying to write to file)No SD Card found!");
  }
}

DhtSensorData readDhtSensorData()
{
  DhtSensorData datastruct;
  datastruct.temperature = dht.readTemperature();
  datastruct.humidity = dht.readHumidity();
  return datastruct;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_CONNECTED)
  {
    DhtSensorData tempData = readDhtSensorData();
    String jsonString = "{\"type\": \"sensor\",\"temperature\":" + String(tempData.temperature) + ",\"humidity\":" + String(tempData.humidity) + "}";
    // webSocket.broadcastTXT(jsonString);
  }
  else if (type == WStype_TEXT && length > 0)
  {
    if (payload[0] == '#')
    {
      uint16_t fanpwm = (uint16_t)strtol((const char *)&payload[1], NULL, 10);
      fanpwm = 1024 - fanpwm;
      analogWrite(LED_BUILTIN, fanpwm);
      DBG_OUTPUT_PORT.print("fanpwm= ");
      DBG_OUTPUT_PORT.println(fanpwm);
    }
    else
    {
      DBG_OUTPUT_PORT.print("Serial Received: ");
      for (unsigned int i = 0; i < length; i++)
      {
        DBG_OUTPUT_PORT.print((char)payload[i]);
      }
      DBG_OUTPUT_PORT.println();
    }
  }
}
void returnOK()
{
  server.send(200, "text/plain", "");
}

void returnFail(String msg)
{
  server.send(500, "text/plain", msg + "\r\n");
}

void handleFileUpload()
{
  if (server.uri() != "/edit")
  {
    server.send(404, "text/plain", "Wrong Page!");
    return;
  }
  if (!hasSD)
  {
    server.send(404, "text/plain", "SD Card Not Found!");
    return;
  }
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    time_lap = millis();
    DBG_OUTPUT_PORT.println(time_lap);
    secs = 0.00;
    if (SD.exists((char *)upload.filename.c_str()))
    {
      SD.remove((char *)upload.filename.c_str());
    }
    uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);
    DBG_OUTPUT_PORT.print("Upload: START, filename: ");
    DBG_OUTPUT_PORT.println(upload.filename);
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (uploadFile)
    {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (uploadFile)
    {
      uploadFile.close();
      time_lap = millis() - time_lap;
      DBG_OUTPUT_PORT.println(time_lap);
      secs = (time_lap / 1000.00);
      String msg = String(secs, 2);
      String webpagea = "";
      webpagea += F("<h3>File was successfully uploaded</h3>");
      webpagea += F("<h2>Uploaded File Name: ");
      webpagea += upload.filename + "</h2>";
      webpagea += F("<h2>File Size: ");
      webpagea += file_size(upload.totalSize) + "</h2>";
      webpagea += F("<h2>Time elapsed: ");
      webpagea += msg;
      webpagea += " seconds</h2><br>";
      server.send(200, "text/html", webpagea);
    }
    DBG_OUTPUT_PORT.print("Upload: END, Size: ");
    DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleNotFound()
{
  if (hasSD && loadFromSdCard(server.uri()))
  {
    return;
  }
  else if (!hasSD & server.uri().endsWith("/"))
  {
    server.send_P(200, "text/html", webpage);
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

bool loadFromSdCard(String path)
{
  String dataType = "text/plain";
  if (path.endsWith("/"))
  {
    path += "index.htm";
    dataType = "text/html";
  }
  else if (path.endsWith(".src"))
  {
    path = path.substring(0, path.lastIndexOf("."));
  }
  else if (path.endsWith(".htm"))
  {
    dataType = "text/html";
  }
  else if (path.endsWith(".css"))
  {
    dataType = "text/css";
  }
  else if (path.endsWith(".js"))
  {
    dataType = "application/javascript";
  }
  else if (path.endsWith(".png"))
  {
    dataType = "image/png";
  }
  else if (path.endsWith(".gif"))
  {
    dataType = "image/gif";
  }
  else if (path.endsWith(".jpg"))
  {
    dataType = "image/jpeg";
  }
  else if (path.endsWith(".ico"))
  {
    dataType = "image/x-icon";
  }
  else if (path.endsWith(".xml"))
  {
    dataType = "text/xml";
  }
  else if (path.endsWith(".pdf"))
  {
    dataType = "application/pdf";
  }
  else if (path.endsWith(".zip"))
  {
    dataType = "application/zip";
  }

  File dataFile = SD.open(path.c_str());
  if (dataFile.isDirectory())
  {
    dataFile.close();
    path += "/index.htm";
    dataType = "text/html";
    dataFile = SD.open(path.c_str());
  }

  if (!dataFile)
  {
    return false;
  }

  if (server.hasArg("download"))
  {
    dataType = "application/octet-stream";
  }

  if (server.streamFile(dataFile, dataType) != dataFile.size())
  {
    DBG_OUTPUT_PORT.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}

//************************************************************************************//
//            Utility Functions                                                       //
//************************************************************************************//
String file_size(int bytes)
{
  String fsize = "";
  if (bytes < 1024)
    fsize = String(bytes) + " B";
  else if (bytes < (1024 * 1024))
    fsize = String(bytes / 1024.0, 3) + " KB";
  else if (bytes < (1024 * 1024 * 1024))
    fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
  else
    fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
  return fsize;
}

boolean intervalPassed(uint32_t *previousMillis, uint32_t interval)
{
  boolean passed = false;
  uint32_t currentMills = millis();
  if (currentMills - *previousMillis >= interval)
  {
    *previousMillis = currentMills;
    passed = true;
  }
  return passed;
}
