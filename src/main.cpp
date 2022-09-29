#include "main.h"
#include "page1.h"


const long utcOffsetInSeconds = 0; // 7200 for greece(UTC+2)
WiFiUDP ntpUDP;
WakeOnLan WOL(ntpUDP);
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
File uploadFile;
File webFile;
bool hasSD = false;
AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");
MDNSResponder mdns;
short cnt = 0;
char ssid[] = "SSID";
char password[] = "WPA_PASS";
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
// esp_log_level_set("*", ESP_LOG_ERROR);        // set all components to ERROR level
// esp_log_level_set("wifi", ESP_LOG_WARN);      // enable WARN logs from WiFi stack
// esp_log_level_set("dhcpc", ESP_LOG_INFO);     // enable INFO logs from DHCP client

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
  WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask()); // Optional  => To calculate the broadcast address, otherwise 255.255.255.255 is used (which is denied in some networks).
  setupWebServerOnRequests();

    // attach AsyncWebSocket
  webSocket.onEvent(onWebSocketEvent);// handle websocket
  server.addHandler(&webSocket);
  server.begin();

//  Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  timeClient.update();
  unsigned long time = timeClient.getEpochTime();
  logDataToSD("datalog.txt", "time:" + String(time) + "\n");
}
//************************************************************************************//
//             Loop Function                                                          //
//************************************************************************************//
void loop()
{
  MDNS.update();
  if (DBG_OUTPUT_PORT.available() > 0)
  {
    char c[] = {(char)DBG_OUTPUT_PORT.read()};
    webSocket.textAll(c, sizeof(c));
  }
  if (intervalPassed(&previousMillis, MIN_INTERVAL))
  {
    DhtSensorData tempData = readDhtSensorData();
    String jsonString = "{\"type\": \"sensor\",\"temperature\":" + String(tempData.temperature) + ",\"humidity\":" + String(tempData.humidity) + "}";
    String dataLog = String(tempData.temperature) + "," + String(tempData.humidity) + "\n";
    webSocket.textAll(jsonString);
    logDataToSD("datalog.txt", dataLog);
    DBG_OUTPUT_PORT.println(jsonString);
  }
}

//************************************************************************************//
//             Setup Webserver "on" requests                                          //
//************************************************************************************//
void setupWebServerOnRequests()
{
  server.on("/cached", [](AsyncWebServerRequest *request){ 
     request->send_P(200, "text/html", webpage); });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
    //Check if GET parameter exists
    if(request->hasParam("input1")){
      handleGetRequest(request);
    }
  });
  server.on("/xml", [](AsyncWebServerRequest *request) {
    handleXMLRequest(request);}); // handle ajax

  server.on("/edit", HTTP_POST, [](AsyncWebServerRequest *request)
      { returnOK(request); }, onUpload);

  server.onNotFound(onRequest);
}

void handleGetRequest(AsyncWebServerRequest *request){
  AsyncWebParameter* p = request->getParam("input1");
  String argValue = p->value().c_str();

  if(argValue.equals("wol")){
    wakeMyPC();
    request->send(200, "text/plain", "Trying to wake up PowerPc");
  }else if(argValue.equals("sdcard")){
    handleListSDCardFiles(request);
  }
  else{
    request->send(400, "text/plain", "");
  }
}

void handleListSDCardFiles(AsyncWebServerRequest *request){
  if(hasSD){
    String topHtml = "<!DOCTYPE html><html><style>table, th, td {border:1px solid black;}</style><body><h2>SD Card Contents</h2><table style=\"width:100%\"><tr><th>File Name</th><th>File Size</th><th>Created</th></tr>";
    String bottomHtml = "</table></body></html>";
    File root = SD.open("/");
    topHtml.concat(getSDCardList(root));
    topHtml.concat(bottomHtml);

    request->send(200, "text/html", topHtml);
  }else{
    request->send(500, "text/plain", "");
  }
}

