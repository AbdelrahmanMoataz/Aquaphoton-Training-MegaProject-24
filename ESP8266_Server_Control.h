#ifndef ESP8266_Server_Control // Header guard
#define ESP8266_Server_Control
// Libraries
#include <ESP8266WiFi.h>      // WiFi library, contains base functions concerning ESP WiFi
#include <ESP8266WiFiMulti.h> // Library that allows ESP to connect to strongest of chosen WiFi connections at any given time
#include <ArduinoOTA.h>       // Over The Air. Allows ESP to be programmed through WiFi after first programming
#include <ESP8266mDNS.h>      // mDNS library

// ################################################################ STARTING WIFI SERVICES #############################################################################
// Soft AP constants
const char *ssid = "Aquaphoton CAR"; // The name of the AP that will be created
const char *password = "TOP1";   // The password required to connect to it
const IPAddress apIP = IPAddress(192, 168, 4, 1); // ESP8266 IP 

// OTA constants
const char *OTAName = "ESP8266_Team4";           // A name and a password for the OTA service
const char *OTAPassword = "esp8266_team4";

// mDNS constant
const char* mdnsName = "team4"; // Domain name for the mDNS responder

// WiFi objects
ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
WiFiServer server(80);    // Create a webserver object that listens for HTTP request on port 80
WiFiClient client;

void startServer(){
// START WIFI
// Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.mode(WIFI_AP_STA); //Both AP and Station modes ON
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // Configure AP
  WiFi.softAP(ssid, password);             // Start the access point

  wifiMulti.addAP("Home_ssid", "Home_password");   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("Data_ssid", "Data_password");

  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect or for a station to connect to created access point
    delay(250);
  }

// START OTA 
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);
  ArduinoOTA.begin();

// START MDNS RESPONDER
  MDNS.begin(mdnsName); // start the multicast domain name server
  MDNS.addService("http", "tcp", 80); // Add service to MDNS-SD   

// START SERVER
  server.begin(); 
}
// ################################################################ END #############################################################################


// ################################################################ SERVER CONTROL FUNCTIONS #############################################################################
void handleOTA(){
  if(WiFi.status() == WL_CONNECTED)
    ArduinoOTA.handle();                        // listen for OTA events only if connected to WiFi
}

// Request Handling Functions
String REQUEST;
String SEND;
String PATH;
bool COMMAND = false;
void takeRequest(){ // TEST if the while loop here will stop the code
  MDNS.update(); // Required for the mDNS server to be detetected by the computer
  client = server.accept();
  if (!client) { return; } // return early from function 
  client.setNoDelay(true);
  // Wait for data from client to become available
  while (client.connected() && !client.available()) { delay(1); }
  // Read the first line of HTTP request
  REQUEST = client.readStringUntil('\r');
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int request_start = REQUEST.indexOf(' ');
  int request_end = REQUEST.indexOf(' ', request_start + 1);
  if (request_start == -1 || request_end == -1) { //Invalid request, no spaces were found, return to
    return;
  }
  PATH = REQUEST.substring(request_start + 1, request_end); // Will be used to receive commands from GUI
  if(PATH == "/") 
    COMMAND = false;
  else 
    COMMAND = true;
  client.flush(); // Discard any bytes that have been written to the client but not yet read
}

void sendData(String data_sent){
  //HTTP Protocol code.
  SEND.reserve(100);
  SEND = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  SEND += data_sent; //Our final raw data to return
  client.print(SEND);
}

String receiveCommand(){
  if(COMMAND){  
    int command_start = PATH.indexOf('/');
    int command_end = PATH.indexOf('/', command_start + 1);
    int data_end = PATH.indexOf('/',command_end + 1);
    String command = PATH.substring(command_start+1,command_end);
    String data = PATH.substring(command_end+1,data_end); // discard the '/' at start and end
    if(command == "DIRECTION") command = "d";
    if(command == "MODE") command = "M";
    if(command == "SPEED") command = "S";
    if(command == "DISTANCE") command = "D";
    return command + data; // First letter will be the command, the rest will be data corresponding to command
  }
  else 
    return "";
}


// Connection Functions
bool onConnectionLoss(){    // If the computer loses its connection to the ESP
  if(WiFi.softAPgetStationNum() < 1 && wifiMulti.run() != WL_CONNECTED)
    return true;
  else
    return false;
}
void waitForConnection(){
  while (WiFi.status() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1)  // Wait for the Wi-Fi to connect or for a station to connect to created access point
      delay(250);   
}

#endif