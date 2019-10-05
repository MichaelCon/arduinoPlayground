// Smoke Cannon controller
#define MOSFET_PIN 3    // Needs to be a PWM pin for analog write
#define REVERSE_PIN 4   // Direction of the drill.  Controls 2 relays
#define READ_LEVER 5
#define FOG_PIN 6
#define LIGHTNING_PIN 7
#define SKELETON_SERVO 8
#define LATCH_SERVO 9

#define FORWARD_SPEED 255

#define SERVO_BOTTOM 10
#define SERVO_TOP 90

#define LATCH_MOVE_DELAY 500
#define REARM_DELAY 300
#define UNWIND_TIMER 1050
#define MAX_MOTOR_TIME 3000
#define FOG_DURATION 2000

// States of the mahcine
#define STARTUP 0
#define FIRED 1
#define LOADING 2
#define ARMED 3
#define FIRING 4
#define ARM_AWAY 5

String stDesc[] = { "Startup", "FIRED", "Loading", "Armed", "Firing", "Arm away"};

#include <Servo.h> 
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xEE, 0xEF, 0xFE, 0xEF }; //physical mac address
byte ip[] = { 192, 168, 1, 240 }; // arduino server ip in lan
EthernetServer server(2148); //arduino server port

Servo latchServo;  // create servo object to control a servo 
Servo skeletonServo;
int state = STARTUP;
int drillSpeed = 255;

// Timers allow the main thread to continue
long fogTimer = 0;
long drillTimer = 0;
long armTimer = 0;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // initialize digital pin 13 as an output.
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW);    // turn the drill off

  pinMode(REVERSE_PIN, OUTPUT);
  digitalWrite(REVERSE_PIN, HIGH);    // turn the drill forward to start
  pinMode(FOG_PIN, OUTPUT);
  digitalWrite(FOG_PIN, HIGH);    // fog off
  pinMode(LIGHTNING_PIN, OUTPUT);
  digitalWrite(LIGHTNING_PIN, HIGH);    // lightning off

  pinMode(READ_LEVER, INPUT);

  skeletonServo.attach(SKELETON_SERVO);
  latchServo.attach(LATCH_SERVO);  // attaches the servo on a pin to the servo object 
  openLatch();       

  // Network start
  Ethernet.begin(mac, ip);
  server.begin();
}

// the loop function runs over and over again forever
void loop() {
  doRead();       // Check for serial request
  checkWebRequests(); // Web
  checkState();   // Check for state change, mostly the arm connectivity
  checkTimers();  // Check for expired timers
  delay(10); 
}

void doRead() {
  if(Serial.available() > 0) {
    readCommand();
  }
}

void checkWebRequests() {
  // Create a client connection
  EthernetClient client = server.available();
  if (client) {
    String readString;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //read char by char HTTP request
        if (readString.length() < 100) {
          //store characters to string 
          readString += c; 
        } 

        //if this is the end of a single line
        if (c == '\n') {
          delay(1);

          // The loop ignore all string that are not a get request
          if(readString.indexOf("GET") == 0) {
            int commandLocation = readString.indexOf("command=");
            if(commandLocation > 0) {
              int command = readString.charAt(commandLocation + 8);
              doCommand(command);
            }
            htmlResponse(client);
            //stopping client
            client.stop();
          }
          //clearing string for next read
          readString="";
        }
      }
    }
  }
}

void htmlResponse(EthernetClient client) {
    //now output HTML data header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println("<HTML>");
    client.println("<HEAD>");
    client.println("<TITLE>Controller</TITLE>");
    client.println("</HEAD>");
    client.println("<BODY>");
    client.println("<H1>BOOM!</H1>");
    client.println("<a href=\"a\">Refresh</a><br/>");
    client.println("<a href=\"a?command=x\">FIRE</a><br/>");
    client.println("<a href=\"a?command=z\">SMOKE</a><br/>");
    client.println("<a href=\"a?command=o\">Open Latch</a><br/>");
    client.println("<a href=\"a?command=O\">Close Latch</a><br/>");
    if(state == STARTUP) {
      client.println("<a href=\"a?command=g\">Start Loop (Arm should be loaded)</a><br/>");
    }
    client.println("Current State : ");
    client.println(stDesc[state]);
    client.println("<br/>");
    client.println("Current Arm PIN : ");
    client.println(digitalRead(READ_LEVER));
    client.println("</BODY>");
    client.println("</HTML>");
}

