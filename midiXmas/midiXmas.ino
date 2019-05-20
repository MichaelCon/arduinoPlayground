// ON our midi wire... (this is wrong, it is reversed)
// Pin 1 - white
// Pin 2 - shielding/ground
// Pin 3 - red
// Pin 4 - blue
// Pin 5 - green
// -----------------------------------------------------------------------------

#include <MIDI.h>
#include <Adafruit_NeoPixel.h>

#define LIGHTS_PIN 6
// Number of lights
#define N 300
#define LOW_KEY 36
#define RANGE 61
#define PIXELS_PER_KEY 6

#define RESET_ALL_FREQUENCY 4000

MIDI_CREATE_DEFAULT_INSTANCE();

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N, LIGHTS_PIN, NEO_RGB + NEO_KHZ800);

//uint32_t offColor = strip.Color(100, 0, 0);
uint32_t offColor = strip.Color(84, 44, 10);
uint32_t onColor = strip.Color(0, 200, 0);

void setup()
{
    // Connect the handleNoteOn function to the library,
    // so it is called upon reception of a NoteOn.
    MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function

    // Do the same for NoteOffs
    MIDI.setHandleNoteOff(handleNoteOff);

    // Initiate MIDI communications, listen to all channels
    MIDI.begin(MIDI_CHANNEL_OMNI);

    pinMode(LED_BUILTIN, OUTPUT);

    strip.begin();
    setAll(offColor);
}

long nextReset = 0;

void loop()
{
    // Call MIDI.read the fastest you can for real-time performance.
    MIDI.read();
    // There is no need to check if there are messages incoming
    // if they are bound to a Callback function.
    // The attached method will be called automatically
    // when the corresponding message has been received.   
    long now = millis();
    if(now > nextReset) {
      setAll(offColor);
      nextReset = now + RESET_ALL_FREQUENCY;
    }
}

// This function will be automatically called when a NoteOn is received.
void handleNoteOn(byte channel, byte pitch, byte velocity)
{
    // Do whatever you want when a note is pressed.
    setPixelsByPitch(pitch, true);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
    // Do something when the note is released.
    setPixelsByPitch(pitch, false);
    // Note that NoteOn messages with 0 velocity are interpreted as NoteOffs.
}

// -----------------------------------------------------------------------------

void setAll(uint32_t color) {
  for(uint16_t i=0; i< N; i++) {
      strip.setPixelColor(i, color);
  }
  strip.show();
}

void setPixel(int i, uint32_t color) {
  strip.setPixelColor(i, color);
  strip.show();
}

// This version divides the lights equally on the whole keyboard.
void setPixelsByPitchV1(byte pitch, uint32_t color) {
  int startPixel = (int) (((float) pitch - LOW_KEY) * N / RANGE);
  for(int i = 0; i < PIXELS_PER_KEY; i++) {
    strip.setPixelColor(i + startPixel, color);
  }
  strip.show();
}

#define LIGHTS_PER_KEY 25
#define KEY_COUNT 12
void setPixelsByPitch(byte pitch, boolean on) {
  uint32_t c = offColor;
  if(on) {
    c = pickColor(pitch);
  }
  int startPixel = ((int) pitch) % KEY_COUNT * LIGHTS_PER_KEY;
  for(int i = 0; i < LIGHTS_PER_KEY; i++) {
    strip.setPixelColor(i + startPixel, c);
  }
  strip.show();
}

uint32_t pickColor(byte key) {
  if(key < 48) {
   return strip.Color(250, 0, 0);
  } else if(key < 60) {
   return strip.Color(250, 125,0);
  } else if(key < 72) {
   return strip.Color(0,250,0);
  } else if(key < 84) {
   return strip.Color(0,0,250);
  } else {
   return strip.Color(140, 0, 180);
  }
}

void rainbowTree() {
  
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t wheel(byte WheelPos) {
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







