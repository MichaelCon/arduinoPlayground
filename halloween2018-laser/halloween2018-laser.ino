#include <Adafruit_NeoPixel.h>
#include <Servo.h> 
#include <SPI.h>
#include <Ethernet.h>

// PWM pins (3, 5, 6, 9, 10, or 11)
// Pin 10-13 are used by ethernet shield
#define TRIG_PIN 2       // Trigger pin for echo (white)
#define ECHO_PIN 3       // Pin for reading in echo (blue)
#define LASER_RELAY 8       // Turn the laser on/off
#define LASER_SERVO 5       // Control the servo
#define SMOKE_RELAY 7       // Turn the fog machine on/off
#define SMOKE_SERVO 9       // Control the servo
#define SMALL_LASERS 6      // Transistor led small lasers 
#define MIN_SMOKE 60
#define MAX_SMOKE 120
#define MIN_LASER 60
#define MAX_LASER 120
#define SD_CARD_BLOCK 4

#define SMOKE_CENTER 90.0
#define SMOKE_FACTOR 30.0   // Degrees off from center max
#define LASER_CENTER 87.0
#define LASER_FACTOR 16.0
#define STEP_CHANGE 0.04    // Percent to change in each loop
#define LOOP_WAIT 40        // Starting amount to wait in each loop
#define N_CYCLES 5          // Number of times the laser goes back and forth
#define S_CYCLES 2          // Number of times the laser goes back and forth if it recently went
#define TRIGGER_DISTANCE 6.0
#define TRIGGER_REPEAT 5    // number of conseq. readings to trigger on
#define TRIGGER_WAIT 30000
#define SHORT_CYCLES_WAIT 300000  // 5 minutes for longer cycles
#define BLINK_SPEED 1000    // cycle time for small lasers

byte mac[] = { 0xDE, 0xAB, 0xEE, 0xEF, 0xFE, 0xEA }; //physical mac address
byte ip[] = { 192, 168, 0, 20 }; // arduino server ip in lan
EthernetServer server(80); //arduino server port
IPAddress computer(192,168,0,100);
IPAddress lights(192,168,0,21);

Servo smokeServo;  // create servo object to control a servo 
Servo laserServo;
int smokeAt = 90;
int laserAt = 90;

unsigned long loopWait = LOOP_WAIT;

float stepSize = 0.0;
float currentPos = 0.0;
int cyclesRemaining = 0;

unsigned long lastLoop = 0;
unsigned long noTriggerUntil = 0;
unsigned long fewCyclesUntil = 0;
int closeCounter = TRIGGER_REPEAT;

boolean soundsOn = false;
boolean distanceOn = true;

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
  
  pinMode(TRIG_PIN, OUTPUT); // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT); // Sets the echoPin as an Input
  
  pinMode(LASER_RELAY, OUTPUT); //pin selected to control
  pinMode(SMOKE_RELAY, OUTPUT); //pin selected to control
  pinMode(SMALL_LASERS, OUTPUT); //pin selected to control
  digitalWrite(LASER_RELAY, HIGH);    // No laser
  digitalWrite(SMOKE_RELAY, HIGH);    // No smoke
  digitalWrite(SMALL_LASERS, LOW);    // No side lasers

  smokeServo.attach(SMOKE_SERVO);  // attaches the servo on a pin to the servo object 
  smokeServo.write(smokeAt);
  
  laserServo.attach(LASER_SERVO);
  laserServo.write(laserAt);

  lastLoop = millis();
}

/* The main loop */
void loop() {
  doRead();             // check the serial port for a command
  checkWebRequests();   // check the network for a command
  if(stepSize != 0) {     // If we are moving, take a step
    takeStep();
  }
  blinkSmallLasers();
  checkDistance();    // See if someone is close
  variableDelay();
}

/** Waits until 'loopWait' ms have passed since last call.  This removes all the time 
 *  spent in other functions so the loop runs closer to regular intervals */
void variableDelay() {
  // Note:  had a previous bug not realizing the longs are unsigned
  unsigned long elapsedTime = millis() - lastLoop;
  if(loopWait > elapsedTime)
    delay(loopWait - elapsedTime);
  lastLoop = millis();
}

/** Check if the distance threshold was broken */
void checkDistance() {
  if(millis() > noTriggerUntil && distanceOn) {
    float current = readDistance();
    if(current < TRIGGER_DISTANCE) {
      closeCounter -= 1;
      if(closeCounter <= 0) {
        int n = N_CYCLES;
        if(millis() < fewCyclesUntil) {
          n = S_CYCLES;
        }
        startAllLaser(S_CYCLES);
        noTriggerUntil = millis() + TRIGGER_WAIT;
        fewCyclesUntil = millis() + SHORT_CYCLES_WAIT;
        closeCounter = TRIGGER_REPEAT;
      }
    } else {
      closeCounter = TRIGGER_REPEAT;
    }
    //Serial.println(current);
  }
}

/* Moves both servos over one step.  The path isn't linear but should be close enough */
void takeStep() {
  float lastPos = currentPos;
  currentPos += stepSize;
  if(currentPos < -1.0) {
    stepSize = -stepSize;
    currentPos = -1.0;
  } else if(currentPos > 1.0) {
    stepSize = -stepSize;
    currentPos = 1.0;    
  }
  smokeAt = (int) (SMOKE_CENTER + SMOKE_FACTOR * currentPos);
  laserAt = (int) (LASER_CENTER + LASER_FACTOR * currentPos);
  tellServos();

  // Check to see if we should stop
  if(currentPos < 0 && lastPos >= 0) {  // true if we just past the centerpoint
    cyclesRemaining -= 1;
    Serial.println("Cycles left");
    Serial.println(cyclesRemaining);
    if(cyclesRemaining <= 0) {
      stopAllLaser();
    }
  }
}

