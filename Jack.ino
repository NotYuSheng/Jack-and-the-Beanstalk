// Base Light
#include <FastLED.h>
// LoadCell
#include <Arduino.h>
#include "HX711.h"
// Song
#include <SoftwareSerial.h>
#include "RedMP3.h"
// Motor
#include <Sweeper.h>
#include <elapsedMillis.h>
Sweeper demoSweeper;  // declare a sweeper object

#include <Servo.h>
// Speaker
#define MP3_RX 11 // RX of Serial MP3 module connect to D7 of Arduino
#define MP3_TX 10 // TX to D8, note that D8 can not be used as RX on Mega2560, you should modify this if you donot use Arduino UNO
MP3 mp3(MP3_RX, MP3_TX);

int8_t index = 0x01;  // the first song in the TF card
int8_t volume = 0x1a; // 0~0x1e (30 adjustable level)
int song = 0;

// Base Light
#define LED_PIN 6
#define NUM_LEDS 11
#define BRIGHTNESS 80
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

// Flowers zones
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

// Servo
Servo myservo;
int pos = 0;
int isUp = 1;
int motor = 0;

// Load Cell - HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 5;
float previousWeight = 0.0;
float difference = 0.0;
float avg = 0.0;
bool initial = true;
bool running = false;
uint8_t startSecond;
uint8_t prevSecond = startSecond;
int repeat = 0;

HX711 scale;

void setup()
{
  previousWeight = 0.0;
  difference = 0.0;
  avg = 0.0;
  initial = true;
  running = false;

  repeat = 0;
  demoSweeper.setTargetGo(0, 3000);
  demoSweeper.sweepAttach(9);
  // demoSweeper.resetTo(0);
  repeat = 0;

  delay(3500); // power-up safety delay

  // Song--------------------------------------------------------------------------------------------
  //    Serial.begin(9600);  // start serial interface
  //  mp3.begin(9600);     // start mp3-communication
  //  mp3.sendCommand(CMD_SEL_DEV, 0, 2);  //select sd-card

  // Load Cells---------------------------------------------------------------------------------------
  initial = true;
  difference = 0;
  Serial.begin(57600);
  Serial.println("Program Initializing");
  Serial.println("Initializing the scale");
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read()); // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20)); // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5)); // print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1); // print the average of 5 readings from the ADC minus tare weight (not set) divided
  // by the SCALE parameter (not set yet)

  scale.set_scale(-66664 / 326); // this value is obtained by calibrating the scale with known weights; see the README for details

  scale.tare(); // reset the scale to 0

  Serial.println("After setting up the scale:");

  Serial.print("read: \t\t");
  Serial.println(scale.read()); // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20)); // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5)); // print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1); // print the average of 5 readings from the ADC minus tare weight, divided
  // by the SCALE parameter set with set_scale

  Serial.println("Readings:");

  // Base Light--------------------------------------------------------------------------------------
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // Flowers Light--------------------------------------------------------------------------------------
  //  FastLED.addLeds<LED_TYPE_FLOWER, zone1_pin>(leds1, NUM_LEDS_Z1);
  //  FastLED.addLeds<LED_TYPE_FLOWER, zone2_pin>(leds2, NUM_LEDS_Z2);
  //  FastLED.addLeds<LED_TYPE_FLOWER, zone3_pin>(leds3, NUM_LEDS_Z3);
  //  FastLED.addLeds<LED_TYPE_FLOWER, zone4_pin>(leds4, NUM_LEDS_Z4);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  SetupBlackPalette();

  // Servo ----------
  myservo.attach(9);
  motor = 0;
}

