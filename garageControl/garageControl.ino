#define CONTROL_PIN 7
#define DOOR_READ_PIN 2
#define CHECK_DELAY 500

#include <SPI.h>
#include <Ethernet.h>

// BEGIN values found in my private header file
#include "secrets.h"
// const char* awsUrl = AWS_URL;
// END values found in my private file

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //physical mac address
//byte ip[] = { 192, 168, 1, 177 }; // arduino server ip in lan
byte ip[] = { 10, 0, 0, 177 }; // arduino server ip in lan
EthernetServer server(2147); //arduino server port

String readString;
String key = "BOGUS";

long checkDoorAt = 0;
int lastState = 2;

//////////////////////
/*
  There is a relay to control the garage switch.  The 5V and GND and PIN 7 go to the relay.
*/
void setup(){

  pinMode(CONTROL_PIN, OUTPUT); //pin selected to control
  digitalWrite(CONTROL_PIN, HIGH); // Start it set to high

  
  //start Ethernet
  Ethernet.begin(mac, ip);
  server.begin();

  //enable serial data print 
  Serial.begin(9600); 
}

void makeNewKey() {
  int n = random(0,500000);
  String letterK = "K";
  key = letterK + n + letterK;
}

// Checks the door state every period defined by CHECK_DELAY
void checkDoorState() {
  if(millis() > checkDoorAt) {
    if(digitalRead(DOOR_READ_PIN) != lastState) {
      lastState = digitalRead(DOOR_READ_PIN);
      reportDoorState(lastState);
    }
  }
  checkDoorAt = millis() + CHECK_DELAY;
}

void loop(){
  // Create a client connection
  EthernetClient client = server.available();
  if (client) {
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

          ///////////////
          //Serial.println(readString);
          delay(1);

          /////////////////////
          if(readString.indexOf("GET") == 0) {
            if(readString.indexOf("action") > 0) {
              //Serial.println(readString);
              //Serial.println(key);
              if(readString.indexOf(key) > 0) {  // check the secret
                if(readString.indexOf("action=open") > 0 && digitalRead(DOOR_READ_PIN) == LOW) {
                  toggleDoor();
                }
                if(readString.indexOf("action=close") > 0 && digitalRead(DOOR_READ_PIN) == HIGH) {
                  toggleDoor();
                }
              }
              makeNewKey();
            }
            if(readString.indexOf("json") > 0) {
              wsResponse(client);
            } else {
              htmlResponse(client);
            }
            
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

void wsResponse(EthernetClient client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/json");
    client.println();
    if(digitalRead(DOOR_READ_PIN) == HIGH)
      client.print("{ \"doorstate\": \"open\" ,");
    else
      client.print("{ \"doorstate\": \"closed\" ,");
    client.print("\"key\": \"");
    client.print(key);
    client.println("\"}");
}

void htmlResponse(EthernetClient client) {
    //now output HTML data header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println("<HTML>");
    client.println("<HEAD>");
    client.println("<TITLE>Garage</TITLE>");
    client.println("<style>");
    client.println("body {   font-size: 75pt; text-align: center; }");
    client.println("</style>");
    client.println("</HEAD>");
    client.println("<BODY>");
    client.println("<H1>My Garage</H1>");
    if(digitalRead(DOOR_READ_PIN) == HIGH) {
      client.println("Door is open<br/>");
      client.println("<br/><br/><br/><br/>");
      client.println("<a href=garage.html?action=close&key=" + key + ">Close</a>");
    } else {
      client.println("Door is closed<br/>");
      client.println("<br/><br/><br/><br/>");
      client.println("<a href=garage.html?action=open&key=" + key + ">Open</a>");
    }
    client.println("</BODY>");
    client.println("</HTML>");
}

void toggleDoor() {
  digitalWrite(CONTROL_PIN, LOW);
  delay(250);
  digitalWrite(CONTROL_PIN, HIGH);
}

void reportDoorState(int doorState) {
//  EthernetClient client;
//
//  // if you get a connection, report back via serial:
//  if (client.connect(awsUrl, 80)) {
//    // Make a HTTP request:
//    client.print("GET /halloween/doorbell.jsp?command=");
//    client.print(command);
//    client.println(" HTTP/1.1");
//    client.println("Host: www.ninox.com");
//    client.println("Connection: close");
//    client.println();
//
//    getPage(client);
//  } 
//  else {
//    // if you didn't get a connection to the server:
//    Serial.println("connection failed");
//  }

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
