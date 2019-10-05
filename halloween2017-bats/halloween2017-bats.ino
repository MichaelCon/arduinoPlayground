// This is a fairly simple app to basically control one servo based on web requests or serial commands
/// Walk through the pins
#define SERIAL_IN 0
#define SERIAL_OUT 1

#define BAT_SERVO 3
#define SD_CARD_BLOCK 4

// Pin 10-13 are used by ethernet shield

#define SERVO_BOTTOM 25    // bat down
#define SERVO_TOP 180     // bat up
#define SERVO_FLAP 35    // bat mostly down

#define FLAP_COUNT 15
#define FLAP_DURATION 350

#include <Servo.h> 
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xEE, 0xEF, 0xFE, 0xEF }; //physical mac address
byte ip[] = { 192, 168, 1, 21 }; // arduino server ip in lan
EthernetServer server(2148); //arduino server port

long flapTimer;
int flapsRemaining;
int currentHeight = SERVO_BOTTOM;

Servo batServo;  // create servo object to control a servo 

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  batServo.attach(BAT_SERVO);  // attaches the servo on a pin to the servo object 

  // disable SD card
  pinMode(SD_CARD_BLOCK, OUTPUT);
  digitalWrite(SD_CARD_BLOCK, HIGH);

  // Network start
  Ethernet.begin(mac, ip);
  server.begin();
}

// the loop function runs over and over again forever
void loop() {
  doRead();       // Check for serial request
  checkWebRequests(); // Web
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
    client.println("<a href=\"a?command=1\">UP</a><br/>");
    client.println("<a href=\"a?command=2\">2</a><br/>");
    client.println("<a href=\"a?command=3\">3</a><br/>");
    client.println("<a href=\"a?command=4\">4</a><br/>");
    client.println("<a href=\"a?command=5\">5</a><br/>");
    client.println("<a href=\"a?command=6\">6</a><br/>");
    client.println("<br/>");
    //client.println("Current Arm PIN : ");
    //client.println(digitalRead(READ_LEVER));
    client.println("</BODY>");
    client.println("</HTML>");
}

void checkTimers() {
  if(flapTimer > 0 && flapTimer < millis()) {
    flapsRemaining = flapsRemaining - 1;
    if(flapsRemaining > 0) {
      if(currentHeight == SERVO_BOTTOM) {
        currentHeight = SERVO_FLAP;
      } else {
        currentHeight = SERVO_BOTTOM;
      }
      flapTimer = millis() + FLAP_DURATION;
    } else {
      currentHeight = SERVO_TOP;
      flapTimer = 0;
    }
    batServo.write(currentHeight);
  }
}

void flap() {
  flapTimer = millis() - 1;
  flapsRemaining = FLAP_COUNT;
}

/** Read a character from serial and react */
void readCommand() {
  // Read the command 
  int command = Serial.read();
  doCommand(command);
}

void doCommand(int command) {
  if(command == '1') {
      batServo.write(0);
  }
  if(command == '2') {
      batServo.write(20);
  }
  if(command == '3') {
      batServo.write(50);
  }
  if(command == '4') {
      batServo.write(75);
  }
  if(command == '5') {
      batServo.write(125);
  }
  if(command == '6') {
      batServo.write(160);
  }
  if(command == '7') {
      batServo.write(180);
  }
  if(command == 'f') {
      flap();
  }
}
