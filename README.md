# C-HAT
Schematics and code for C-HAT loggers

Open-source, homebuilt dataloggers designed initially for work on lobsters.

C-HAT: Crustacean Heart and Activity Tracker

Arduino-based with substantial use of code snippets from Adafruit example libraries (among others).

## Basic usage
Once the logger is built, set the clock with C-HAT_SetClock.ino, test the analog inputs with C-HAT_TestAnalogInputs.ino and the digital sensors with C-HAT_Accel_Mag_Test.ino, and check the SD card with the CardInfo example from the SD library. If all that works out, load the main code and go do some science!

### Important tips
- The heart sensor output is *NOT* the heart rate. It is raw readings from the analog-digital converter (ADC) on the board, so its values range from 0-1023 (10 bit logic). Plot these values out and use a peak finding algorithm to determine the rate of heartbeats. I use a (very expensive) program called LabChart, which is designed for physiological experiments; however, I've heard Python works well for that too, and R likely also has ways of doing it.
To go from ADC values to true voltage, divide the ADC output by the total number of possible values (1024), then multiply that value by the voltage of the board (3.3V). So, for ADC value of 500: (500/1024)x3.3V = 1.61V.
- The Real Time Clock chip is not smart enough to figure out Daylight Savings Time. Remember to reset it each time there's a time change!

### Helpful resources
The C-HAT code assumes a basic familiarity with Arduino boards, the Arduino language, and soldering together basic circuits. If you're just starting out, the [main Arduino website](https://www.arduino.cc/en/Guide) has many useful links and tips, including [examples that are excellent for practicing and learning](https://docs.arduino.cc/built-in-examples/), and their [language reference page](https://www.arduino.cc/reference/en/) is definitely worth bookmarking! Sparkfun has some [good beginner tutorials](https://learn.sparkfun.com/tutorials/what-is-an-arduino), including an [introduction to analog-digital converters](https://learn.sparkfun.com/tutorials/analog-to-digital-conversion) like those used for the heart sensor in the C-HAT.

Adafruit also has excellent tutorials and libraries for use of their boards.
The main "brains" of this setup is a pair of two Adafruit Feather boards, stacked using their short header pins. The main board is an [Adafruit 32u4 Featherlogger](https://learn.adafruit.com/adafruit-feather-32u4-adalogger) is Arduino-programmable and features the microSD card slot, while the [DS3231 Precision RTC Featherwing](https://learn.adafruit.com/adafruit-ds3231-precision-rtc-breakout/) stacks under the main board and requires no added wiring for its I2C communications (the header pins take care of that). The [LSM303AGR accelerometer](https://learn.adafruit.com/lsm303-accelerometer-slash-compass-breakout/downloads) also communicates over I2C - I like using Adafruit breakout boards because the Adafruit Sensor library package allows the output to be immediately displayed in standard units. Becoming familiar with using these boards is probably the best way to get ready to set up a C-HAT.
