#include <ESP8266WiFi.h>      // WiFi library, contains base functions concerning ESP WiFi
#include <ESP8266WiFiMulti.h> // Library that allows ESP to connect to strongest of chosen WiFi connections at any given time
#include <ArduinoOTA.h>       // Over The Air. Allows ESP to be programmed through WiFi after first programming
#include <ESP8266WebServer.h> // Better ESP8266 server library
#include <ESP8266mDNS.h>      // mDNS library
#include <WebSocketsServer.h> // Low latency and TCP secure

//NOTE: all serial usage is only for testing, will be removed in code uploaded to car to free up the RX and TX pins

ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

const char *ssid = "Aquaphoton CAR"; // The name of the Wi-Fi network that will be created
const char *password = "TOP1";   // The password required to connect to it, leave blank for an open network

const char *OTAName = "ESP8266_Team4";           // A name and a password for the OTA service
const char *OTAPassword = "esp8266_team4";

const char* mdnsName = "team4"; // Domain name for the mDNS responder

#define VOLTAGE_SENSOR 
#define CURRENT_SENSOR 
#define ULTRASONIC_ECHO 
#define ULTRASONIC_TRIG
#define MUX_A 
#define MUX_B 
#define RGB_LED
#define DISTNACE_LED

//L298N
#define IN1
#define IN2
#define IN3
#define IN4
#define EN1
#define EN2

//Custom Motor Driver

float time;
float distance;
const int* speed = 0.034;
String dir_cmd_GUI;
char mode_GUI;
float dist_GUI;

void setup() {
  // DEFINE pinMode of each pin
  pinMode(,);

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  
  startOTA();                  // Start the OTA service

  startWebSocket();            // Start a WebSocket server
  
  startMDNS();                 // Start the mDNS responder

  startServer();               // Start a HTTP server with a file read handler and an upload handler

}

void loop() {
  MDNS.update(); // Required for the mDNS server to be detetected by the computer
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server
  ArduinoOTA.handle();                        // listen for OTA events
  sensorRead();
  carDirectionControl(mode_GUI);
  
}

  


void sensorRead(){
    // ULTRASONIC:
    digitalWrite(ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10); // Wait for 10 microseconds to trigger a pulse
    digitalWrite(ULTRASONIC_TRIG, LOW);
    delayMicroseconds(5);
    time = pulseIn(ULTRASONIC_ECHO,HIGH); //Reads time pin 0 is high in microseconds
    speed = 0.034; // speed of sound is 340 m/s or 0.034 cm/us
    distance = speed * time / 2; // we divide by 2 to account for travelling back distance
    Serial.println(distance);
    ////////////////////////// Function to send to GUI

    // Current Sensor:
    analogRead(CURRENT_SENSOR);
    ////////////////////////// Function to send to GUI

    // Voltage Sensor:
    analogRead(VOLTAGE_SENSOR);
    ////////////////////////// Function to send to GUI

}

void carDirectionControl(mode){
  switch(mode){
    case 'M': //Generic motor driver - Manual
      // Switch on the direction incoming from GUI
      switch(dir_cmd_GUI){
        case "LEFT":
        motorControl()
        turnOFF();
        break;

        case "RIGHT":
        break;

        case "DOWN":
        break;

        case "UP":
        break;

        default
        break;
      }
      break;


    case 'A': //Generic motor driver - Autonomous (PID LATER) ultrasonic will be on right of car
      if(distance < dist_GUI){ // too close to wall, need to go left
        motorControl('R',"LEFT",200)
      }
      else{
        motorControl('L',"RIGHT",200)
      }
    break;

    default: //Add a default state later
    break;
  }
}

void motorControl(char motor,String direction,int time){
  switch(motor){
    case 'L':
    if(direction )
  }

}

void turnOFF(){
  digitalWrite(EN1,LOW);
  digitalWrite(EN2,LOW);
}


void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started\r\n");

  wifiMulti.addAP("Home_ssid", "Home_password");   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("Data_ssid", "Data_password");

  Serial.println("Connecting");
  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
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


void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void startServer() {



}
