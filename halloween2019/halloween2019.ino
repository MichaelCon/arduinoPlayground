// Guillotine control 
#include <Ethernet.h>

/// Walk through the pins
#define SERIAL_IN 0
#define SERIAL_OUT 1

#define NEGATIVE_MOTOR 2   // Other side
#define POSITIVE_MOTOR 3    // Two relays control the motor
#define SD_CARD_BLOCK 4
#define UPPER_SWITCH 5
#define TRIG_PIN 8       // Trigger pin for echo
#define ECHO_PIN 9       // Pin for reading in1
#define LED 13
// Pin 10-13 are used by ethernet shield

#define RELOAD_DELAY 5000L

boolean bladeUp = false;
boolean armed = false;

long reloadTimer = 0;
boolean autoReload = true;

long armedTimer = 0;

int counter = 0;

byte mac[] = { 0xDE, 0xEF, 0xFF, 0xEF, 0xFE, 0xEA }; //physical mac address
byte ip[] = { 10, 0, 0, 99 }; // arduino server ip in lan
EthernetServer server(80); //arduino server port
IPAddress computer(10,0,0,1);

void setup() {
  // initialize digital pins.
  pinMode(POSITIVE_MOTOR, OUTPUT);
  digitalWrite(POSITIVE_MOTOR, HIGH);    // turn the drill off
  pinMode(NEGATIVE_MOTOR, OUTPUT);
  digitalWrite(NEGATIVE_MOTOR, HIGH);    // turn the drill off
  pinMode(UPPER_SWITCH, INPUT);

  pinMode(TRIG_PIN, OUTPUT); // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT); // Sets the echoPin as an Input

  // Open serial communications and wait for port to open:
  Serial.begin(9600);    
  debug("setup complete");

  // Network start
  Ethernet.begin(mac, ip);
  server.begin();
  debug("Setup complete");

  // disable SD card - not needed with bent pin
  pinMode(SD_CARD_BLOCK, OUTPUT);
  digitalWrite(SD_CARD_BLOCK, HIGH);
}

void loop() {
  doRead();
  
  if(armed && bladeUp) {
    if(holeBlocked()) {
      dropTheBlade();
    }
  }

  checkButton();
  checkTimers();
  checkWebRequests();
  delay(20);
}

void checkWebRequests() {
  // Create a client connection
  EthernetClient client = server.available();
  if (client) {
    String readS;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //read char by char HTTP request
        if (readS.length() < 100) {
          //store characters to string 
          readS += c; 
        } 

        //if this is the end of a single line
        if (c == '\n') {
          //debug(readString);
          delay(1);

          // The loop ignore all string that are not a get request
          if(readS.indexOf("GET") == 0) {
            int commandLocation = readS.indexOf("command=");
            if(commandLocation > 0) {
              int command = readS.charAt(commandLocation + 8);
              doCommand(command);
            }
            htmlResponse(client);
            //stopping client
            client.stop();
            //debug("closed client");
          }
          //clearing string for next read
          readS="";
        }
      }
    }
  }
}

int parseNumber(String s, int start) {
  int val = s.charAt(start) - '0';
  start++;
  char next = s.charAt(start);
  while(next >= '0' && next <= '9') {
    val *= 10;
    val += next - '0';
    start++;
    next = s.charAt(start);
  }
  return val;
}

void htmlResponse(EthernetClient client) {
    //now output HTML data header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println("<HTML>");
    client.println("<HEAD>");
    client.println("<TITLE>2019 Halloween</TITLE>");
    client.println("</HEAD>");
    client.println("<BODY>");
    client.println("<H1></H1>");
    client.println("<a href=\"a\">Refresh</a><br/>");
    client.println("<a href=\"a?command=1\">Right</a><br/>");
    client.println("<a href=\"a?command=4\">Center</a><br/>");
    client.println("<a href=\"a?command=7\">Left</a><br/>");
    client.println("<br/>");
    client.println("</BODY>");
    client.println("</HTML>");
}

/** Read the serial port & run a command */
void doRead() {
  if(Serial.available() > 0) {
    int command = Serial.read();    
    doCommand(command);
  }
}

/** Check the button state and light led */
void checkButton() {
  // read the state of the pushbutton value:
  int buttonState = digitalRead(UPPER_SWITCH);
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    // turn LED on:
    digitalWrite(LED, HIGH);
  } else {
    // turn LED off:
    digitalWrite(LED, LOW);
  }
}

/** Simple int read */
int parseString(String s, int from, int to) {
  char carray[6];
  s.substring(from, to).toCharArray(carray, to - from + 1);
  int i = atoi(carray);
  return i;
}

/** Need to set timers since we are single threaded.  Need to continue to loop */
void checkTimers() {
  if(reloadTimer != 0 && reloadTimer < millis()) {
    reloadTimer = 0;
    raiseTheBlade();
  }
  if(armedTimer != 0 && armedTimer < millis()) {
    armedTimer = 0;
    armed = true;
  }
}

void debug(String s) {
  Serial.println(s);  
}

void raiseTheBlade() {
  unsigned long endBy = millis() + 1100;
  
  digitalWrite(NEGATIVE_MOTOR, LOW);    // turn the drill on
  while(digitalRead(UPPER_SWITCH) == LOW && endBy > millis())
    delay(1);
  digitalWrite(NEGATIVE_MOTOR, HIGH);    // turn the drill off
  bladeUp = true;
  debug("Blade Up");
  armedTimer = millis() + 2000L;
}

void dropTheBlade() {
  debug("Lower the blade");
  counter++;
  Serial.println(counter);
  digitalWrite(POSITIVE_MOTOR, LOW);    // turn the drill off
  delay(700);
  digitalWrite(POSITIVE_MOTOR, HIGH);    // turn the drill off  
  bladeUp = false;       
  if(autoReload) 
    reloadTimer = millis() + RELOAD_DELAY;
  armed = false;
}

/** Perform an action */
void doCommand(int command) {
  if(command == 'U') {    // UP - Raise the blade
    raiseTheBlade();
  }
  if(command == 'L') {    // LOWER - Drop the blade
    dropTheBlade();
  }
  if(command == 'R') {
    autoReload = true;
  }
  if(command == 'r') {
    autoReload = false;
  }
  if(command == 't') {
    callPage(computer, 'c');
  }
}

/** Read the distance from the echo sensor - returned in 1/1776th of a foot */
long readDistanceLong() {
  long duration;

  // Clears the trigPin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds - timeout for max default is 200ms
  duration = pulseIn(ECHO_PIN, HIGH, 30000ul);  
  
  return duration;
}

boolean holeBlocked() {
  boolean nothing = false;
  for(int i = 0; i < 4; i++) {
    if(readDistanceLong() > 1776ul) {   //  than a foot
      nothing = true;
    }
  }
  return !nothing;
}

/** Read the distance from the echo sensor - returned in ft */
float readDistance() {
  long duration;
  float distance;

  // Clears the trigPin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds - timeout for max default is 200ms
  duration = pulseIn(ECHO_PIN, HIGH, 30000ul);
  
  // Calculating the distance
  //distance= duration*0.034/2;
  
  //distance = (((float) duration)/2) / 74;
  //distance /= 12;

  distance = ((float) duration)/1776; // in feet
  
  return distance;
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
