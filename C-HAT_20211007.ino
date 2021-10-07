/*Crustacean HAT Datalogger
 * (Heart and Activity Trackerr)
 * v2.4 2021-10-7
 * Ben Gutzler bgutzler@gmail.com
 * Uses Adafruit Feather Adalogger, Adafruit DS3231 Precision RTC and LSM303 compass/accelerometer
 * along with TMP36 temp sensor, and HW5P phototransistor
 * Much code borrowed shamelessly from Adafruit examples
 * Tilt compensation now included on compass, based on Pololu and ST Microelectronics guides
 * Outputs a shedload of raw-ish data of pitch and roll to help decode heading later, maybe, if we ever figure out the tilt issues

*/

/*USAGE: connect power, then hit RESET button to ensure everything is starting at same time
 * If solid LED: something is not right
 * LED should flash every 4 seconds to indicate writing to SD card
 * Check this BEFORE starting trial!
 * Not a bad idea to check RTC is set either
 */
 
#include <Wire.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LIS2MDL.h>
#include <Adafruit_LSM303_Accel.h>
#include "RTClib.h"


// LOGGING STUFF
// how many milliseconds between grabbing data and logging it. 1000 ms is once a second
  #define LOG_INTERVAL 200 // mills between entries (reduce to take more/faster data)

// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
// the last 10 reads if power is lost but it uses less power and is much faster!
  #define SYNC_INTERVAL 4000 // mills between calls to flush() - to write data to the card
  uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL   1 // echo data to serial port

#define IRPin 0    //IR transceiver signal to A0
#define TempPin 1 // thermistor signal to A1
#define LightPin 2 // phototransistor signal to A2
#define statusPin 13 //D13 is red LED
#define errorPin 8 //D13 is green LED

RTC_DS3231 RTC;
  const int chipSelect = 4; //SS pin on Featherlogger
  File logfile; // the logging file

 //defining how to read the uncalibrated data
  int IRReading;
  int TempReading;
  int LightReading;
  float accelX = 0.00;
  float accelY = 0.00;
  float accelZ = 0.00;
  float TempV;
  float TempC;

  // this should let me take more than one measurement per second and keep them straight
  unsigned long millis_now;
  unsigned long millis_prev = 0;
  int millis_diff;
  uint16_t seconds_now;
  uint16_t seconds_prev = 0;

/* Assign a unique ID to this sensor at the same time */
Adafruit_LIS2MDL mag = Adafruit_LIS2MDL(12345);
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);

  sensors_event_t a, m;

void error(char const *str)
{
  Serial.print("error: ");
  Serial.println(str);
  digitalWrite(errorPin, HIGH);
  digitalWrite(statusPin, HIGH);
   while (1){ //This loops to make the lights blink on and off to make it clear there's an issue
    delay(200);
    digitalWrite(errorPin, LOW);
    digitalWrite(statusPin, LOW);
    delay(200);
    digitalWrite(errorPin, HIGH);
    digitalWrite(statusPin, HIGH);
   }
}


void setup(void) 
{
  Serial.begin(9600);
 Serial.println();

  pinMode(IRPin, INPUT);
  pinMode(TempPin, INPUT);
  pinMode(LightPin, INPUT);

  // initialize the SD card
  pinMode(10, OUTPUT);

  // let it know the light pins are output only
  pinMode(8, OUTPUT);
  //pinMode(13, OUTPUT);
  /*pin 13 blinks most often, and setting to OUTPUT allows it to
   * dump a lot more current into lighting up the LED - it's fine without
   * and saves us some power!
   */
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");
  
  // create a new file
  char filename[] = "HATLOG00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldn't create file");
    digitalWrite(errorPin, HIGH);
  }

  // connect to RTC
  Wire.begin();  
  if (!RTC.begin()) {
    error("RTC failed");
    digitalWrite(errorPin, HIGH);
  }
 
  if(!mag.begin())
  {
    /* There was a problem detecting the LIS2MDL ... check your connections */
    error("Ooops, no LIS2MDL detected ... Check your wiring!");
    while(1);
  }
  if (!accel.begin()) {
    /* There was a problem detecting the LSM303 ... check your connections */
    error("Ooops, no LSM303 Accelerometer detected ... Check your wiring!");
    while (1)
      ;
  }
 
  //THIS IS WHAT GETS LOGGED
  logfile.println("datetime,heartIR,accelX,accelY,accelZ,hdg_uncomp,hdgcompensated,temp,light,roll,pitch,magX,magY,magZ");    
#if ECHO_TO_SERIAL
  Serial.println("datetime,heartIR,accelX,accelY,accelZ,hdg_uncomp,hdgcompensated,temp,light,roll,pitch,magX,magY,magZ");
#endif //ECHO_TO_SERIAL

}

// ACTUALLY RUNNING THIS
void loop(void) 
{ 
  DateTime now;

  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));

  digitalWrite(statusPin, LOW); //making LED blink when writing to SD
    //easier to see if it's still running or the battery is dead
  
  // fetch the time
  now = RTC.now();

  seconds_now = now.second();
  millis_now = millis();
  millis_diff = millis_now - millis_prev;
  if (millis_diff > 900) {
    millis_diff = 0;
  }
  if (seconds_prev != seconds_now) {
    seconds_prev = seconds_now;
    millis_prev = millis_now;
   }

  //Get analog sensor readings
  IRReading = analogRead(IRPin);
  TempReading = analogRead(TempPin);
  LightReading = analogRead(LightPin);

  //Convert analog temp input to degrees C
  //The next block of code is an expansion of this: TempC = ((TempReading*3.3)*100)-50;
    TempV = TempReading*3.3;
    TempC=TempV/1024; //equivalent to saying "TempC = previous value of TempC * 100"
    TempC-=.5; 
    TempC*=100;
  
  /* Get new sensor events */ 
 // sensors_event_t a, m;
  accel.getEvent(&a);
  mag.getEvent(&m);

   accelX = a.acceleration.x;
   accelY = a.acceleration.y;
   accelZ = a.acceleration.z;

