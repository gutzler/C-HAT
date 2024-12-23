/* I2C COMMUNICATIONS TEST SKETCH
 *  
 *  This sketch makes sure that the digital sensors the C-HAT 
 *  uses that communicate via I2C are working properly.  These
 *  sensors are the DS3231 RTC, the LSM6DS3TRC accelerometer, 
 *  and the LIS2MDL magnetometer.
 *  The accelerometer and magnetometer are located on the same
 *  Adafruit breakout board.
 *  
 *  It's good to troubleshoot that this communication is all
 *  working before you pour epoxy or try a deployment. Don't
 *  forget to reload the main C-HAT sketch onto the logger
 *  before returning it to service!
 *  
 *  The code here is taken largely from Adafruit example sketches
 *  and included for convenience.
 */


#include <Wire.h>
#include <Adafruit_Sensor.h>  //does the work to get raw outputs to usable units for us
#include <Adafruit_LSM6DS3TRC.h> //accelerometer
#include <Adafruit_LIS3MDL.h> //magnetometer
#include "RTClib.h"

/* Assign a unique ID to each sensor at the same time */
Adafruit_LSM6DS3TRC accel; //this is the accelerometer/gyro
Adafruit_LIS3MDL mag; //this is the magnetometer
RTC_DS3231 rtc; //clock

/*Set up sensor event framework*/
  //note: even though we're only showing the accel data (not gyro and temp) the LSM6 library seems to get angry if you don't grab them all at once?
  sensors_event_t accelEvent;
  sensors_event_t gyroEvent;
  sensors_event_t tempEvent;
  sensors_event_t magEvent; 


void setup(void)
{

  Serial.begin(9600);
  Serial.println("Accelerometer/magnetometer Test"); Serial.println("");
  
  if(!mag.begin_I2C())
  {
    /* There was a problem detecting the magnetometer ... check your connections */
    Serial.println("Ooops, no LIS3MDL detected ... Check your wiring!");
    while(1);
  }
  /* Initialise the sensor */
  if(!accel.begin_I2C())
  {
    /* There was a problem detecting the accelerometer ... check your connections */
    Serial.println("Ooops, no LSM3 detected ... Check your wiring!");
    while(1);
  }

}

void loop(void)
{
//Get new sensor events 
  accel.getEvent(&accelEvent, &gyroEvent, &tempEvent);
  mag.getEvent(&magEvent);
  
 float Pi = 3.14159;
  float heading = (atan2(magEvent.magnetic.y,magEvent.magnetic.x) * 180) / Pi;
  // Normalize to 0-360
  if (heading < 0)
  {
    heading = 360 + heading;
  }

/*Get time*/
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
  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("X: "); Serial.print(accelEvent.acceleration.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(accelEvent.acceleration.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(accelEvent.acceleration.z); Serial.print("  ");
  Serial.print(heading); Serial.println(" degrees");

  /* Note: You can also get the raw (non unified values) for */
  /* the last data sample as follows. The .getEvent call populates */
  /* the raw values used below. */
  //Serial.print("X Raw: "); Serial.print(accel.raw.x); Serial.print("  ");
  //Serial.print("Y Raw: "); Serial.print(accel.raw.y); Serial.print("  ");
  //Serial.print("Z Raw: "); Serial.print(accel.raw.z); Serial.println("");

  /* Delay before the next sample */
  delay(500);
}
