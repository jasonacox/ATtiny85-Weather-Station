#pragma GCC optimize ("-Os")
/*
  ATtiny85 Temperature + Humidity + Barometer on TM1637 LED Display

  Author: Jason A. Cox - @jasonacox

  Date: 7 Mar 2021

  Components:
      ATiny85 Microcontroller
      BME-280 Sensor (Temperature, Pressure, Humidity)
      TM1637 Display Module
      5V Power Supply

  Requirement: This sketch requires a version of the Wire library that works with the ATtiny85, e.g.
      ATTinyCore by Spence Konde board manager URL http://drazzy.com/package_drazzy.com_index.json
      ATtiny85 chip at 8Mhz (internal)

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
      [_970] - Pressure in hPa with prefix animation for rising for falling
      [ 21c] - Temperature in degree C (positive & negative)

  ATtiny85 Pinout:
                             +-------+
                            -|1  T  8|- V    (5V)
      TM1637  (CLKpin)  PB3 -|2  i  7|- PB2  (SCLpin)  BME280
      TM1637  (DIOpin)  PB4 -|3  n  6|- PB1
              (GND)       G -|5  y  5|- PN0  (SDApin)  BME280
                             +-------+   
*/

/* Includes */

// BME280 Sensor Library
#include <Adafruit_Sensor.h>
#include <Tiny_BME280.h>

// Include TM1637Display Library
#include <Arduino.h>
#include <TM1637TinyDisplay.h>
// #include "segment.h"

// Pins definitions for ATtiny85
#define SDApin PB0      // Pin connected to SDA   of BME280
#define SCLpin PB2      // Pin connected to SCL   of BME280 
#define CLKpin PB3      // Pin connected to CLK   of TM1637
#define DIOpin PB4      // Pin connected to DIO   of TM1637

/* Pressure Change */
#define STEADY 0
#define RISE 1
#define FALL 2

/* Globals */
TM1637TinyDisplay display(CLKpin, DIOpin);  // Display
Tiny_BME280 bme;                        // Sensor
// circular buffer to store pressure trend for last ~15m
int cbuf[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte cread = 1;  // index
int state = 0;
uint8_t top[] = { SEG_A, 0, 0, 0 };

/*
   ************************ SETUP ************************
*/
void setup() {

  // Display
  display.setBrightness(0x0f); // set the brightness to 100 %
  delay(10);

  // Initialize BME280 - Temp, Humidity and Pressure sensor
  while (bme.begin(0x76) == false) {
    display.showString("----");
    delay(250);
  }

}

/*
    ************************ MAIN LOOP ************************
*/
void loop() {

  int value;
  byte pressure;

  delay(5000);  // 5s delay

  switch (state) {
    case 0:
    case 1:
    case 2:
    case 3:
      value = (int) (bme.readTemperature() * 1.8) + 32; // in Fahrenheit
      display.showString("\xB0", 1, 3);        // Degree Mark
      display.showNumber(value, false, 3, 0);
      break;
    case 4:
      value = (int) (bme.readHumidity());
      display.showString("r", 1, 3);          // relative humidity
      display.showNumber(value, false, 3, 0);
      break;
    case 5:
      value = (int) (bme.readPressure() / 10 );                 // in dekapascals
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
      display.showNumber(value, false, 3, 1);
      // Pressure Animation
      switch (pressure) {
        case STEADY:
          delay(250 * 4 * 2);
          break;
        case RISE:
          for (int x = 1; x < 3; x++) {
            display.showString("_", 1, 0);
            delay(250);
            display.showString("-", 1, 0);
            delay(250);
            display.setSegments(top, 1, 0);
            delay(250);
            display.showString(" ", 1, 0);
            delay(250);
          }
          break;
        case FALL:
          for (int x = 1; x < 3; x++) {
            display.setSegments(top, 1, 0);
            delay(250);
            display.showString("-", 1, 0);
            delay(250);
            display.showString("_", 1, 0);
            delay(250);
            display.showString(" ", 1, 0);
            delay(250);
          }
          break;
      }
      break;

    case 6:
      value = (int) (bme.readTemperature());     // in Celsius
      display.showString("c", 1, 3);             //c Mark
      display.showNumber(value, false, 3, 0);
      break;

    default:
      state = 0;
  }
  state++;
}
