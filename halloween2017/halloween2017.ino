// Vampire in the coffin
// Control for motor to pull vampire down, motor on coffin lid, fog machine, and latch servo to release vampire
// Analog pins read the button states on lid and when vampire reached bottom
// Walk through the pins
#define SERIAL_IN 0
#define SERIAL_OUT 1

#define NEGATIVE_LID 2   // Other side
#define POSITIVE_MOTOR 3    // Two relays control the motor
#define SD_CARD_BLOCK 4
#define POWER_LEVEL 5
#define POSITIVE_LID 6    // Two relays control the motor
#define FOG_PIN 7
#define NEGATIVE_MOTOR 8   // Other side
#define LATCH_SERVO 9

// Pin 10-13 are used by ethernet shield

#define LID_CLOSED_PIN A5
#define LID_OPEN_PIN A3
#define READ_LEVER A2

#define LID_TIMER 500

#define SERVO_BOTTOM 40
#define SERVO_TOP 115

#define LATCH_MOVE_DELAY 500
#define REARM_DELAY 4000  // How long to wait before pulling vampire back down
#define UNWIND_TIMER 15350 // How long to unwind the pull string
#define MAX_MOTOR_TIME 45000  // Max time to pull down vampire
#define FOG_DURATION 3500 // Amount of fog
#define OPEN_TO_JUMP 2000 // Delay between open lid and pop up

// States of the machine
#define STARTUP 0
#define FIRED 1
#define LOADING 2
#define ARMED 3
#define FIRING 4
#define ARM_AWAY 5

int unwindTime = UNWIND_TIMER;
int lidTime = LID_TIMER;
int openSpeed = 120;
int closeSpeed = 100;
int pullSpeed = 120;
int unwindSpeed = 65;

String stDesc[] = { "Startup", "FIRED", "Loading", "Armed", "Firing", "Arm away"};

#include <Servo.h> 
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xEE, 0xEF, 0xFE, 0xEF }; //physical mac address
byte ip[] = { 192, 168, 1, 20 }; // arduino server ip in lan
EthernetServer server(2148); //arduino server port

Servo latchServo;  // create servo object to control a servo 
Servo skeletonServo;
int state = ARMED;

// Timers allow the main thread to continue
long fogTimer = 0;
long drillTimer = 0;
long armTimer = 0;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // initialize digital pins.
  pinMode(POSITIVE_MOTOR, OUTPUT);
  digitalWrite(POSITIVE_MOTOR, HIGH);    // turn the drill off
  pinMode(NEGATIVE_MOTOR, OUTPUT);
  digitalWrite(NEGATIVE_MOTOR, HIGH);    // turn the drill off

  pinMode(FOG_PIN, OUTPUT);
  digitalWrite(FOG_PIN, HIGH);    // fog off
//  pinMode(LIGHTNING_PIN, OUTPUT);
//  digitalWrite(LIGHTNING_PIN, HIGH);    // lightning off

  pinMode(POWER_LEVEL, OUTPUT);
  analogWrite(POWER_LEVEL, 60);
  
  pinMode(READ_LEVER, INPUT);
  pinMode(LID_CLOSED_PIN, INPUT);
  pinMode(LID_OPEN_PIN, INPUT);

  pinMode(POSITIVE_LID, OUTPUT);

  digitalWrite(POSITIVE_LID, HIGH);    // turn the drill off
  pinMode(NEGATIVE_LID, OUTPUT);
  digitalWrite(NEGATIVE_LID, HIGH);    // turn the drill off

// disable SD card
  pinMode(SD_CARD_BLOCK, OUTPUT);
  digitalWrite(SD_CARD_BLOCK, HIGH);

  latchServo.attach(LATCH_SERVO);  // attaches the servo on a pin to the servo object 
  closeLatch();       

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

void debug(String s) {
  Serial.println(s);
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
    //debug("New client");
    //long start = millis();
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
          //debug(readString);
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
            //debug("closed client");
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
    client.println("<a href=\"a?command=x\">Release Vampire</a><br/>");
    client.println("<a href=\"a?command=z\">SMOKE</a><br/>");
    client.println("<a href=\"a?command=o\">Open Latch</a><br/>");
    client.println("<a href=\"a?command=O\">Close Latch</a><br/>");
    client.println("<a href=\"a?command=u\">Open Coffin</a><br/>");
    client.println("<a href=\"a?command=w\">Close Coffin</a><br/>");
    if(digitalRead(LID_OPEN_PIN) == LOW) {
      client.println("LID_OPEN_PIN LOW");
    } else {
      client.println("LID_OPEN_PIN HIGH");
    }
    
    if(digitalRead(LID_CLOSED_PIN) == LOW) {
      client.println("LID_CLOSED_PIN LOW");
    } else {
      client.println("LID_CLOSED_PIN HIGH");
    }
    
    if(digitalRead(READ_LEVER) == LOW) {
      client.println("READ_LEVER LOW");
    } else {
      client.println("READ_LEVER HIGH");
    }
    client.println("<br/>");
    //client.println("Current Arm PIN : ");
    //client.println(digitalRead(READ_LEVER));
    client.println("</BODY>");
    client.println("</HTML>");
}

