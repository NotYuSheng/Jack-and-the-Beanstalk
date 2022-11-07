//Base Light
#include <FastLED.h>
//LoadCell
#include <Arduino.h>
#include "HX711.h"
//Song
#include "SerialMP3Player.h"
//Motor
#include <Servo.h>
//Speaker
#define RX 10
#define TX 11
SerialMP3Player mp3(RX, TX);

//Base Light
#define LED_PIN 6
#define NUM_LEDS 11
#define BRIGHTNESS 80
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

//Flowers zones
#define NUM_LEDS_Z1 4
#define NUM_LEDS_Z2 6
#define NUM_LEDS_Z3 8
#define NUM_LEDS_Z4 10
#define LED_TYPE_FLOWER NEOPIXEL

const int zone1_pin = 2;
const int zone2_pin = 3;
const int zone3_pin = 7;
const int zone4_pin = 8;
int count = 1;

CRGB leds1[NUM_LEDS_Z1];
CRGB leds2[NUM_LEDS_Z2];
CRGB leds3[NUM_LEDS_Z3];
CRGB leds4[NUM_LEDS_Z4];

CRGB flower1_color, flower2_color, flower3_color, flower4_color;

CRGBPalette16 currentPalette;
TBlendType currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

//Servo
Servo myservo;
int pos = 0;
int isUp = 1;

// Load Cell - HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 5;
float previousWeight = 0.0;
float difference = 0.0;
float avg = 0.0;
bool initial = true;
bool running = false;
uint8_t startSecond;

HX711 scale;

void setup() {
  delay(3000);  // power-up safety delay

  //Song--------------------------------------------------------------------------------------------
  Serial.begin(9600);  // start serial interface
  mp3.begin(9600);     // start mp3-communication
  delay(500);          // wait for init

  mp3.sendCommand(CMD_SEL_DEV, 0, 2);  //select sd-card

  //Load Cells---------------------------------------------------------------------------------------
  Serial.begin(57600);
  Serial.println("HX711 Demo");
  Serial.println("Initializing the scale");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());  // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));  // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));  // print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight (not set) divided
  // by the SCALE parameter (not set yet)

  scale.set_scale(-66664 / 326);  // this value is obtained by calibrating the scale with known weights; see the README for details

  scale.tare();  // reset the scale to 0

  Serial.println("After setting up the scale:");

  Serial.print("read: \t\t");
  Serial.println(scale.read());  // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));  // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));  // print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight, divided
  // by the SCALE parameter set with set_scale

  Serial.println("Readings:");

  //Base Light--------------------------------------------------------------------------------------
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  //Flowers Light--------------------------------------------------------------------------------------
  // FastLED.addLeds<LED_TYPE_FLOWER, zone1_pin>(leds1, NUM_LEDS_Z1);
  // FastLED.addLeds<LED_TYPE_FLOWER, zone2_pin>(leds2, NUM_LEDS_Z2);
  // FastLED.addLeds<LED_TYPE_FLOWER, zone3_pin>(leds3, NUM_LEDS_Z3);
  // FastLED.addLeds<LED_TYPE_FLOWER, zone4_pin>(leds4, NUM_LEDS_Z4);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, INPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);

  FastLED.setBrightness(255);

  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;

  //Servo ----------
  myservo.attach(9);
}

