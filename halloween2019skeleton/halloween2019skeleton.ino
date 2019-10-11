// Guillotine control + skeleton
#include <Servo.h>
#include <Ethernet.h>
#include <Adafruit_NeoPixel.h>

/// Walk through the pins
#define SERIAL_IN 0
#define SERIAL_OUT 1
#define EYES_PIN 3
#define SD_CARD_BLOCK 4
#define SKULL_SERVO 6
#define JAW_SERVO 7
// Pin 10-13 are used by ethernet shield

#define JAW_OPEN 130
#define JAW_CLOSED 50
#define JAW_DELAY 200L

Servo skullServo;
int skullAt = 90;
Servo jawServo;
boolean jawOpen = false;
long jawTimer = 0;
int jawMoves = 0;

byte mac[] = { 0xDE, 0xDD, 0xFF, 0xEF, 0xFE, 0xEA }; //physical mac address
byte ip[] = { 192, 168, 0, 22 }; // arduino server ip in lan
EthernetServer server(80); //arduino server port
IPAddress computer(192,168,0,100);

// Initialize 2 eyes
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel eyes = Adafruit_NeoPixel(2, EYES_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // initialize digital pins.
  skullServo.attach(SKULL_SERVO);  // attaches the servo on a pin to the servo object 
  skullServo.write(skullAt);
  jawServo.attach(JAW_SERVO);
  jawServo.write(JAW_CLOSED);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);    
  debug("setup complete");

  // Eyes
  eyes.begin();
  eyes.setPixelColor(0, eyes.Color(50, 50, 0));
  eyes.setPixelColor(1, eyes.Color(50, 50, 0));
  eyes.setPixelColor(2, eyes.Color(50, 50, 0));
  eyes.show();
  
  // Network start
  Ethernet.begin(mac, ip);
  server.begin();
  debug("Setup complete");

  // disable SD card
  pinMode(SD_CARD_BLOCK, OUTPUT);
  digitalWrite(SD_CARD_BLOCK, HIGH);
}

void loop() {
  doRead();
  
  checkTimers();
  //checkWebRequests();
  //rainbowCycle(100);
  eyes.setPixelColor(0, eyes.Color(50, 50, 0));
  eyes.setPixelColor(1, eyes.Color(50, 50, 0));

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
            int scott = readS.indexOf("scott=");
            if(scott > 0) {
              scott += 6;
              int angle = parseNumber(readS, scott + 6);
              skullServo.write(angle);
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

/** Simple int read */
int parseString(String s, int from, int to) {
  char carray[6];
  s.substring(from, to).toCharArray(carray, to - from + 1);
  int i = atoi(carray);
  return i;
}

/** Need to set timers since we are single threaded.  Need to continue to loop */
void checkTimers() {
  if(jawTimer != 0 && jawTimer < millis()) {
    toggleJaw();
    jawMoves--;
    if(jawMoves > 0) {
      jawTimer = millis() + JAW_DELAY;
    } else {
      jawTimer = 0;
    }
  }
}

void debug(String s) {
  Serial.println(s);  
}

void openJaw() {
  jawOpen = true;
  jawServo.write(JAW_OPEN);
}
void closeJaw() {
  jawOpen = false;
  jawServo.write(JAW_CLOSED);
}
void toggleJaw() {
  if(jawOpen) {
    closeJaw();
  } else {
    openJaw();
  }
}
/** Open and close the jaw "chomps" number of times */
void chatterJaw(int chomps) {
  jawMoves = (chomps * 2) - 1;
  openJaw();
  jawTimer = millis() + JAW_DELAY;
}

/** Perform an action */
void doCommand(int command) {
  Serial.println(command);
  if(command == 'J') {
    Serial.println("open jaw");
    openJaw();
  }
  if(command == 'K') {
    closeJaw();
  }
  if(command == 'C') {
    chatterJaw(5);
  }
  if(command == 't') {
    callPage(computer, 'c');
  }
  if(command == '2') {
    skullServo.write(40);
  }
  if(command == '3') {
    Serial.println("Left");
  eyes.setPixelColor(0, eyes.Color(50, 0, 0));
  eyes.setPixelColor(1, eyes.Color(50, 0, 0));
  eyes.show();
    skullServo.write(70);
  }
  if(command == '4') {
    Serial.println("Straight");
  eyes.setPixelColor(0, eyes.Color(255, 255, 255));
  eyes.setPixelColor(1, eyes.Color(255, 255, 255));
  eyes.show();
    skullServo.write(90);
  }
  if(command == '5') {
    Serial.println("Right");
  eyes.setPixelColor(0, eyes.Color(0, 0, 50));
  eyes.setPixelColor(1, eyes.Color(0, 0, 50));
  eyes.show();
    skullServo.write(110);
  }
  if(command == '6') {
    skullServo.write(140);
  }
  if(command == '7') {
    skullServo.write(170);
  }
}

void setEyes(uint32_t color) {
  eyes.setPixelColor(0, color);
  eyes.setPixelColor(1, color);
  eyes.show();
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

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< eyes.numPixels(); i++) {
      eyes.setPixelColor(i, Wheel(((i * 256 / eyes.numPixels()) + j) & 255));
    }
    eyes.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return eyes.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return eyes.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return eyes.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
