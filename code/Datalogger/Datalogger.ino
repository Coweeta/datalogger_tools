const int TempPin = A1;
const int RhPin = A2;
//const int RefPin = A3;


void setup(){
  
  Serial.begin(9600);
  
}

void loop(){
  
  
float Temp;
float Rh;  
int TempVal;
int RhVal;
  
  Temp = (analogRead(TempPin)) * 0.14;
  Rh = analogRead(RhPin) * .1; 
 // Ref = analogRead(RefPin);
  
  
  TempVal = map(Temp, 0, 1023, -80, 60);
  RhVal = map(Rh, 0, 1023, 0, 100);
  
  Serial.print("  Temp = ");
  Serial.print(TempVal);
  Serial.print("  Rh = ");
  Serial.println(RhVal);
  //Serial.print("  Ref = ");
  //Serial.println(Ref);
  
  
  delay(5000);
}
