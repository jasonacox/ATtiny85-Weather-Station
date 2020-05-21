# ATtiny85 Weather Station
This project is an ATtiny85 based mini Weather Station that displays temperature, humidity, and pressure via four 7-segment LED displays.  

## Circuit
Components:
* ATiny85 Microcontroller
* BME-280 Sensor (Temperature, Pressure, Humidity)
* 74HC595 8-bit Shift Register (Qty 4)
* 7-Segement LED Display (Qty 4)
* 0.1uF Ceramic Capacitor (Qty 2)
* 100uF Electrolytic Capacitor
* 5V Power Supply

## Requirements
This sketch requires a version of the Wire library that is compatible with the ATtiny85 for the I2C communciation to work with the BME-280 sensor.  I used the [ATTinyCore arduino core by Spence Konde](https://github.com/SpenceKonde/ATTinyCore) which has a version of the Wire library that works with the ATtiny85 and was able to use the standard *Adafruit_BME280* library to pull data from the BME-280.  You can install *ATTinyCore* putting the board manager URL in the Arduino IDE preferences: `http://drazzy.com/package_drazzy.com_index.json`  
Set the board to the ATtiny85 chip at 1Mhz (internal).

## Programming Notes:
I2C communcation with BME-280 uses pins PB0/SDA and PB2/SCL. If you use the *Tiny AVR Programmer* from Sparkfun
or something simliar it drives an LED on PB0 which will interfear with I2C communcation. You will need to remove
the chip from the programmer after uploading to get it to work in the circuit.

## Memory Warning
This sketch uses nearly all of the ATtiny85 program storage space (8174 bytes) so you may get an overflow error if the libraries change or you add any code.

