/*  ANALOG INPUT TEST SKETCH
 *  
 *  This is a simple sketch for testing whether all the analog
 *  inputs are working correctly. If you aren't sure a sensor is
 *  working happily, run this and see if you can get the values
 *  to change as you alter the condition.
 *  I find a finger covering and uncovering the IR and light
 *  sensors is good, as is pressing a thumb over the temp sensor.
 *  
 *  Don't forget to reload the main C-HAT sketch onto the logger
 *  before returning it to service!
 */

int A0read; //IR
int A1read; //temp
int A2read; //light


void setup() {
Serial.begin(9600);

}

void loop() {
analogRead(A0);
A0read = analogRead(A0);
analogRead(A1);
A1read = analogRead(A1);
analogRead(A2);
A2read = analogRead(A2);
Serial.print(A0read);
Serial.print(",");
Serial.print(A1read);
Serial.print(",");
Serial.println(A2read);
delay(500);

}
