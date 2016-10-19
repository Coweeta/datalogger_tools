#include <Servo.h>

Servo servo;

int pos = 0;

void setup()
{
   servo.attach(9); //set servo to pin 9
   servo.write(60); //set servo to mid-point
   Serial.begin(9600); 
}

void show_stuff(int from, int to)
{
  if((pos >= from) && (pos <= to)){
  
  Serial.println("hello");
  
  }
  else{ 
    Serial.println("nope");
  }
}

void loop()
{
  if (Serial.available() > 0) {

    // look for the next valid integer in the incoming serial stream:
    pos = Serial.parseInt(); 
     servo.write(pos); 
     show_stuff(10, 80);
  }
 /* for(pos = 0; pos < 180; pos += 1)  // goes from 0 degrees to 180 degrees 
  {                                  // in steps of 1 degree 
    servo.write(pos);                // tell servo to go to position in variable 'pos' 
    show_stuff(60, 70);
    delay(15);                       // waits 15ms for the servo to reach the position 
  } 
  for(pos = 180; pos>=1; pos-=1)     // goes from 180 degrees to 0 degrees 
  {                                
    servo.write(pos);                 // tell servo to go to position in variable 'pos' 
    show_stuff(10, 90);
    delay(15);                       // waits 15ms for the servo to reach the position 
  } */
  
}

   
