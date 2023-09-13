#include <NewPingESP8266.h>
#include "ESP8266_Server_Control.h"
//NOTE: all serial usage is only for testing, will be removed in code uploaded to car to free up the RX and TX pins

// #################################################################### VARIABLES AND DEFINITIONS ####################################################################

// Pin Name Definitions (+ Shift Register Definitions):

//Sensors
#define VOLTAGE_SENSOR A0
#define CURRENT_SENSOR A0
#define VOLTAGE_SWITCH 3  // PNP transistor electronic switch
#define CURRENT_SWITCH 1  // PNP transistor electronic switch
#define ULTRASONIC_ECHO 16
#define ULTRASONIC_TRIG 15
#define ULTRASONIC_LED 0 // QA LSB of shift register
//RGB LED
#define RED 1 // QB
#define BLUE 3 // QD
#define GREEN 2 // QC
//L293D
#define IN1 7 // QH MSB of shift register
#define IN2 6 // QG
#define IN3 5 // QF
#define IN4 4 // QE
#define EN1 13
#define EN2 14
//Shift Register
#define LATCH_SR  4                         // Shift register latch pin
#define CLK_SR 5                            // Shift register clock pin
#define DATA_SR 12                          // Shift register data pin


// Variable Names:

//Ultrasonic variables
NewPingESP8266 sonar(ULTRASONIC_TRIG,ULTRASONIC_ECHO ,100); //object which takes the trig and echo pins , and max distance to measure (in cm)
float time_U;
float distance_U;
const float speed = 0.034; // Speed of sound is 340 m/s or 0.034 cm/us

//Current sensor variables
float ACS_OUT = 0.0; // Voltage output (digital)
float samples = 0.0;
float ACS_OUT_AVG = 0.0; // Voltage output (digital)
float  ACS_CURRENT = 0.0; // Actual current value (analog)

//Voltage sensor variables
const float R1 = 100.0;
const float R2 = 20.0;
float V_SENSOR = 0.0; // Sensor reading
float V_ACTUAL = 0.0; // Actual voltage reading

//Sensor control boolean
bool SENSOR_TURN = true; // Starts at current sensor by default

//GUI data variables
String sensor_send;
String dir_cmd_GUI;
char mode_GUI;
float dist_GUI;
int motor_speed_GUI = 255; //Default is at max speed

//Shift register control byte
byte CTRL_SR = 0;

//Motor variables
bool turnoff = true; // Car is at rest at start

//millis() variables
unsigned long SENSOR_TIME = 0;
unsigned long prevtime_S = 0;  // Send to server
unsigned long prevtime_U = 0;  // Ultrasonic sensor
unsigned long prevtime_C = 0;  // Current sensor
unsigned long prevtime_M = 0;  // Motor

// #################################################################### SETUP AND LOOP ########################################################################