boolean smallLaserState = false;
unsigned long laserBlinkTimer = 0;
void blinkSmallLasers() {
  unsigned long now = millis();
  if(now > noTriggerUntil) {   // Turn off after timeout
    if(smallLaserState) {
      digitalWrite(SMALL_LASERS, LOW);
    }
  } else {
    if(now > laserBlinkTimer) {
      smallLaserState = !smallLaserState;
      if(smallLaserState) {
        digitalWrite(SMALL_LASERS, HIGH);
      } else {
        digitalWrite(SMALL_LASERS, LOW);
      }
      laserBlinkTimer = now + BLINK_SPEED;
    }
  }
}
void startBlink() {
  digitalWrite(SMALL_LASERS, HIGH);
  smallLaserState = true;
  laserBlinkTimer = millis() + BLINK_SPEED;
}

/** Starts a few cycles of the laser and smoke */
void startAllLaser(int n) {
  playLaserSounds();
  callPage(lights, 80, '5');    // turns on flashing red lights
  startBlink();
  // Turn on the laser
  digitalWrite(LASER_RELAY, LOW);
  // Turn on the smoke
  digitalWrite(SMOKE_RELAY, LOW);
  // Start taking steps
  stepSize = STEP_CHANGE;
  // Set the counter which eventually triggers the auto off
  cyclesRemaining = n;
}
/** Stops the laser cycles even if not finished */
void stopAllLaser() {
  stopHumSound();
  callPage(lights, 80, '6');    // turns off flashing red lights
  // Turn off the laser
  digitalWrite(LASER_RELAY, HIGH);
  // Turn off the smoke
  digitalWrite(SMOKE_RELAY, HIGH);
  // Stop taking steps
  stepSize = 0;
}

void tellServos() {
  smokeServo.write(smokeAt);
  laserServo.write(laserAt);  
}

/** Read the serial port */
void doRead() {
  if(Serial.available() > 0) {
    int command = Serial.read();
    doCommand(command);
  }
}

/** Check for a web request */
void checkWebRequests() {
  // Create a client connection
  EthernetClient client = server.available();
  if (client) {
    debug("New client");
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
            debug("closed client");
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
    client.println("<TITLE>Laser Controller</TITLE>");
    client.println("</HEAD>");
    client.println("<BODY>");
    client.println("<H1>LASER</H1>");
    client.println("<a href=\"a\">Refresh</a><br/>");
    client.println("<a href=\"a?command=2\">Laser On</a><br/>");
    client.println("<a href=\"a?command=1\">Laser Off</a><br/>");
    client.println("<a href=\"a?command=w\">Motion On</a><br/>");
    client.println("<a href=\"a?command=q\">Motion Off</a><br/>");
    client.println("<br/>");
    client.println("</BODY>");
    client.println("</HTML>");
}

void doCommand(int command) {
  // Start and 
  if(command == 'w') {
    stepSize = STEP_CHANGE;
  }
  if(command == 'q') {
    stepSize = 0.0;
  }
  if(command == '0') {
    noTriggerUntil = 0;
  }
  if(command >= '1' && command <='9') {
    startAllLaser(command - '1' + 1);
  }
  if(command == 'l') {
    digitalWrite(LASER_RELAY, HIGH);
  }
  if(command == 'L') {
    digitalWrite(LASER_RELAY, LOW);
  }
  if(command == 's') {
    digitalWrite(SMOKE_RELAY, HIGH);
  }
  if(command == 'S') {
    digitalWrite(SMOKE_RELAY, LOW);
  }
  if(command == 'r') {
    digitalWrite(SMALL_LASERS, LOW);
  }
  if(command == 'R') {
    digitalWrite(SMALL_LASERS, HIGH);
  }
  if(command == 'h') {
    soundsOn = true;
  }
  if(command == 'H') {
    soundsOn = false;
  }
  if(command == 'd') {
    distanceOn = true;
  }
  if(command == 'D') {
    distanceOn = false;
  }
  adjustmentCommands(command);
}

/** A group of commands useful during setup and testing */
void adjustmentCommands(int command) {
  // Manually adjust the smoke servo
  if(command == 'z') {
    smokeAt += 2;
    smokeServo.write(smokeAt);
    Serial.println(smokeAt);
  }
  if(command == 'x') {
    smokeAt -= 2;
    smokeServo.write(smokeAt);
    Serial.println(smokeAt);
  }
  // Manually check the laser servo
  if(command == 'c') {
    laserAt += 2;
    laserServo.write(laserAt);
    Serial.println(laserAt);
  }
  if(command == 'v') {
    laserAt -= 2;
    laserServo.write(laserAt);
    Serial.println(laserAt);
  }
  // Manually adjust the wait time in each loop
  if(command == 'b') {
    loopWait -= 1;
    Serial.println(loopWait);    
  }
  if(command == 'n') {
    loopWait += 1;
    Serial.println(loopWait);
  }

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
  distance = (((float) duration)/2) / 74;
  distance /= 12;
  if(distance == 0)
    distance = 20;
  return distance;
}

void playLaserSounds() {
  if(soundsOn) {
      callPage(computer, 8080, 'l');
  }
}
void stopHumSound() {
  if(soundsOn) {
      callPage(computer, 8080, 'q');
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
      //Serial.print(c);
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

