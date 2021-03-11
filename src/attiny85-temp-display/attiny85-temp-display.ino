#pragma GCC optimize ("-Os")
/*
  ATtiny85 Temperature + Humidity + Barometer 7-Segment LED Display

  Author: Jason A. Cox - @jasonacox

  Date: 17 May 2020

  Components:
      ATiny85 Microcontroller
      BME-280 Sensor (Temperature, Pressure, Humidity)
      74HC595 8-bit Shift Register (Qty 4)
      7-Segment LED Display (Qty 4)
      0.1uF Ceramic Capacitor (Qty 2)
      100uF Electrolytic Capacitor
      5V Power Supply

  Requirement: This sketch requires a version of the Wire library that works with the ATtiny85, e.g.
      ATTinyCore by Spence Konde board manager URL http://drazzy.com/package_drazzy.com_index.json
      ATtiny85 chip at 1Mhz (internal)

  Programming Notes:
      I2C communcation with BME-280 uses PB0/SDA and PB2/SCL. If you use the Tiny AVR Programmer from Sparkfun
      it drives an LED on PB0 and will interfear with I2C communcation. You will need to remove
      the chip from the programmer after uploading to get it to work in the circuit.

      This sketch uses minimized version of the Adafruit_BME280 Library that excludes SPI supports and other
      advanced features to reduce the amount of PROGMEM space that is required. Library is available
      here: https://github.com/jasonacox/Tiny_BME280_Library

  Display:
      [ 70'] - Temperature in degree F (positive & negative)
      [ 24r] - Relative Humidity
      [_970] - Pressure in hPa with prefix for rising - or falling _
      [ 21c] - Temperature in degree C (positive & negative)
*/

/* Includes */
#include <Adafruit_Sensor.h>
#include <Tiny_BME280.h>

/* ATtiny85 Pins */
#define SDApin 0      // Pin connected to SDA   of BME280
#define SCLpin 2      // Pin connected to SCL   of BME280 
#define dataPin 1     // Pin connected to DS    of 74HC595
#define latchPin 3    // Pin connected to ST_CP of 74HC595
#define clockPin 4    // Pin connected to SH_CP of 74HC595

/* Pressure Change */
#define STEADY 0
#define RISE 1
#define FALL 2

/* Global variables */
byte state = 0;       // State flag to trigger units change

Tiny_BME280 bme; // I2C

/* Set up 7-segment LED Binary Data

    |--A--|
    F     B
    |--G--|
    E     C
    |--D--|
           H - Decimal

    0b00000000
      ABCDEFGH
*/
const static byte numArray[]  = {
  0b11111100, // 0
  0b01100000, // 1
  0b11011010, // 2
  0b11110010, // 3
  0b01100110, // 4
  0b10110110, // 5
  0b10111110, // 6
  0b11100000, // 7
  0b11111110, // 8
  0b11110110, // 9
  0b11111101, // 0. (Decimal point)
  0b01100001, // 1.
  0b11011011, // 2.
  0b11110011, // 3.
  0b01100111, // 4.
  0b10110111, // 5.
  0b10111111, // 6.
  0b11100001, // 7.
  0b11111111, // 8.
  0b11110111, // 9.
  0b00000001, // . (Decimal)
  0b11000110, // . (Degree Mark)
  0b00000000, // blank           (index 22)
  0b00001010, // r               (index 23)
  0b00101110, // h               (index 24)
  0b00010000, // falling v       (index 25)
  0b10000000, // rising  ^       (index 26)
  0b00000010, // negative   -    (index 27)
  0b01101110,  // H              (index 28)
  0b00011010   // c              (index 29)
}; // All Off

// circular buffer to store pressure trend for last ~15m
int cbuf[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte cread = 1;  // index

/*
   SETUP
*/
void setup() {

  // Set up pins for driving the 74HC595 shift registers / LEDs
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);

  // Initialize BME280 - Temp, Humidity and Pressure sensor
  while (bme.begin(0x76) == false) {
    displayRaw(0b10011110, 0b00001010, 0b00001010, 0b00000000);  // Err
    delay(250);
  }

  // Set display with startup image
  displayRaw(0b10010000, 0b10010000, 0b10010000, 0b10010000);

}