void checkTimers() {
  if(drillTimer > 0 && drillTimer < millis()) {
    debug("Drill timer up!");
    stopArmMotor();
  }
  if(armTimer > 0 && armTimer < millis()) {
    state = FIRED;
    debug("State = FIRED");
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
  if(command == 'q') {
      startArmMotor(true);
      delay(500);
      stopArmMotor();
  }
  if(command == 'a') {
      startArmMotor(false);
      delay(500);
      stopArmMotor();
  }
  if(command == '1') {
      openSpeed = openSpeed - 1;
      Serial.println(openSpeed);
  }
  if(command == '2') {
      openSpeed = openSpeed + 1;
      Serial.println(openSpeed);
  }
  if(command == '3') {
      closeSpeed = closeSpeed - 1;
      Serial.println(closeSpeed);
  }
  if(command == '4') {
      closeSpeed = closeSpeed + 1;
      Serial.println(closeSpeed);
  }
  if(command == '5') {
      pullSpeed = pullSpeed - 1;
      Serial.println(pullSpeed);
  }
  if(command == '6') {
      pullSpeed = pullSpeed + 1;
      Serial.println(pullSpeed);
  }
  if(command == '7') {
      unwindSpeed = unwindSpeed - 1;
      Serial.println(unwindSpeed);
  }
  if(command == '8') {
      unwindSpeed = unwindSpeed + 1;
      Serial.println(unwindSpeed);
  }
  if(command == '9') {
      unwindTime = unwindTime - 100;
      Serial.println(unwindTime);
  }
  if(command == '0') {
      unwindTime = unwindTime + 100;
      Serial.println(unwindTime);
  }
  if(command == 'o') {
      openLatch();
  }
  if(command == 'O') {
      closeLatch();
  }
  if(command == 'u') {
      openLid();
  }
  if(command == 'w') {
      closeLid();
  }
  if(command == 'g') {    // g for GO, it puts the state to firing
      state = ARMED;
  }
  if(command == 'x') {
    fire();
  }
  if(command == 'e') {
    debug("Echo");
    debug((String) digitalRead(LID_OPEN_PIN));
  }
  if(command == 'z') {
    fog(FOG_DURATION);
  }
  if(command == 'v') {
    vampire();
  }
}

// This function checks the lever arm to see if it has released in firing or returned
void checkState() {
  if(state == FIRING) {  
    // check to see if we finished openning the latch
    if(digitalRead(READ_LEVER) == LOW) {   // if the arm isn't still back
      // we just disconnected so wait before activating the motor
      if(armTimer == 0)
        armTimer = millis() + REARM_DELAY;
    }
  }
  if(state == FIRED) {      // The vampire was released and timer passed, so start pulling him down
    startArmMotor(true);
    state = LOADING;
    debug("State = LOADING");
  }
  if(state == LOADING) {    // Motor is pulling down so check to see if it's all the way down
    if(digitalRead(READ_LEVER) == HIGH) {
      stopArmMotor();
      debug("Closing latch");
      closeLatch();
      delay(LATCH_MOVE_DELAY); // time for latch to close
      closeLid();
      debug("Unwind");
      startArmMotor(false);
      delay(unwindTime); // need to determine the exact right delay here!!
      stopArmMotor();
      state = ARMED;
      debug("State = ARMED");
    }
  }
}

void closeLatch() {
  latchServo.write(SERVO_TOP);
}

void openLatch() {
  latchServo.write(SERVO_BOTTOM);
}

void startArmMotor(boolean forward) {
  drillTimer = millis() + MAX_MOTOR_TIME;
  if(forward) {
    debug("Starting motor forward");
    analogWrite(POWER_LEVEL, pullSpeed);
    digitalWrite(POSITIVE_MOTOR, LOW);
  } else {
    debug("Starting motor back");
    analogWrite(POWER_LEVEL, unwindSpeed);
    digitalWrite(NEGATIVE_MOTOR, LOW);
  }
}

void stopArmMotor() {
  debug("Stopping motor");
  drillTimer = 0;
  digitalWrite(POSITIVE_MOTOR, HIGH);
  digitalWrite(NEGATIVE_MOTOR, HIGH);
}

void fire() {
  if(state == ARMED) {
      debug("FIRE!");
      openLatch();
      state = FIRING;
  } else {
    debug("Not armed");
  }
}

// Open the coffin lid
void openLid() {
  debug("Open lid");
  analogWrite(POWER_LEVEL, openSpeed);
  digitalWrite(NEGATIVE_LID, LOW);
  int x;
  long t1 = millis();
  x = 0;
  while(digitalRead(LID_OPEN_PIN) == LOW && x < 2000) {
    delay(2);
    x = x + 1;
  }
  digitalWrite(NEGATIVE_LID, HIGH);
  debug((millis() - t1) + " ms");
}

void closeLid() {
  debug("Close lid");
  analogWrite(POWER_LEVEL, closeSpeed);
  digitalWrite(POSITIVE_LID, LOW);
  int x;
  x = 0;
  int pin = digitalRead(LID_CLOSED_PIN);
  while(pin == LOW && x < 1500) {
    delay(2);
    x = x + 1;
    pin = digitalRead(LID_CLOSED_PIN);
  }
  if(pin == HIGH)
    debug("Button hit");
  digitalWrite(POSITIVE_LID, HIGH);
}

void fog(long duration) {
  debug("Start fog");
  digitalWrite(FOG_PIN, LOW);
  fogTimer = millis() + duration;
}

void vampire() {
  if(state == ARMED) {
    // Do the fog machine direct to pin rather than async
    debug("Start fog");
    digitalWrite(FOG_PIN, LOW);
    delay(FOG_DURATION);
    digitalWrite(FOG_PIN, HIGH);
    
    openLid();  // Sync method so it blocks, just over 3 sec
    delay(OPEN_TO_JUMP);
    
    fire();
  }
}