String getSDCardList(File dir) {
  String html ;
  File entry;
  while (entry = dir.openNextFile()) {
    html.concat("<tr>");
    html.concat("<td>");
    html.concat("<p><a href=\"/");
    html.concat(entry.name());
    html.concat("\">");
    html.concat(entry.name());
    html.concat("</a></p>");
    html.concat("</td>");

    html.concat("<td>");
    html.concat(file_size(entry.size()));
    html.concat("</td>");

    html.concat("<td>");
    time_t cr = entry.getCreationTime();
    struct tm* tmstruct = localtime(&cr);
    char creationDate[22];
    sprintf(creationDate,"%02d-%02d-%d  %02d:%02d:%02d", tmstruct->tm_mday, (tmstruct->tm_mon) + 1, (tmstruct->tm_year) + 1900, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    html.concat(creationDate);
    html.concat("</td>");

    html.concat("</tr>");
    entry.close();
    }
  return html;
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
void handlePageFromSD(AsyncWebServerRequest *request)
{
  if (hasSD)
  {
    if (!SD.exists("index.htm"))
    {
      request->send(404, "text/plain", "Page Not Found");
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
    request->send(200, "text/html", page);
    webFile.close();
  }
  else
  {
    request->send(404, "text/plain", "SD Card Not Found");
  }
}

//************************************************************************************//
//                Handle creation and sending of the XML Content                      //
//************************************************************************************//
void handleXMLRequest(AsyncWebServerRequest *request)
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
  IPAddress client_ip = request->client()->remoteIP();
  DBG_OUTPUT_PORT.println(client_ip);
  request->send(200, "text/xml", XML);
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

void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t length)
{
  if (type == WS_EVT_CONNECT)
  {
    //DhtSensorData tempData = readDhtSensorData();
    //String jsonString = "{\"type\": \"sensor\",\"temperature\":" + String(tempData.temperature) + ",\"humidity\":" + String(tempData.humidity) + "}";
    // webSocket.broadcastTXT(jsonString);
  }
  else if (type == WS_EVT_DATA && length > 0)
  {
    if (data[0] == '#')
    {
      uint16_t fanpwm = (uint16_t)strtol((const char *)&data[1], NULL, 10);
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
        DBG_OUTPUT_PORT.print((char)data[i]);
      }
      DBG_OUTPUT_PORT.println();
    }
  }
}

void returnOK(AsyncWebServerRequest *request)
{
  request->send(200, "text/plain", "");
}

void returnFail(AsyncWebServerRequest *request, String msg)
{
  request->send(500, "text/plain", msg + "\r\n");
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (request->url() != "/edit")
  {
    request->send(404, "text/plain", "Wrong Page!");
    return;
  }
  if (!hasSD)
  {
    request->send(404, "text/plain", "SD Card Not Found!");
    return;
  }

  if (!index)
  {
    time_lap = millis();
    secs = 0.00;
    if (SD.exists((char *)filename.c_str()))
    {
      SD.remove((char *)filename.c_str());
    }
    filename.replace(' ','_');
    uploadFile = SD.open(filename.c_str(), FILE_WRITE);
    request->_tempFile = uploadFile;
    DBG_OUTPUT_PORT.print("Upload: START, filename: ");
    DBG_OUTPUT_PORT.println(filename);
  }
  if (len)
  {
    if (uploadFile)
    {
      uploadFile.write(data, len);
    }
  }
  if (final)
  {
    if (uploadFile)
    {
      request->_tempFile.close();
      time_lap = millis() - time_lap;
      secs = (time_lap / 1000.00);
      String msg = String(secs, 2);
      String webpagea = "";
      webpagea += F("<h3>File was successfully uploaded</h3>");
      webpagea += F("<h2>Uploaded File Name: ");
      webpagea += filename + "</h2>";
      webpagea += F("<h2>File Size: ");
      webpagea += file_size(index + len) + "</h2>";
      webpagea += F("<h2>Time elapsed: ");
      webpagea += msg;
      webpagea += " seconds</h2><br>";
      request->send(200, "text/html", webpagea);
    }
    DBG_OUTPUT_PORT.print("Upload: END, Size: ");
    DBG_OUTPUT_PORT.println( String(index + len));
  }
}

void onRequest(AsyncWebServerRequest *request)
{
  String uri = request->url().c_str();
  if (hasSD && loadFromSdCard(request)){
    return;
  }
  else if (!hasSD & uri.endsWith("/")){
    request->send_P(200, "text/html", webpage);
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += uri;
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  int args = request->args();
  for (uint8_t i = 0; i < args; i++)
  {
    String argName = request->argName(i).c_str();
    String argValue  = request->arg(i).c_str();
    message += " " + argName + ": " + argValue + "\n";
  }

  request->send(404, "text/plain", message);
}

bool loadFromSdCard(AsyncWebServerRequest *request)
{
  String path = request->url().c_str();
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

  if (request->hasParam("download"))
  {
    dataType = "application/octet-stream";
  }


  // AsyncWebServerResponse *response = request->beginChunkedResponse("application/octet-stream", [dataFile](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
  //   return load_data(dataFile, buffer, 2048, index);
  // });
  //response->setContentLength(dataFile.size());
  
  AsyncWebServerResponse *response = request->beginResponse(
          dataType,
          dataFile.size(),
          [dataFile](uint8_t *buffer, size_t maxLen, size_t total) mutable -> size_t {
              int bytes = dataFile.read(buffer, maxLen);
              // close file at the end
              if (bytes + total == dataFile.size()) dataFile.close();
              return max(0, bytes); // return 0 even when no bytes were loaded
          }
  );
  request->send(response);
  // if (server.streamFile(dataFile, dataType) != dataFile.size())
  // {
  //   DBG_OUTPUT_PORT.println("Sent less data than expected!");
  // }

  //dataFile.close();
  return true;
}

//************************************************************************************//
//            Utility Functions                                                       //
//************************************************************************************//

size_t load_data(File f, uint8_t* buffer, size_t maxLen, size_t index) {
  if (f.available()) {
    return f.read(buffer, maxLen);
  }
  else {
    return 0;
  }
}


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

void wakeMyPC() {
    const char *MACAddress = "xx:xx:xx:xx:xx";
  
    WOL.sendMagicPacket(MACAddress); // Send Wake On Lan packet with the above MAC address. Default to port 9.
    // WOL.sendMagicPacket(MACAddress, 7); // Change the port number
}