/*
    MAIN LOOP
*/
void loop() {
  int value;
  byte a, b, c, d, pressure;

  delay(5000);  // 5s delay

  switch (state) {
    case 7:
      state = 0;
    case 0:
    case 1:
    case 2:
    case 3:
      value = (int) (bme.readTemperature() * 1.8 * 10) + 320; // in Fahrenheit
      // value = (int) (bme.readTemperature() * 10);          // in Celsius
      break;
    case 4:
      value = (int) (bme.readHumidity() * 10);
      break;
    case 5:
      value = (int) (bme.readPressure() / 10 );                 // in dekapascals
      // value = (int) (bme.readPressure() * 0.0002953 * 100);  // in inHg

      // Determine if pressure is rising or falling by looking back 12m ago
      pressure = STEADY;
      if (cbuf[cread] < value) {
        pressure = RISE;
      }
      if (cbuf[cread] > value) {
        pressure = FALL;
      }
      // Use circular buffer to store pressure trend
      cbuf[(cread + 31) % 32] = value;
      cread = (cread + 1) % 32;
      value = (int)(value / 10);  // convert to hPa (hectopascal)
      break;
    case 6:
      value = (int) (bme.readTemperature() * 10);          // in Celsius
      break;
  }

  // Determine Digits on Display [abcd]
  a = 0;
  if (value < 0) {
    a = 27; // negative sign -
    value = -value; // flip positive
  }
  d = value % 10;  // ones
  value = (value - d) / 10;
  c = value % 10; // tens
  value = (value - c) / 10;
  b = value % 10; // hundreds
  value = (value - b) / 10;
  if (!a) {
    a = value % 10; // thousands
  }

  // Remove leading zeros
  if (a == 0) {
    a = 22; // space
    if (b == 0) {
      b = 22;  // space
    }
  }

  // Temperature F - add degree mark
  if (state < 4) {
    d = 21; // degree mark
  }

  // Temperature C - add c mark
  if (state == 6) {
    d = 29; // Add c suffix
  }

  // Humidity - rh %
  if (state == 4) {
    d = 23; // Add r suffix - relative humidity
  }

  // Send digits to display
  displayRaw(numArray[a], numArray[b], numArray[c], numArray[d]);

  // Pressure Animation
  if (state == 5) {
    if (a == 22) { // if < 1000hPa add rising/falling indicator prefix
      if (pressure == RISE) { // rising
        for (int x = 1; x < 3; x++) {
          displayRaw(0b00010000,  numArray[b], numArray[c], numArray[d]);
          delay(250);
          displayRaw(0b00000010,  numArray[b], numArray[c], numArray[d]);
          delay(250);
          displayRaw(0b10000000,  numArray[b], numArray[c], numArray[d]);
          delay(250);
          displayRaw(0b00000000,  numArray[b], numArray[c], numArray[d]);
          delay(250);
        }
      }
      if (pressure == STEADY) { // flat
        delay(250 * 4 * 2);
      }
      if (pressure == FALL) { // falling
        for (int x = 1; x < 3; x++) {
          displayRaw(0b10000000,  numArray[b], numArray[c], numArray[d]);
          delay(250);
          displayRaw(0b00000010,  numArray[b], numArray[c], numArray[d]);
          delay(250);
          displayRaw(0b00010000,  numArray[b], numArray[c], numArray[d]);
          delay(250);
          displayRaw(0b00000000,  numArray[b], numArray[c], numArray[d]);
          delay(250);
        }
      }
    }
  }

  state++;

}

/*
   Send digits out to 7-segment LEDs
*/
void displayRaw (byte a, byte b, byte c, byte d) {
  digitalWrite(latchPin, 0);
  sendOut(a);
  sendOut(b);
  sendOut(c);
  sendOut(d);
  digitalWrite(latchPin, 1);
}

/*
   Send myDataOut to 8 bit register

   This sifts 8 bits out MSB first on the rising edge of the clock
*/
void sendOut(byte myDataOut) {
  int i = 0;
  int pinState;

  // Clear data and clock output
  digitalWrite(dataPin, 0);
  digitalWrite(clockPin, 0);

  // Send each bit in the byte myDataOut
  for (i = 7; i >= 0; i--)  {
    digitalWrite(clockPin, 0);
    if ( myDataOut & (1 << i) ) {
      pinState = 1;
    }
    else {
      pinState = 0;
    }
    // Send pinState
    digitalWrite(dataPin, pinState);
    // Shift register with Clock
    digitalWrite(clockPin, 1);
    // Zero out data pin
    digitalWrite(dataPin, 0);
  }
  digitalWrite(clockPin, 0);
}
