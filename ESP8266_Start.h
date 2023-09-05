#include <ESP8266WiFi.h>      // WiFi library, contains base functions concerning ESP WiFi
#include <ESP8266WiFiMulti.h> // Library that allows ESP to connect to strongest of chosen WiFi connections at any given time
#include <ArduinoOTA.h>       // Over The Air. Allows ESP to be programmed through WiFi after first programming
#include <ESP8266mDNS.h>      // mDNS library

//NOTE: all serial usage is only for testing, will be removed in code uploaded to car to free up the RX and TX pins

ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

const char *ssid = "Aquaphoton CAR"; // The name of the AP that will be created
const char *password = "TOP1";   // The password required to connect to it
const IPAddress apIP = IPAddress(192, 168, 4, 1); // ESP8266 IP 

const char *OTAName = "ESP8266_Team4";           // A name and a password for the OTA service
const char *OTAPassword = "esp8266_team4";
const char* mdnsName = "team4"; // Domain name for the mDNS responder

WiFiServer server(80);    // Create a webserver object that listens for HTTP request on port 80
WiFiClient client;

void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.mode(WIFI_AP_STA); //Both AP and Station modes ON
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // Configure AP
  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started\r\n");

  wifiMulti.addAP("Home_ssid", "Home_password");   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("Data_ssid", "Data_password");

  Serial.println("Connecting");
  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect or for a station to connect to created access point
    delay(250);
    Serial.print('.');
  }
  Serial.println("\r\n");
  if(WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    Serial.print("Station connected to ESP8266 AP");
  }
  Serial.println("\r\n");
}


void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}


void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void startServer() {
  server.begin();
  Serial.println("TCP server started");
}
