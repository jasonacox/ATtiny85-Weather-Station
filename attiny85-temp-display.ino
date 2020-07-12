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

      This sketch uses nearly all of the ATtiny85 program storage space (8k bytes) so you
      may get an overflow error if the libraries change.

  Display:
      [ 70'] - Temperature in degree (positive & negative)
      [ 24r] - Relative Humidity
      [_970] - Pressure in hPa with prefix for rising - or falling _
*/

/* Includes */
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

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

Adafruit_BME280 bme; // I2C

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
  0b01101110  // H               (index 28)
}; // All Off

// circular buffer to store pressure trend for last ~12m
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
  bool status = bme.begin(0x76);

  // Set display with startup image
  digitalWrite(latchPin, 0);
  sendOut(0b10010000);
  sendOut(0b10010000);
  sendOut(0b10010000);
  sendOut(0b10010000);
  digitalWrite(latchPin, 1);
}

/*
    MAIN LOOP
*/
void loop() {
  int value;
  byte a, b, c, d, pressure;
  bool neg;

  delay(5000);  // 5s delay

  switch (state) {
    case 5:
      state = 0;
    case 0:
    case 1:
    case 2:
      value = (int) (bme.readTemperature() * 1.8 * 10) + 320; // for Fahrenheit
      // value = (int) (bme.readTemperature() * 10);          // for Celsius
      break;
    case 3:
      value = (int) (bme.readHumidity() * 10);
      break;
    case 4:
      value = (int) (bme.readPressure() );               // for Pa
      // value = (int) (bme.readPressure() * 0.0002953 * 100);  // for inHg

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
      value = value / 100;  // convert to hPa
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

  // Temperature - add degree mark
  if (state < 3) {
    d = 21; // degree mark
  }

  // Humidity - rh %
  if (state == 3) {
    d = 23; // Add r suffix - relative humidity
  }

  // Pressure - hPa
  if (state == 4) {
    if (a == 22) { // if < 1000hPa add rising/falling indicator prefix
      // if pressure==STEADY leave blank
      if (pressure==RISE) a = 26;  // rising sign
      if (pressure==FALL) a = 25;  // falling sign
    }

  }

  // Send digits to display
  digitalWrite(latchPin, 0);
  sendOut(numArray[a]);
  sendOut(numArray[b]);
  sendOut(numArray[c]);
  sendOut(numArray[d]);
  digitalWrite(latchPin, 1);

  state++;

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