void setup() {
  // Define GPIOs
  pinMode(A0,INPUT);
  pinMode(16,INPUT);
  pinMode(1,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(4,OUTPUT);
  pinMode(5,OUTPUT);
  pinMode(12,OUTPUT);
  pinMode(13,OUTPUT);
  pinMode(14,OUTPUT);
  pinMode(15,OUTPUT);

  // Initial States
  digitalWrite(CURRENT_SENSOR,LOW); // PNP transistors are used which are turned on at LOW signal to base. Start with current sensor at base
  digitalWrite(VOLTAGE_SENSOR,HIGH);
  CTRL_SR = 0b00001110; // Motor IN pins, RGB LEDs, Ultrasonic indicator all turned off 
                        // Note: RGB LED used is common anode. The microcontroller is connected to the each diode's cathode and so they are active LOW
  updateShiftRegister();

  //Starting WiFi functions using our library
  startServer();               // Startup all WiFi services
}

void loop() {
  // Activate OTA if connected to WiFi
  handleOTA();
  // Receive HTTP request 
  takeRequest();
  // Read then send sensor data to server
  sensorRead();
  // After receiving HTTP request, check if it is a command and update GUI variables
  takeAction(receiveCommand());
  // Control RGB LED
  RGBLED();
  // Control motion of car
  carDirectionControl(mode_GUI);
  // Always check if connection is lost, wait for reconnection
  if(onConnectionLoss()){
    turnOFF();
    RGBLED();
    waitForConnection(); // Stay in loop until car reconnects
  };
}


// #################################################################### FUNCTION DEFINITIONS #######################################################################

// Shift Register Function
void updateShiftRegister(){
   digitalWrite(LATCH_SR, LOW);
   shiftOut(DATA_SR, CLK_SR, MSBFIRST, CTRL_SR);  //MSBFIRST used so byte in CLK is same as the byte "CTRL_SR" and not reversed
   digitalWrite(LATCH_SR, HIGH);
}

// Sensor read and send function
int i = 0; // Counter for current sensor
void sensorRead(){
    // Ultrasonic Sensor:
    if(millis()-prevtime_U > 33){
      time_U = sonar.ping(); // Sends a ping and returns the time of signal reception in microseconds
      distance_U = speed * time_U / 2.0; // we divide by 2 to account for travelling back distance
      prevtime_U = millis();
    }

    // Current and voltage sensor switching:
    if(SENSOR_TURN){
      sensor_send.reserve(30);// reserve 30 bytes for sensor_send string
    // Current Sensor:
      if(millis() - prevtime_C > 3){
        ACS_OUT = analogRead(CURRENT_SENSOR);
        samples +=  ACS_OUT;
        prevtime_C = millis();
        i++;
        if(i == 100){
          i = 0;
          ACS_OUT_AVG = samples / 100.0;
          samples = 0; // Reset samples variable to be used next reading
          ACS_CURRENT = ((ACS_OUT_AVG  * 3.3 / 1024) - 2.5 + 0.7 ) / 0.185 + 0.02; // convert from digital to analogue volt (3.3V is max of ESP), add the diode voltage drop,
                                                                                   // subtract the offset(from datasheet) then divide by sensitivity(from datasheet)
                                                                                   // Finally, add the 10mA going to 12V indicator LED and 10mA consumed by ACS712
        }
      }
    }
    else{
      // Voltage Sensor:
      V_SENSOR = analogRead(VOLTAGE_SENSOR);
      V_ACTUAL = (V_SENSOR * 3.3 / 1024) * (R1+R2)/R2 + 0.7; // (R1+R2)/R2 = 5 , add the diode voltage drop
    }

    // Sensor switching code
    if(millis()-SENSOR_TIME > 1000){ // Every 1 second, switch sensor being read
      SENSOR_TURN = !SENSOR_TURN;
      digitalWrite(CURRENT_SWITCH,!digitalRead(CURRENT_SWITCH)); // switch the sensor being activated
      digitalWrite(VOLTAGE_SWITCH,!digitalRead(VOLTAGE_SWITCH));
      SENSOR_TIME = millis();
    } 

    // Sending data to GUI every 50ms
    if(millis() - prevtime_S > 50){
      sensor_send = String(distance_U) + "." + String(ACS_CURRENT) + "." + String(V_ACTUAL);
      sendData(sensor_send);
      prevtime_S = millis();
    }
    
}

// Receive data from GUI function
void takeAction(String command){
  switch(command[0]){
      case 'd':
      dir_cmd_GUI = command.substring(1);
      break;
      
      case 'S':
      motor_speed_GUI = command.substring(1).toInt();
      break;

      case 'M':
      mode_GUI = command.charAt(1);
      break;

      case 'D':
      dist_GUI = command.substring(1).toFloat();
      break;

      default:
      break;
    }
}

// RGB LED control
void RGBLED(){
  if(motor_speed_GUI >= 170){
    bitClear(CTRL_SR,RED);
    bitSet(CTRL_SR,BLUE);
    bitSet(CTRL_SR,GREEN);
  } 
  else if(motor_speed_GUI >= 85 ){
    bitSet(CTRL_SR,RED);
    bitSet(CTRL_SR,BLUE);
    bitClear(CTRL_SR,GREEN);
  } 
  else{
    bitSet(CTRL_SR,RED);
    bitClear(CTRL_SR,BLUE);
    bitSet(CTRL_SR,GREEN);
  }

  if(motor_speed_GUI == 0 || turnoff){ // Notice that this if condition is put after the previous three so that RGB LED is not blue while car is stopped.
    bitSet(CTRL_SR,RED);
    bitSet(CTRL_SR,BLUE);
    bitSet(CTRL_SR,GREEN);
  }
  updateShiftRegister(); // Updates the shift register output after changing the control byte
}

// Motor control functions

void turnON(){
  turnoff = false; // Related to RGB LED
  analogWrite(EN1,motor_speed_GUI);
  analogWrite(EN2,motor_speed_GUI);
}

void turnOFF(){  // Turnoff, only write do enable pins for free motor stopping instead of fast motor stopping
  turnoff = true; // Related to RGB LED
  digitalWrite(EN1,LOW);
  digitalWrite(EN2,LOW);
}

void motorControl(String direction,unsigned int time){
    if(direction == "FORWARD"){
      prevtime_M = millis();
      turnON();
      bitSet(CTRL_SR,IN1);
      bitClear(CTRL_SR,IN2);
      bitSet(CTRL_SR,IN3);
      bitClear(CTRL_SR,IN4);
    }

    if(direction == "BACKWARD"){
      prevtime_M = millis();
      turnON();
      bitClear(CTRL_SR,IN1);
      bitSet(CTRL_SR,IN2);
      bitClear(CTRL_SR,IN3);
      bitSet(CTRL_SR,IN4);
    }

    if(direction == "LEFT"){
      prevtime_M = millis();
      turnON();
      bitClear(CTRL_SR,IN1);
      bitClear(CTRL_SR,IN2);
      bitSet(CTRL_SR,IN3);
      bitClear(CTRL_SR,IN4);
    }

    if(direction == "RIGHT"){
      prevtime_M = millis();
      turnON();
      bitSet(CTRL_SR,IN1);
      bitClear(CTRL_SR,IN2);
      bitClear(CTRL_SR,IN3);
      bitClear(CTRL_SR,IN4);
    }

    dir_cmd_GUI = "" // Reset the direction after executing function once

    if(millis()-prevtime_M > time)  //If no inputs were received within "time", the car stops
      turnOFF();
    updateShiftRegister(); // Updates the shift register output after changing the control byte
}


void carDirectionControl(char mode){
  switch(mode){
    case 'M': // Manual driving, controlled by GUI
      motorControl(dir_cmd_GUI,3000);
      break;

    case 'A': // Autonomous (PID LATER) (ultrasonic will be on right side of car)
      if((distance_U > dist_GUI * 0.95) && (distance_U < dist_GUI * 1.05)) //5% tolerance for distance to be detected as correct
        bitSet(CTRL_SR,ULTRASONIC_LED);
      else
        bitClear(CTRL_SR,ULTRASONIC_LED);
      updateShiftRegister();

      if(distance_U < dist_GUI) // Too close to wall, need to go left
        motorControl("LEFT",200);
      else // Otherwise go right
        motorControl("RIGHT",200);
    break;

    default:  //default mode is manual
    mode_GUI = 'M';
    break;
  }
}

