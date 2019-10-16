// This is essentially a controller for the string of led lights 
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Ethernet.h>

#define PIN 5
#define N 3

// strip.Color(84, 44, 10) <- yellowish Casye and Jen liked 

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N, PIN, NEO_GRB + NEO_KHZ800);

byte mac[] = { 0xDE, 0xAD, 0xEE, 0xEF, 0xFE, 0xEF }; //physical mac address
byte ip[] = { 192, 168, 1, 21 }; // arduino server ip in lan
EthernetServer server(2148); //arduino server port

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  strip.begin();
  setAll(strip.Color(84, 44, 10));
  strip.show();

  // Network start
//  Ethernet.begin(mac, ip);
//  server.begin();
//  debug("Setup complete");

  setAll(strip.Color(0,0,50));
}
int i = 0;

void loop() {

  //checkNetwork();
  //randomFlicker(250);
  //delay(3000);
//    setAll(strip.Color(94, 54, 20));

  //runner(strip.Color(84, 44, 10), strip.Color(255,0,0), 40);
  //theaterChase(strip.Color(127, 127, 127), 50, 15); // White
//    colorWipe(strip.Color(0, 0, 0), 2);
//    colorWipe(strip.Color(94, 54, 20), 5); 
//    colorWipe(strip.Color(50, 180, 50), 5); // Green
//    colorWipe(strip.Color(0, 0, 255), 5); // Blue
  // Send a theater pixel chase in...
  //theaterChase(strip.Color(127, 127, 127), 50, 2); // White
//  theaterChase(strip.Color(127,   0,   0), 50); // Red
//  theaterChase(strip.Color(  0,   0, 127), 50); // Blue
//
//  rainbow(20);
  //rainbowCycle(15);
  //theaterChaseRainbow(50);
//checkWebRequests();
//checkNetwork();
delay(200);
}

void debug(String s) {
  Serial.println(s);
}

void checkNetwork() {
  int from, to;
  String readString;
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
          //debug(readString);
          delay(1);

          // The loop ignore all string that are not a get request
          if(readString.indexOf("GET") == 0) {
            debug(readString);
            from = readString.indexOf("/") + 1;
            to = readString.indexOf("/", from);
            int index = parseString(readString, from, to);
            Serial.println(index);
            if(index >= 0) {
              from = to+1;
              to = readString.indexOf(",", from);
              int red = parseString(readString, from, to);

              from = to + 1;
              to = readString.indexOf(",", from);
              int green = parseString(readString, from, to);

              from = to + 1;
              to = readString.indexOf(" ", from);
              int blue = parseString(readString, from, to);

              Serial.println(red);
              Serial.println(green);
              Serial.println(blue);
              setAll(strip.Color(red, green, blue));

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

int parseString(String s, int from, int to) {
  char carray[6];
  s.substring(from, to).toCharArray(carray, to - from + 1);
  int i = atoi(carray);
  return i;
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
    client.println("<H1>Merry</H1>");
    client.println("<br/>");
    client.println("</BODY>");
    client.println("</HTML>");
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// Fill the dots one after the other with a color
void runner(uint32_t base, uint32_t runner, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, base);
  }
  for(uint16_t i=1; i<strip.numPixels()-1; i++) {
      strip.setPixelColor(i-1, base);
      strip.setPixelColor(i, runner);
      strip.setPixelColor(i+1, runner);
      strip.show();
      delay(wait);
  }
}

void randomFlicker(uint8_t wait) {
  int n = random (0, N);
  uint32_t color = strip.getPixelColor(n);
  strip.setPixelColor(n, strip.Color(255,255,255));
  strip.show();
  delay(wait);
  strip.setPixelColor(n, color);
  strip.show();
  
}

void setAll(uint32_t color) {
  for(uint16_t i=0; i< N; i++) {
      strip.setPixelColor(i, color);
  }
  strip.show();
}

void setRange(uint32_t color, int from, int to) {
  for(uint16_t i=0; i< N; i++) {
      strip.setPixelColor(i, color);
  }
  strip.show();
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait, int cycles) {
  for (int j=0; j<  cycles; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
