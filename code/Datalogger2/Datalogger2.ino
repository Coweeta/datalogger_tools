const int TempPin = A1;

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  int Temp = analogRead(TempPin);
    Serial.print(Temp); Serial.print(">");
  float millivolts = (Temp/1024.0) * 5000;
  float celsius = millivolts/10;
  Serial.print(celsius);
  Serial.println(" degrees Celsius,");
  
  delay(1000);
}