/*
 * This big chunk of code does the tilt compensation.
 * It works together with a couple functions at the bottom to calculate pitch and roll.
 * I got the math from a post on the Pololu forums: https://forum.pololu.com/t/lsm303d-tilt-compensation-problem/11611
 * Kevin at Pololu wrote Heading2.ino which formed the base of the math.
 * I adapted some of the pitch and roll equations from ST Microelectronics app note DT0058 "Computing tilt measurement and tilt-compensated e-compass"
 * https://www.st.com/content/ccc/resource/technical/document/design_tip/group0/56/9a/e4/04/4b/6c/44/ef/DM00269987/files/DM00269987.pdf/jcr:content/translations/en.DM00269987.pdf
 * I could've used an existing Pololu library with the LSM303DLHC, but I'm trying to futureproof.
 */

  int32_t temp_mx = mag.raw.x;
  int32_t temp_my = mag.raw.y;
  int32_t temp_mz = mag.raw.z;  

 //Uncomment this next section if you want to calibrate each LSM303 chip - leaving it commented sticks with factory calibration
  // doing math for offsets from empirical calibration values - these values are from the testbed version at Wells 2020/2/6
     /*    min   max   avg
      * x  -407  177   -115
      * y  -405  271   -67
      * z  -540  134   -203
     */

  /* 
  int32_t temp_mx -= (-115);
  int32_t temp_my -=  (-67);
  int32_t temp_mz -= (-203);
 */
 
/*Heading calculations*/
//h1 = uncompensated Adafruit code
float h1 = (atan2(mag.raw.y,mag.raw.x) * 180) / PI;
 if (h1 < 0) h1 += 360;

//hcomp = from https://www.instructables.com/id/Tilt-Compensated-Compass/
float Mag_roll = (atan2(a.acceleration.y, a.acceleration.z)*(180/PI));
float Mag_pitch = (atan(-a.acceleration.x/((a.acceleration.y * sin(atan2(a.acceleration.y, a.acceleration.z)) + a.acceleration.z * cos(atan2(a.acceleration.y, a.acceleration.z))))))*(180/PI);
float Xhorizontal = accelX*cos(Mag_pitch) + accelY*sin(Mag_roll)*sin(Mag_pitch) - accelZ*cos(Mag_roll)*sin(Mag_pitch);
float Yhorizontal = accelY*cos(Mag_roll) + accelZ*sin(Mag_roll);
float hcomp = atan2(Yhorizontal,Xhorizontal) * 180 / PI;
  if (hcomp < 0) hcomp += 360;

//end of the tilt compensation and heading calculation bit

  
// log time and data
  logfile.print('"');
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print(".");
  logfile.print(millis_diff);
  logfile.print('"');
  logfile.print(", ");    
  logfile.print(IRReading); //to get from IR reading to voltage: IRreading*3.3/1024
  logfile.print(", ");    
  logfile.print(accelX);
  logfile.print(", ");
  logfile.print(accelY);
  logfile.print(", ");
  logfile.print(accelZ);
  logfile.print(", ");    
  logfile.print(h1);
  logfile.print(", "); 
  logfile.print(hcomp); //this is VERY noisy - will want to smooth afterwards
  logfile.print(", ");
  logfile.print(TempC); //TMP36 outputs straight to ºC
  logfile.print(", ");    
  logfile.print(LightReading); //to get from reading to voltage: reading*3.3/1024
  logfile.print(", ");    
  logfile.print(Mag_roll);
  logfile.print(", ");
  logfile.print(Mag_pitch);
  logfile.print(", ");
  logfile.print(temp_mx);
  logfile.print(", ");
  logfile.print(temp_my);
  logfile.print(", ");
  logfile.print(temp_mz);
  logfile.println();
#if ECHO_TO_SERIAL
  Serial.print('"');
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print(".");
  Serial.print(millis_diff);
  Serial.print('"');
  Serial.print(", ");   
  Serial.print(IRReading);
  Serial.print(", ");
  Serial.print(accelX,4);
  Serial.print(", ");
  Serial.print(accelY,4);
  Serial.print(", "); 
  Serial.print(accelZ,4);
  Serial.print(", ");
  Serial.print(h1);
  Serial.print(", "); 
  Serial.print(hcomp);
  Serial.print(", ");    
  Serial.print(TempC);
  Serial.print(", ");
  Serial.print(LightReading);
  Serial.print(", "); 
  Serial.print(Mag_roll);
  Serial.print(", "); 
  Serial.print(Mag_pitch);
  Serial.print(", ");
  Serial.print(temp_mx);
  Serial.print(", ");
  Serial.print(temp_my);
  Serial.print(", ");
  Serial.print(temp_mz);
 Serial.println();
#endif //ECHO_TO_SERIAL

 
// Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  logfile.flush();
  digitalWrite(statusPin, HIGH);
}
