# IOT_DEVICE_ESP
Simple IOT device with Wifi webserver. Can be imported and built using the PlatformIO 

ESP8266 Wifi webserver. Monitors and logs temperature and humidity with timestamp in the SD Card, using a DHT11 sensor.
Updates a jChart in the web UI with the current temperature/humidity values. On webpage load/refresh, the stored temperature/humidity values
are loaded into the jChart.
The ESP8266 can act as an AP or a Wifi client to a predetermined Wifi router.
The web UI offers some utilities too. Like uploading a file on the SD Card, serving html pages and files from the SD Card, controlling 
the brightness of the onboard led, a simple bidirectional serial to webpage chat, etc.
[Sample pic](img/esp8266.jpg)
