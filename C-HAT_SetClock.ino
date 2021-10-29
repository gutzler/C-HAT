/* This sketch is for setting the Real Time Clock module.
 *  
 *  Run it once, then reload the main C-HAT sketch onto the logger.
 *  The chip will remember the time unless its battery runs out, or it
 *  gets fried somehow. You will need to run it again after time changes
 *  (it's not smart enough to realize DST is a thing).
 *  
 *  Running this sketch is a useful test that the clock is working and
 *  communicating as it should.
 *  
 * The code here is "borrowed" and modified slightly from RTClib,
 * and included for convenenience.
 */


// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;


void setup () {

  Serial.begin(9600);

  delay(3000); // wait for console opening

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }


// This line sets the RTC to the date & time this sketch was compiled
 rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 23, 30, 0));
  
}

void loop () {
    DateTime now = rtc.now();
    
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
        delay(3000);
}
