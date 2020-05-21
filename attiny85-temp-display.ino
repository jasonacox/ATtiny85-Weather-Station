/*
  ATtiny85 Weather Station
  Temperature + Humidity + Barometer via 7-Segment LED Display

  Author: Jason A. Cox - @jasonacox

  Date: 17 May 2020

  Components:
      ATiny85 Microcontroller
      BME-280 Sensor (Temperature, Pressure, Humidity)
      74HC595 8-bit Shift Register (Qty 4)
      7-Segement LED Display (Qty 4)
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
      
      This sketch uses nearly all of the ATtiny85 program storage space (8174 bytes) so you
      may get an overflow error if the libraries change.
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

/* Global variables */
bool units;           // True = SI, False = Imperial
int state;            // State flag to trigger units change

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
static byte numArray[] = {
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
  0b00000000, // blank            (index 22)
  0b01101110,  // H                (index 23)
  0b00101110  // h                (index 24)
}; // All Off

/*
   SETUP
*/
void setup() {
  state = 0;

  // Set up pins for driving the 74HC595 shift registers / LEDs
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);

  // Initialize BME280 - Temp, Humidity and Pressure sensor
  bool status = bme.begin(0x76);
  
  // Clear displays
  digitalWrite(latchPin, 0);
  sendOut(dataPin, clockPin, 0b00000000);
  sendOut(dataPin, clockPin, 0b00000000);
  sendOut(dataPin, clockPin, 0b00000000);
  sendOut(dataPin, clockPin, 0b00000000);
  digitalWrite(latchPin, 1);
  delay(100);
}

/*
    MAIN LOOP
*/
void loop() {
  int value, mswait;
  int a, b, c, d;

  switch (state) {
    case 0:
      value = (int) (bme.readTemperature() * 1.8 * 10) + 320; // Fahrenheit
      // value = (int) (bme.readTemperature() * 10);          // Celcius
      state = 1;
      break;
    case 1:
      value = (int) (bme.readPressure() / 100.0);
      state = 2;
      break;
    case 2:
      value = (int) (bme.readHumidity());
      state = 0;
      break;
  }

  // Determine Digits
  d = value % 10;  //ones
  value = (value - d) / 10;
  c = value % 10; // tens
  value = (value - c) / 10;
  b = value % 10; // hundreds
  value = (value - b) / 10;
  a = value % 10; // thousands

  // Remove leading zeros
  if (a == 0) {
    a = 22;
    if (b == 0) {
      b = 22;
    }
  }

  // Temperature in F'
  if (state == 1) {
    d = 21; // Add degree mark
  }

  // Humiditiy in %
  if (state == 0) {
    a = 23; // Add H prefix
  }

  // Pressure in hPa
  if (state == 2) {
    if (a == 22) {
      a = b; // if < 1000hPa shift digits left
      b = c;
      c = d;
      d = 24; // and add "h" suffix
    }
    delay(5000);  // add 5s delay before replacing temp display
  }

  // Send digits to display
  digitalWrite(latchPin, 0);
  sendOut(dataPin, clockPin, numArray[a]);
  sendOut(dataPin, clockPin, numArray[b]);
  sendOut(dataPin, clockPin, numArray[c]);
  sendOut(dataPin, clockPin, numArray[d]);
  digitalWrite(latchPin, 1);

  delay(3000);  // 3s delay

}

/*
   Send myDataOut to 8 bit register

   This sifts 8 bits out MSB first on the rising edge of the clock
*/
void sendOut(int myDataPin, int myClockPin, byte myDataOut) {
  int i = 0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);

  // Clear data and clock output
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);
  // Send each bit in the byte myDataOut

  for (i = 7; i >= 0; i--)  {
    digitalWrite(myClockPin, 0);
    if ( myDataOut & (1 << i) ) {
      pinState = 1;
    }
    else {
      pinState = 0;
    }
    // Send pinState
    digitalWrite(myDataPin, pinState);

    // Shift register with Clock
    digitalWrite(myClockPin, 1);
    // Zero out data pin
    digitalWrite(myDataPin, 0);
  }
  digitalWrite(myClockPin, 0);
}
