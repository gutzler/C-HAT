/*Lobster HAT Datalogger
 * (Heart and Activity Tracker - now with Hall Effect sensor)
 * v3.2 2024-12-23
 * Ben Gutzler bgutzler@gmail.com
 * Uses Adafruit Feather Adalogger, Adafruit DS3231 Precision RTC and LSM6DS3TRC+LIS3MDL IMU board
 * along with TMP36 temp sensor, phototransistor, and Hall effect sensor
 * Much code borrowed shamelessly from Adafruit examples
 * Outputs a shedload of raw-ish data of pitch and roll to help decode heading later via tilt compensation
 * (done in post-processing for power and speed reasons)
 * Removed some of the things that might not be as useful to save memory space
*/

/*USAGE: connect power, then hit RESET button to ensure everything is starting at same time
 * If solid LED: something is not right
 * LED should flash every 4 seconds to indicate writing to SD card
 * Check this BEFORE starting trial!
 * Not a bad idea to check RTC is set either
 */
 
#include <Wire.h>
#include <SD.h>
#include <Adafruit_Sensor.h>  //does the work to get raw outputs to usable units for us
#include <Adafruit_LSM6DS3TRC.h> //accelerometer
#include <Adafruit_LIS3MDL.h> //magnetometer
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

#define IRPin 0    //IR transceiver signal to A0
#define TempPin 1 // thermistor signal to A1
#define LightPin 2 // phototransistor signal to A2
#define HallPin 3 //Hall Effect signal to A3
#define statusPin 13 //D13 is red LED
#define errorPin 8 //D13 is green LED

  const int chipSelect = 4; //SS pin on Featherlogger
  File logfile; // the logging file

 //defining how to read the uncalibrated data
  int IRReading;
  int TempReading;
  int LightReading;
  int HallReading;
  float accelX, accelY, accelZ; 
  float gyro_pitch, gyro_roll, gyro_yaw;
  float magX, magY, magZ;
  float TempV;
  float TempC;

  //using these to avoid repeated calls to check the RTC
  uint16_t timeY;
  uint16_t timeM;
  uint16_t timeD;
  uint16_t timeHH;
  uint16_t timeMM;

  // this should let me take more than one measurement per second and keep them straight
  unsigned long millis_now;
  unsigned long millis_prev = 0;
  int millis_diff;
  uint16_t seconds_now;
  uint16_t seconds_prev = 0;

/* Assign a unique ID to each sensor at the same time */
Adafruit_LSM6DS3TRC accel; //this is the accelerometer/gyro - also gets temp 'cause why not
Adafruit_LIS3MDL mag; //this is the magnetometer
RTC_DS3231 RTC; //clock

/*Set up sensor event framework*/
  sensors_event_t accelEvent;
  sensors_event_t gyroEvent;
  sensors_event_t tempEvent;
  sensors_event_t magEvent; 


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
  pinMode(HallPin, INPUT);

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
  char filename[] = "LOGGER00.CSV";
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
 
  /* Initialise the sensors */
    if(!accel.begin_I2C())
  {
    /* There was a problem detecting the LSM6DS33 ... check your connections */
    error("Accelerometer failed");
    digitalWrite(errorPin, HIGH);
    while(1);
  }
  if(!mag.begin_I2C())
  {
    /* There was a problem detecting the LIS3MDL ... check your connections */
    error("Magnetometer failed");
    digitalWrite(errorPin, HIGH);
    while(1);
    
  }
 
  //THIS IS WHAT GETS LOGGED - CSV header row
  logfile.println("datetime,heartIR,tempC,lightADC,hallADC,accelX,accelY,accelZ,pitch,roll,yaw,magX,magY,magZ");
 //if assembling own timestamps using big chunk below, use below instead:
 // logfile.println("datetime,year,month,day,hour,min,sec,heartIR,tempC,lightADC,hallADC,accelX,accelY,accelZ,pitch,roll,yaw,magX,magY,magZ");

}


// ACTUALLY RUNNING THIS
void loop(void) 
{ 
  DateTime now;

  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));

  digitalWrite(statusPin, LOW); //making LED blink when writing to SD
    //easier to see if it's still running or the battery is dead
  
//fetch the time
  now = RTC.now();
  seconds_now = now.second();

//this bit lets us record subsecond resolution - RTC won't do that natively
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
  HallReading = analogRead(HallPin);

//Convert analog input to degrees C
  //The next block of code is an expansion of this: TempC = ((TempReading*3.3)*100)-50;
    TempV = TempReading*3.3;
    TempC=TempV/1024; //equivalent to saying "TempC = previous value of TempC * 100"
    TempC-=.5; 
    TempC*=100;
    
//Get new sensor events 
  accel.getEvent(&accelEvent, &gyroEvent, &tempEvent);
  mag.getEvent(&magEvent);

  accelX = accelEvent.acceleration.x;
  accelY = accelEvent.acceleration.y;
  accelZ = accelEvent.acceleration.z;
  gyro_pitch = gyroEvent.gyro.x;
  gyro_roll = gyroEvent.gyro.y;
  gyro_yaw = gyroEvent.gyro.z;
  gyro_pitch += gyro_roll * sin(gyro_yaw);            // Transfer the roll angle to the pitch angle if the Z-axis has yawed
  gyro_roll -= gyro_pitch * sin(gyro_yaw);
  magX = magEvent.magnetic.x;
  magY = magEvent.magnetic.y;
  magZ = magEvent.magnetic.z;



// log time and data
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
  logfile.print(seconds_now);
  logfile.print(".");
  logfile.print(millis_diff);
  logfile.print(",");  
//The commented out block can be put back in for people who need to assemble their own timestamps from separate columns
/* 
  logfile.print(timeY);
  logfile.print(",");
  logfile.print(timeM);
  logfile.print(",");
  logfile.print(timeD);
  logfile.print(",");    
  logfile.print(timeHH);
  logfile.print(",");
  logfile.print(timeMM);
  logfile.print(",");
  logfile.print(seconds_now);
  logfile.print(".");
  logfile.print(millis_diff);
  logfile.print(",");
*/
  logfile.print(IRReading); //to get from IR reading to voltage: IRreading*3.3/1024
  logfile.print(",");  
  logfile.print(TempC);
  logfile.print(",");     
  logfile.print(LightReading);   
  logfile.print(",");    
  logfile.print(HallReading);
  logfile.print(",");    
  logfile.print(accelX,4);
  logfile.print(",");
  logfile.print(accelY,4);
  logfile.print(",");
  logfile.print(accelZ,4);
  logfile.print(",");
  logfile.print(gyro_pitch);
  logfile.print(",");
  logfile.print(gyro_roll);
  logfile.print(",");
  logfile.print(gyro_yaw);
  logfile.print(",");
  logfile.print(magX);
  logfile.print(",");
  logfile.print(magY);
  logfile.print(",");
  logfile.print(magZ);
  logfile.println();

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
  Serial.print(seconds_now);
  Serial.print(".");
  Serial.print(millis_diff);
  Serial.print(", IR ");   
  Serial.print(IRReading);
  Serial.print(", X ");    
  Serial.print(accelX,4);
  Serial.print(", Y ");
  Serial.print(accelY,4);
  Serial.print(", Z "); 
  Serial.print(accelZ,4);
  Serial.print(", Temp ");
  Serial.print(TempC);
  Serial.print(", Light ");
  Serial.print(LightReading);
  Serial.print(", Hall ");
  Serial.print(HallReading);
  Serial.println();

 
// Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  logfile.flush();
  digitalWrite(statusPin, HIGH);
}