void loop()
{
  // Load Cell---------------------------------------------------------------------------------------
  if (running == false)
  {
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
    // For Servo
    pos = 0;
    isUp = 1;
    myservo.write(0);

    SetupBlackPalette();
  }
  if (abs(difference) >= 2 && !initial)  //Detect coin's weight
  //if (abs(difference) >= 0 && !initial)
  // if (abs(difference) >= 5 && !initial)
  { // Detect coin's weight
    //Serial.println("Im here, start lightshow");
    //    delay(400);//Requires 500ms to wait for the MP3 module to initialize
    if (song == 0)
    {
      mp3.playWithVolume(index, volume);
      delay(50); //
      song = 1;
    }

    // startSecond = millis() / 1000;
    running = true;

    // Servo-------------------------------------------------------------------
    //    myservo.write(pos);
    //    if (motor == 1)
    //    {
    //      if (pos >= 180)
    //      {
    //        isUp = 0;
    //      }
    //
    //      if (isUp == 1)
    //      {
    //        pos++;
    //        delay(143);
    //      }
    //      else if ((isUp == 0) && (pos == 0))
    //      {
    //        pos = 0;
    //      }
    //      else if (isUp == 0)
    //      {
    //        pos--;
    //        delay(143);
    //      }
    //    }

    // Start Base Light-------------------------------------------------------------------------------
    ChangePalettePeriodically();

    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */

    FillLEDsFromPaletteColors(startIndex);
    // LightUpFlowers();

    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
  }

  initial = false;
}

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
  uint8_t brightness = 255;

  for (int i = 0; i < NUM_LEDS; i++)
  {
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

void ChangePalettePeriodically()
{
  // uint8_t secondHand = (millis() / 1000) % 60;
  uint8_t secondHand = (millis() / 1000) - startSecond;
  static uint8_t lastSecond = 79;

  if (prevSecond != secondHand) 
  {
    repeat = 0;
  }
  
  Serial.println(secondHand);
  //Serial.println(millisHand);
  if (lastSecond >= secondHand)
  {
    if (secondHand == 5 && repeat == 0) {
      demoSweeper.setTargetGo(50, 5000);
      Serial.println("Second 5");
      repeat = 1;
    }
    if (secondHand == 10 && repeat == 0) {
      demoSweeper.setTargetGo(30, 1000);
      Serial.println("Second 11");
      repeat = 1;
    }
    
    if (secondHand == 11 && repeat == 0) {
      demoSweeper.setTargetGo(50, 1000);
      Serial.println("Second 12");
      repeat = 1;
    }
    if (secondHand == 12 && repeat == 0) {
      demoSweeper.setTargetGo(45, 1000);
       Serial.println("Second 16");
       repeat = 1;
    }
    if (secondHand == 13 && repeat == 0) {
      demoSweeper.setTargetGo(130, 3000);
       Serial.println("Second 19");
       repeat = 1;
    }
    
    if (secondHand == 16 && repeat == 0) {
      demoSweeper.setTargetGo(50, 1000);
      repeat = 1;
    }
    if (secondHand == 17 && repeat == 0) {
      demoSweeper.setTargetGo(140, 11000);
      repeat = 1;
    }
    if ((secondHand == 0) || (secondHand == 1))
    {
      FastLED.setBrightness(255);
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
//    if (millisHand % 2 == 0) 
//    {
//        pos ++;
//        myservo.write(pos);
//        Serial.println(pos);
//    }
//    if ((secondHand == 10)) {
//      myservo.write(30);
//    }
//    if ((secondHand >= 11) && (secondHand < 12) && (millisHand % 100 == 0))
//    {
//      Serial.println("5 to 10");
//        pos += 5;
//        myservo.write(pos);
//    }
//    if (secondHand == 13)
//    {
//        myservo.write(45);
//    }
//    if ((secondHand >= 14) && (secondHand < 12) && (millisHand % 100 == 0))
//    {
//        pos ++;
//        myservo.write(pos);
//    }
    if (secondHand == 20)
    {
      SetupTotallyRandomPalette();
      currentBlending = LINEARBLEND;
      Serial.println("20");
    }
    if (secondHand == 28)
    {
      SetupBlackPalette();
      currentBlending = NOBLEND;
      Serial.println("28");
    }

    // if (secondHand == 30)
    // {
    //   currentPalette = CloudColors_p;
    //   currentBlending = LINEARBLEND;
    //   //motor = 1;
    //   Serial.println("30");
    // }

    // if (secondHand == 42)
    // {
    //   currentPalette = CloudColors_p;
    //   currentBlending = LINEARBLEND;
    //   Serial.println("42");
    // }

    // if (secondHand == 51)
    // {
    //   motor = 0;
    // }
    // if (secondHand == 62)
    // {
    //   SetupBlackPalette();
    //   currentBlending = NOBLEND;
    //   Serial.println("62");
    // }

    if ((secondHand == 29) && (secondHand == 31))
    { // Off everything
      digitalWrite(2, LOW);
      digitalWrite(3, LOW);
      digitalWrite(7, LOW);
      digitalWrite(8, LOW);

      SetupBlackPalette();
      currentBlending = NOBLEND;

      Serial.println("29");
    }

    // FLOWERS----------------------------------------------------------------------------------------
    if ((secondHand >= 32) && (secondHand <= 76))
    {
      SetupBlackPalette();
      if (secondHand % 4 == 1)
      {
        digitalWrite(2, HIGH);
        digitalWrite(3, LOW);
        digitalWrite(7, LOW);
        digitalWrite(8, LOW);
      }
      else if (secondHand % 4 == 2)
      {
        digitalWrite(3, HIGH);
        digitalWrite(2, LOW);
        digitalWrite(7, LOW);
        digitalWrite(8, LOW);
      }
      else if (secondHand % 4 == 3)
      {
        digitalWrite(7, HIGH);
        digitalWrite(3, LOW);
        digitalWrite(2, LOW);
        digitalWrite(8, LOW);
      }
      else if (secondHand % 4 == 0)
      {
        digitalWrite(8, HIGH);
        digitalWrite(3, LOW);
        digitalWrite(7, LOW);
        digitalWrite(2, LOW);
      }
    }
    if ((secondHand == 76))
    {
      digitalWrite(8, HIGH);
      digitalWrite(3, HIGH);
      digitalWrite(7, HIGH);
      digitalWrite(2, HIGH);
    }
    if (secondHand >= lastSecond)
    {
      running = false;
      initial = true;
      song = 0;

      digitalWrite(2, LOW);
      digitalWrite(3, LOW);
      digitalWrite(7, LOW);
      digitalWrite(8, LOW);

      SetupBlackPalette();
      currentBlending = NOBLEND;

      mp3.pause();
      setup();
    }
    prevSecond = secondHand;
  }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
  for (int i = 0; i < 16; i++)
  {
    currentPalette[i] = CHSV(random8(), 255, random8());
  }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid(currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;
}

void SetupBlackPalette()
{
  fill_solid(currentPalette, 16, CRGB::Black);
}

void SetupWhitePalette()
{
  fill_solid(currentPalette, 16, CRGB::White);
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
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
    CRGB::Gray, // 'white' is too bright compared to red and blue
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
    CRGB::Black};
