# C-HAT
Schematics and code for C-HAT loggers

Open-source, homebuilt dataloggers designed initially for work on lobsters.

C-HAT: Crustacean Heart and Activity Tracker

Arduino-based with substantial use of code snippets from Adafruit example libraries (among others).
![Gutzler and Watson Highlight copy-1600](https://user-images.githubusercontent.com/90336383/162221387-6e27c7bb-1556-41b3-820d-707b5e8ba9bb.jpg)

## Basic usage
Once the logger is built, set the clock with C-HAT_SetClock.ino, test the analog inputs with C-HAT_TestAnalogInputs.ino and the digital sensors with C-HAT_Accel_Mag_Test.ino, and check the SD card with the CardInfo example from the SD library. If all that works out, load the main code and go do some science!

### Important tips
- The heart sensor output is *NOT* the heart rate. It is raw readings from the analog-digital converter (ADC) on the board, so its values range from 0-1023 (10 bit logic). Plot these values out and use a peak finding algorithm to determine the rate of heartbeats. I use a (very expensive) program called LabChart, which is designed for physiological experiments; however, I've heard Python works well for that too, and R likely also has ways of doing it.
To go from ADC values to true voltage, divide the ADC output by the total number of possible values (1024), then multiply that value by the voltage of the board (3.3V). So, for ADC value of 500: (500/1024)x3.3V = 1.61V.
- The Real Time Clock chip is not smart enough to figure out Daylight Savings Time. Remember to reset it each time there's a time change!

### The Movement Index
To simplify analyses, I like to condense the triaxial acceleration values into a single number I refer to as the Movement Index. This is not done on the main board due to limited computing power, and I do it after the fact in other programs. The calculation is done by differentiating each axis of acceleration, squaring those values, combining them, and then taking the square root of the whole thing:

  <img src="https://latex.codecogs.com/svg.image?MI&space;=&space;\sqrt{(\textbf{differentiate}(accelX))^{2}&plus;(\textbf{differentiate}(accelY))^{2}&plus;(\textbf{differentiate}(accelZ))^{2}}" title="https://latex.codecogs.com/svg.image?MI = \sqrt{(\textbf{differentiate}(accelX))^{2}+(\textbf{differentiate}(accelY))^{2}+(\textbf{differentiate}(accelZ))^{2}}" />

I use the Savitzy-Golay method and a three-sample window for the differentiation. At a 5 Hz sampling rate, this has the effect of magnifying short-term changes in acceleration (three samples is 0.6 seconds), more so than the longer windows typically used in the ODBA/VeDBA calculations often used in other accelerometer literature.

#### Implementing the MI in R:
```
library(prospectr)
...
#differentiate each axis using the Savitzky-Golay method
Xdiff <- savitzkyGolay(X=df1$accelX,m=1,p=1,w=3)
Ydiff <- savitzkyGolay(X=df1$accelY,m=1,p=1,w=3)
Zdiff <- savitzkyGolay(X=df1$accelZ,m=1,p=1,w=3)
#construct MI
NAobj <- as.vector("NA") #need this to build proper vectors
MI <- sqrt(Xdiff^2+Ydiff^2+Zdiff^2) #calculate MI
MI <- as.numeric(c(NAobj,MI,NAobj)) #the two added NA values are inserted because of the three sample window for differentiation
```

#### Implementing the MI in Python:
```
from numpy import square, sqrt
from scipy.signal import savgol_filter 
...
#differentiate each axis using the Savitzky-Golay method
df['dX'] = savgol_filter(df['accelX'],3,1,deriv=1)
df['dY'] = savgol_filter(df['accelY'],3,1,deriv=1)
df['dZ'] = savgol_filter(df['accelZ'],3,1,deriv=1)
#construct MI
df['MI'] = sqrt(square(df['dX'])+square(df['dY'])+square(df['dZ']))
```

### Helpful resources
The C-HAT code assumes a basic familiarity with Arduino boards, the Arduino language, and soldering together basic circuits. If you're just starting out, the [main Arduino website](https://www.arduino.cc/en/Guide) has many useful links and tips, including [examples that are excellent for practicing and learning](https://docs.arduino.cc/built-in-examples/), and their [language reference page](https://www.arduino.cc/reference/en/) is definitely worth bookmarking! Sparkfun has some [good beginner tutorials](https://learn.sparkfun.com/tutorials/what-is-an-arduino), including an [introduction to analog-digital converters](https://learn.sparkfun.com/tutorials/analog-to-digital-conversion) like those used for the heart sensor in the C-HAT.

Adafruit also has excellent tutorials and libraries for use of their boards.
The main "brains" of this setup is a pair of two Adafruit Feather boards, stacked using their short header pins. The main board is an [Adafruit 32u4 Featherlogger](https://learn.adafruit.com/adafruit-feather-32u4-adalogger) is Arduino-programmable and features the microSD card slot, while the [DS3231 Precision RTC Featherwing](https://learn.adafruit.com/adafruit-ds3231-precision-rtc-breakout/) stacks under the main board and requires no added wiring for its I2C communications (the header pins take care of that). The [LSM303AGR accelerometer](https://learn.adafruit.com/lsm303-accelerometer-slash-compass-breakout/downloads) also communicates over I2C - I like using Adafruit breakout boards because the Adafruit Sensor library package allows the output to be immediately displayed in standard units. Becoming familiar with using these boards is probably the best way to get ready to set up a C-HAT.
