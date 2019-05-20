#include <Adafruit_NeoPixel.h>
#include <Servo.h> 
#include <SPI.h>
#include <Ethernet.h>

// PWM pins (3, 5, 6, 9, 10, or 11)
// Pin 10-13 are used by ethernet shield
#define SD_CARD_BLOCK 4
#define LIGHTS_PIN 9
// Number of lights
#define N 100

// Loop delay
#define DELAY 20
#define LIGHT_DELAY_FACTOR 8

byte mac[] = { 0xDE, 0xEE, 0xEE, 0xEF, 0xFE, 0xEA }; //physical mac address
byte ip[] = { 192, 168, 0, 21 }; // arduino server ip in lan
EthernetServer server(80); //arduino server port
IPAddress computer(192,168,0,100);
IPAddress laser(192,168,0,20);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N, LIGHTS_PIN, NEO_RGB + NEO_KHZ800);

uint32_t restColor = strip.Color(244, 40, 0);
uint32_t redColor = strip.Color(100, 0, 0);
uint32_t redOffColor = strip.Color(10, 0, 0);
uint32_t countDownColor = strip.Color(0, 250, 0);
uint32_t countDownOffColor = strip.Color(25, 25, 0);
    
#define SOLID_REST 1
#define FLASH_RED 2
#define COUNTDOWN 3
int lightsState = FLASH_RED;
int countDown = N/2;

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

  pinMode(LIGHTS_PIN, OUTPUT);
  strip.begin();
  setAll(strip.Color(244, 40, 0));
}

void loop() {
  doRead();
  checkWebRequests();
  stepLights();
  delay(DELAY);
}

void doRead() {
  if(Serial.available() > 0) {
    debug("+");
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
    client.println("<TITLE>Lights Controller</TITLE>");
    client.println("</HEAD>");
    client.println("<BODY>");
    client.println("<H1>Candy</H1>");
    client.println("<a href=\"a\">Refresh</a><br/>");
    client.println("<a href=\"a?command=4\">Rest</a><br/>");
    client.println("<a href=\"a?command=5\">Flash</a><br/>");
    client.println("<a href=\"a?command=6\">Countdown</a><br/>");
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
  if(command == '4') {
    changeLights(SOLID_REST);
  }
  if(command == '5') {
    changeLights(FLASH_RED);
  }
  if(command == '6') {
    changeLights(COUNTDOWN);
  }
}

void callPage(IPAddress location, int port, char command) {
  EthernetClient client;

  // if you get a connection, report back via serial:
  if (client.connect(location, port)) {
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

boolean redOn = true;
int skipStep = 0;

void changeLights(int newState) {
  if(newState == SOLID_REST) {
    setAll(restColor);
  }
  if(newState == FLASH_RED) {
    skipStep = 0;
  }
  if(newState == COUNTDOWN) {
    setAll(countDownColor);
    skipStep = 0;
    countDown = N/2;
  }
  lightsState = newState;
}

void stepLights() {
  if(skipStep > 0) {    // skip step allow us to ignore most calls and only act on one of each LIGHT_DELAY_FACTOR calls
    skipStep -= 1;
  } else {
    skipStep = LIGHT_DELAY_FACTOR;
    if(lightsState == FLASH_RED) {
      if(redOn) {
        setAll(redColor);
      } else {
        setAll(redOffColor);
      } 
      redOn = !redOn;
    } else if(lightsState == COUNTDOWN) {
      Serial.println(countDown);
      for(uint16_t i=0; i< N; i++) {
          strip.setPixelColor(i, countDownColor);
      }
      for(uint16_t i = 0; i < N/2 - countDown; i++) {
          strip.setPixelColor(i, countDownOffColor);
          strip.setPixelColor(99 - i, countDownOffColor);
      }
      strip.show();
      countDown -= 1;
      if(countDown == 0) {
        buzzer();
      }
    }
  }
}

// Function to call when the countdown finishes
void buzzer() {
  changeLights(SOLID_REST);
  callPage(laser, 80, '0');
}

void setAll(uint32_t color) {
  for(uint16_t i=0; i< N; i++) {
      strip.setPixelColor(i, color);
  }
  strip.show();
}

void debug(String s) {
  Serial.println(s);
}

