/* I2C COMMUNICATIONS TEST SKETCH
 *  
 *  This sketch makes sure that the digital sensors the C-HAT 
 *  uses that communicate via I2C are working properly.  These
 *  sensors are the DS3231 RTC, the LSM303AGR accelerometer, 
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
#include <Adafruit_Sensor.h>
#include <Adafruit_LIS2MDL.h>
#include <Adafruit_LSM303_Accel.h>
#include "RTClib.h"
RTC_DS3231 rtc;

/* Assign a unique ID to this sensor at the same time */
Adafruit_LIS2MDL mag = Adafruit_LIS2MDL(12345);
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);


void setup(void)
{

  Serial.begin(9600);
  Serial.println("Accelerometer/magnetometer Test"); Serial.println("");
  
  if(!mag.begin())
  {
    /* There was a problem detecting the LIS2MDL ... check your connections */
    Serial.println("Ooops, no LIS2MDL detected ... Check your wiring!");
    while(1);
  }
  /* Initialise the sensor */
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }

}

void loop(void)
{
  /* Get new sensor events */
  sensors_event_t accelEvent; 
  sensors_event_t magEvent; 
  
  accel.getEvent(&accelEvent);
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
