#include <NewPingESP8266.h>
#include "ESP8266_Start.h"
//NOTE: all serial usage is only for testing, will be removed in code uploaded to car to free up the RX and TX pins
// Pin Name Definitions:

//Sensors
#define VOLTAGE_SENSOR 0
#define CURRENT_SENSOR 0
#define ULTRASONIC_ECHO 
#define ULTRASONIC_TRIG
#define ULTRASONIC_LED

//RGB LED
#define RED
#define BLUE
#define GREEN

/*//L298N
#define IN1
#define IN2
#define IN3
#define IN4
#define EN1
#define EN2*/

//Custom Motor Driver
#define IN1C
#define IN2C
#define IN3C
#define IN4C

//Ultrasonic variables
NewPingESP8266 sonar(ULTRASONIC_TRIG,ULTRASONIC_ECHO ,100); //object which takes the trig and echo pins , and max distance to measure (in cm)
float time;
float distance;
const int speed = 0.034;

//Current sensor variables
float ACS_OUT = 0.0; // Voltage output
float samples = 0.0;
float ACS_OUT_AVG = 0.0; // Voltage output
float  ACS_CURRENT = 0.0; //Actual current value

//Voltage sensor variables
const float R1 = 100.0;
const float R2 = 20.0;
float V_SENSOR = 0.0; //Sensor reading
float V_ACTUAL = 0.0; //Actual voltage reading


//GUI data variables
String dir_cmd_GUI;
char mode_GUI;
float dist_GUI;

//Shift register control byte
byte CTRL = 0;

//millis() variables
unsigned long prevtime_U = 0;
unsigned long prevtime_C = 0;
unsigned long prevtime_M = 0;

void setup() {
  //Starting WiFi functions using our library
  Serial.begin(115200);
  delay(10);
  Serial.println('\n');
  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  startOTA();                  // Start the OTA service
  startMDNS();                 // Start the mDNS responder
  startServer();               // Start a HTTP server with a file read handler and an upload handler
  
  // Define GPIOs
  pinMode(A0,INPUT);

}

void loop() {
  MDNS.update(); // Required for the mDNS server to be detetected by the computer
  server.handleClient();                      // run the server
  ArduinoOTA.handle();                        // listen for OTA events
  sensorRead();
  carDirectionControl(mode_GUI);
  
}

void sensorRead(){
    // ULTRASONIC:
    if(millis()-prevtime_U > 33){
      time = sonar.ping() // Sends a ping and returns the time of receving in microseconds
      speed = 0.034; // speed of sound is 340 m/s or 0.034 cm/us
      distance = speed * time / 2; // we divide by 2 to account for travelling back distance
      Serial.println(distance);
      prevtime_U = millis();
    }
    ////////////////////////// Function to send to GUI

    // Current Sensor:
    if(millis() - prevtime_C > 3){
      for(int i = 0; i < 100; i++ ){ // Get 100 samples of reading, giving 3 millisecond delay between each reading for ADC to settle
        ACS_OUT = analogRead(CURRENT_SENSOR);
        samples +=  ACS_OUT;
        prevtime_C = millis();
        if(i == 99){
          ACS_OUT_AVG = samples / 100.0;
          samples = 0; // Reset samples variable to be used next reading
          ACS_CURRENT = ((ACS_OUT_AVG * 5 / 1024) - 2.5) / 0.185; // convert from digital to analogue volt, subtract the offset(from datasheet) then divide by sensitivity(from datasheet)
        }
      }
    }
    ////////////////////////// Function to send to GUI

    // Voltage Sensor:
    V_SENSOR = analogRead(VOLTAGE_SENSOR);
    V_ACTUAL = (V_SENSOR * 5 / 1024) * R2/(R1+R2);

    ////////////////////////// Function to send to GUI

}

void carDirectionControl(char mode){
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


    case 'A': // Autonomous (PID LATER) ultrasonic will be on right of car
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

void motorControl(String direction,int time){
    if(direction == "LEFT"){
      digitalWrite()
    }

}

/*void turnOFF_G(){           //Generic Turnoff
  digitalWrite(EN1,LOW);
  digitalWrite(EN2,LOW);
}*/

void turnOFF(){           //Turn off all motors
  digitalWrite(INC1,LOW);
  digitalWrite(INC2,LOW);
  digitalWrite(INC3,LOW);
  digitalWrite(INC4,LOW);
}

void onConnectionLoss(){

}