void checkTimers() {
  if(drillTimer > 0 && drillTimer < millis()) {
    Serial.println("Drill timer up!");
    stopArmMotor();
  }
  if(armTimer > 0 && armTimer < millis()) {
    state = FIRED;
    Serial.println("State = FIRED");
    armTimer = 0;
  }
  if(fogTimer > 0 && fogTimer < millis()) {
    fogTimer = 0;
    digitalWrite(FOG_PIN, HIGH);
  }
}

/** Read a character from serial and react */
void readCommand() {
  // Read the command 
  int command = Serial.read();
  doCommand(command);
}

void doCommand(int command) {
  
  if(command == 'p') {    // Pull the arm back
    Serial.println("Pull arm ");
    Serial.println(drillSpeed);
    startArmMotor(true);
  }
  // More drill speed
  if(command == 'm') { 
    drillSpeed = drillSpeed + 5;
    if(drillSpeed > 255)
      drillSpeed = 255;
    Serial.println("Increased drill speed to ");
    Serial.println(drillSpeed);
  }
  // Less drill speed
  if(command == 'l') { 
    drillSpeed = drillSpeed - 5;
    Serial.println("Decreased drill speed to ");
    Serial.println(drillSpeed);
  }
  if(command == 'F') { 
      digitalWrite(REVERSE_PIN, LOW);
  }
  if(command == 'f') {
      digitalWrite(REVERSE_PIN, HIGH);
  }
  if(command == 'L') {
      digitalWrite(LIGHTNING_PIN, LOW);
  }
  if(command == 'l') {
      digitalWrite(LIGHTNING_PIN, HIGH);
  }
  if(command == 'o') {
      openLatch();
  }
  if(command == 'O') {
      closeLatch();
  }
  if(command == 'g') {    // g for GO, it puts the state to firing
      state = ARMED;
  }
  if(command == 'x') {
    fire();
  }
  if(command == 'a') {
    skeletonServo.write(0);
  }
  if(command == 'b') {
    skeletonServo.write(90);
  }
  if(command == 'c') {
    skeletonServo.write(160);
  }
  if(command == 'z') {
    fog(FOG_DURATION);
  }
}

// This function checks the lever arm to see if it has released in firing or returned
void checkState() {
  if(state == FIRING) {  // check to see if we finished openning the latch
    if(digitalRead(READ_LEVER) == HIGH) {   // if the arm isn't still back
      // we just disconnected so wait before activating the motor
      if(armTimer == 0)
        armTimer = millis() + REARM_DELAY;
    }
  }
  if(state == FIRED) {
    startArmMotor(true);
    state = LOADING;
    Serial.println("State = LOADING");
  }
  if(state == LOADING) {
    if(digitalRead(READ_LEVER) == LOW) {
      stopArmMotor();
      Serial.println("Closing latch");
      closeLatch();
      delay(LATCH_MOVE_DELAY); // time for latch to close
      Serial.println("Unwind");
      startArmMotor(false);
      delay(UNWIND_TIMER); // need to determine the exact right delay here!!
      stopArmMotor();
      state = ARMED;
      Serial.println("State = ARMED");
    }
  }
}

void closeLatch() {
  latchServo.write(SERVO_BOTTOM);
}

void openLatch() {
  latchServo.write(SERVO_TOP);
}

void startArmMotor(boolean forward) {
  drillTimer = millis() + MAX_MOTOR_TIME;
  if(forward) {
    Serial.println("Starting motor forward");
    digitalWrite(REVERSE_PIN, HIGH);
    analogWrite(MOSFET_PIN, drillSpeed);  // Start the drill
  } else {
    Serial.println("Starting motor back");
    digitalWrite(REVERSE_PIN, LOW);
    analogWrite(MOSFET_PIN, drillSpeed);  // Start the drill
  }
}

void stopArmMotor() {
  Serial.println("Stopping motor");
  drillTimer = 0;
  analogWrite(MOSFET_PIN, LOW);
}

void fire() {
  if(state == ARMED) {
      Serial.println("FIRE!");
      openLatch();
      state = FIRING;
  } else {
    Serial.println("Not armed");
  }
}

void fog(long duration) {
  Serial.println("Start fog");
  digitalWrite(FOG_PIN, LOW);
  fogTimer = millis() + duration;
}