void loop() {
  //Load Cell---------------------------------------------------------------------------------------
  if (running == false) {
    delay(1000);
    Serial.print("average: \t");
    avg = scale.get_units(10);
    Serial.println(avg, 5);

    difference = abs(avg - previousWeight);
    Serial.print("Difference: \t");
    Serial.println(difference, 3);

    previousWeight = avg;
    Serial.print("previousWeight: \t");
    Serial.println(previousWeight, 5);

    startSecond = millis() / 1000;

    //For Servo
    pos = 0;
    isUp = 1;
  }

  if (abs(difference) >= 1.5 && !initial) {  //Detect coin's weight
    Serial.println("Im here, start lightshow");
    mp3.play();  //Start song
    //startSecond = millis() / 1000;
    running = true;

    //Servo-------------------------------------------------------------------
    myservo.write(pos);

    if (pos >= 180) {
      isUp = 0;
    }

    if (isUp == 1) {
      pos++;
      // delay(15);
    }
    else if ((isUp == 0) && (pos == 0)) {
      pos = 0;
    }
    else if (isUp == 0) {
      pos--;
      // delay(15);
    }

    //Start Base Light-------------------------------------------------------------------------------
    ChangePalettePeriodically();

    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */

    FillLEDsFromPaletteColors(startIndex);
    // LightUpFlowers();

    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
  }

  initial = false;
  //delay(1000);
}

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  uint8_t brightness = 255;

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void ChangePalettePeriodically() {
  //uint8_t secondHand = (millis() / 1000) % 60;
  uint8_t secondHand = (millis() / 1000) - startSecond;
  static uint8_t lastSecond = 78;
  // Serial.println(secondHand);

  if (lastSecond >= secondHand) {
    if (secondHand == 0) {
      currentPalette = RainbowColors_p;
      currentBlending = LINEARBLEND;
    }
    // if ((secondHand == 1) || (secondHand == 9) || (secondHand == 11)) {
    //   currentPalette = RainbowColors_p;
    //   currentBlending = LINEARBLEND;
    // }
    // if ((secondHand == 8) || (secondHand == 10) || (secondHand == 14)) {
    //   SetupWhitePalette();
    //   currentBlending = LINEARBLEND;
    // }
    // if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
    // if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
    // if( secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
    // if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
    // if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
    // if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
    if (secondHand == 20) {
      SetupTotallyRandomPalette();
      currentBlending = LINEARBLEND;
      Serial.println("20");
    }

    // if (secondHand == 30) {
    //   currentPalette = CloudColors_p;
    //   currentBlending = LINEARBLEND;
    //   Serial.println("30");
    // }

    if (secondHand == 42) {
      currentPalette = CloudColors_p;
      currentBlending = LINEARBLEND;
      Serial.println("42");
    }

    if (secondHand == 62) {
      currentPalette = OceanColors_p;
      currentBlending = LINEARBLEND;
      Serial.println("62");
    }

    if ((secondHand == 40) && (secondHand == 41)) {  //Off everything
      digitalWrite(2, LOW);
      digitalWrite(3, LOW);
      digitalWrite(7, LOW);
      digitalWrite(8, LOW);

      SetupBlackPalette();
      currentBlending = NOBLEND;

      Serial.println("40");
    }

    //FLOWERS----------------------------------------------------------------------------------------
    if (secondHand % 4 == 1) {
      digitalWrite(2, HIGH);
      digitalWrite(3, LOW);
      digitalWrite(7, LOW);
      digitalWrite(8, LOW);

    } else if (secondHand % 4 == 2) {
      digitalWrite(3, HIGH);
      digitalWrite(2, LOW);
      digitalWrite(7, LOW);
      digitalWrite(8, LOW);

    } else if (secondHand % 4 == 3) {
      digitalWrite(7, HIGH);
      digitalWrite(3, LOW);
      digitalWrite(2, LOW);
      digitalWrite(8, LOW);

    } else if (secondHand % 4 == 0) {
      digitalWrite(8, HIGH);
      digitalWrite(3, LOW);
      digitalWrite(7, LOW);
      digitalWrite(2, LOW);
    }
  }

  if (secondHand >= lastSecond) {
    running = false;
    initial = true;

    digitalWrite(2, LOW);
    digitalWrite(3, LOW);
    digitalWrite(7, LOW);
    digitalWrite(8, LOW);

    SetupBlackPalette();
    currentBlending = NOBLEND;

    mp3.pause();
  }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette() {
  for (int i = 0; i < 16; i++) {
    currentPalette[i] = CHSV(random8(), 255, random8());
  }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette() {
  // 'black out' all 16 palette entries...
  fill_solid(currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;
}

void SetupBlackPalette() {
  fill_solid(currentPalette, 16, CRGB::Black);
}

void SetupWhitePalette() {
  fill_solid(currentPalette, 16, CRGB::White);
}


// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette() {
  CRGB purple = CHSV(HUE_PURPLE, 255, 255);
  CRGB green = CHSV(HUE_GREEN, 255, 255);
  CRGB black = CRGB::Black;

  currentPalette = CRGBPalette16(
                     green, green, black, black,
                     purple, purple, black, black,
                     green, green, black, black,
                     purple, purple, black, black);
}

// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM = {
  CRGB::Red,
  CRGB::Gray,  // 'white' is too bright compared to red and blue
  CRGB::Blue,
  CRGB::Black,

  CRGB::Red,
  CRGB::Gray,
  CRGB::Blue,
  CRGB::Black,

  CRGB::Red,
  CRGB::Red,
  CRGB::Gray,
  CRGB::Gray,
  CRGB::Blue,
  CRGB::Blue,
  CRGB::Black,
  CRGB::Black
};
