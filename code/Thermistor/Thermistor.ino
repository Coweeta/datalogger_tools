#include <math.h>         //loads the more advanced math functions
 
void setup() {            //This function gets called when the Arduino starts
  Serial.begin(9600);   //This code sets up the Serial port at 115200 baud rate
}
 
double Thermister(int RawADC) {  //Function to perform the fancy math of the Steinhart-Hart equation
 double Temp;
 double R1;
 double Volt;
 double TCel;
 double TKel;
 R1 = log(((312600 * 1024)/RawADC) - 312600); // (Resistance * digital voltage)/RawADC - Resistance
 TKel = 1 / (0.0008253 + (0.0002045 * R1) + (0.0000001144 * R1 * R1 * R1)); //steinhart and hart formula into Kelvin
 TCel = TKel - 273.15;              // Convert Kelvin to Celsius
 Temp = (TCel * 9.0)/ 5.0 + 32.0; // Celsius to Fahrenheit - comment out this line if you need Celsius
 return Temp;
}
 
void loop() {             //This function loops while the arduino is powered
  int val;                //Create an integer variable
  double temp;            //Variable to hold a temperature value
  val=analogRead(0);      //Read the analog port 1 and store the value in val
  temp=Thermister(val);   //Runs the fancy math on the raw analog value
  Serial.print("temp =  ");
  Serial.print(temp);   //Print the value to the serial port
  Serial.print("   A0 =  ");
  Serial.println(analogRead(A0));
  delay(1000);            //Wait one second before we do it again
  
}
