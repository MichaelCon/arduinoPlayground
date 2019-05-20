#include <Servo.h> 
#include <SPI.h>
#include <Ethernet.h>

// PWM pins (3, 5, 6, 9, 10, or 11)
// Pin 10-13 are used by ethernet shield
#define SD_CARD_BLOCK 4
// 4 servos
#define LEFT_HOLD 5
#define RIGHT_HOLD 3
#define LEFT_GATE 2
#define RIGHT_GATE 7
#define LEFT_BUTTON 6
#define RIGHT_BUTTON 8
#define SHORT_DELAY 250
#define LONG_DELAY 350

// Loop delay
#define DELAY 20

byte mac[] = { 0xDE, 0xAD, 0xEE, 0xEF, 0xFE, 0xEA }; //physical mac address
byte ip[] = { 192, 168, 0, 19 }; // arduino server ip in lan
EthernetServer server(80); //arduino server port
IPAddress computer(192,168,0,100);
//IPAddress laser(192,168,0,20);

// create servo objects to control each servo 
Servo leftGateServo;  
Servo rightGateServo;  
Servo leftHoldServo;
Servo rightHoldServo;

boolean soundsOn = false;


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // Network start
  Ethernet.begin(mac, ip);
  server.begin();
  debug("Setup complete");

  // disable SD card
  pinMode(SD_CARD_BLOCK, OUTPUT);
  digitalWrite(SD_CARD_BLOCK, HIGH);

  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  
  // attaches the servo on a pin to the servo object
  leftGateServo.attach(LEFT_GATE);   
  rightGateServo.attach(RIGHT_GATE);
  leftHoldServo.attach(LEFT_HOLD);
  rightHoldServo.attach(RIGHT_HOLD);

  leftGate(false);
  rightGate(false);
  leftHold(false);
  rightHold(false);
}

void loop() {
  doRead();
  checkWebRequests();
  checkButtons();
  delay(DELAY);
}

void doRead() {
  if(Serial.available() > 0) {
    debug("+");
    readCommand();
  }
}

void checkButtons() {
  if(digitalRead(LEFT_BUTTON) == LOW) {
    dropLeftCandy();
  }
  if(digitalRead(RIGHT_BUTTON) == LOW) {
    dropRightCandy();
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
    client.println("<TITLE>Candy Controller</TITLE>");
    client.println("</HEAD>");
    client.println("<BODY>");
    client.println("<H1>Candy</H1>");
    client.println("<a href=\"a\">Refresh</a><br/>");
    client.println("<a href=\"a?command=1\">Kit Kat</a><br/>");
    client.println("<a href=\"a?command=2\">Crunch</a><br/>");
    client.println("<br/>");
    client.println("</BODY>");
    client.println("</HTML>");
}

/** Read a character from serial and react */
void readCommand() {
  // Read the command 
  int command = Serial.read();
  doCommand(command);
}

void doCommand(int command) {
  if(command == 'l') {
      leftGate(true);
  }
  if(command == 'L') {
      leftGate(false);
  }
  if(command == 'r') {
      rightGate(true);
  }
  if(command == 'R') {
      rightGate(false);
  }
  if(command == 'm') {
      leftHold(true);
  }
  if(command == 'M') {
      leftHold(false);
  }
  if(command == 's') {
      rightHold(true);
  }
  if(command == 'S') {
      rightHold(false);
  }
  if(command == '1') {
      dropLeftCandy();
  }
  if(command == '2') {
      dropRightCandy();
  }
  if(command == 'h') {
    soundsOn = true;
  }
  if(command == 'H') {
    soundsOn = false;
  }
}

void dropLeftCandy() {
  playSound();
  leftHold(true);
  delay(SHORT_DELAY);
  leftGate(true);
  delay(LONG_DELAY);
  leftGate(false);
  delay(SHORT_DELAY);
  leftHold(false);
}
void dropRightCandy() {
  playSound();
  rightHold(true);
  delay(SHORT_DELAY);
  rightGate(true);
  delay(LONG_DELAY);
  rightGate(false);
  delay(SHORT_DELAY);
  rightHold(false);
}

// Function to control the left gate
void leftGate(boolean state) {
  if(state) {
    leftGateServo.write(100);
  } else {
    leftGateServo.write(80);
  }
}
// Function to control the arm that holds back the cnady
void leftHold(boolean state) {
  if(state) {
    leftHoldServo.write(85);
  } else {
    leftHoldServo.write(102);
  }
}

void rightGate(boolean state) {
  if(state) {
    rightGateServo.write(84);
    //rightGateServo.write(71); // 71 will remove the gate
  } else {
    rightGateServo.write(102);
  }
}

// Function to control the arm that holds back the cnady
void rightHold(boolean state) {
  if(state) {
    rightHoldServo.write(83);
  } else {
    rightHoldServo.write(70);
  }
}

void playSound() {
  if(soundsOn) {
      callPage(computer, 'c');
  }
}

void callPage(IPAddress location, char command) {
  EthernetClient client;

  // if you get a connection, report back via serial:
  if (client.connect(location, 8080)) {
    Serial.println("connected");
    // Make a HTTP request:
    client.print("GET /halloween/doorbell.jsp?command=");
    client.print(command);
    client.println(" HTTP/1.1");
    client.println("Host: www.ninox.com");
    client.println("Connection: close");
    client.println();

    getPage(client);
  } 
  else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }

}

void getPage(EthernetClient client)
{
  int reading = 1;
  while(reading > 0) {
    // if there are incoming bytes available 
    // from the server, read them and print them:
    if (client.available()) {
      char c = client.read();
      Serial.print(c);
    }

    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnecting.");
      client.stop();
      reading = 0;
    }
  }
}

void debug(String s) {
  Serial.println(s);
}

