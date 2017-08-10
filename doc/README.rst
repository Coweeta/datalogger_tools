#################
Data Logger Tools
#################

A software framework for building a data logger with Arduino hardware.

Intent
######

To provide code that handles most of the common tasks of data logging to allow
researcher to focus on the specifics of their application.

* file management
* clock management
* user interface
* log file download
* system identification


Components
##########



Requirements
############

* An Arduino microcontroller board such as the UNO
  (https://www.arduino.cc/en/Main/ArduinoBoardUno)

* An Adafruit Data Logger shield (Product ID: 1141) or equivalent.
  (https://learn.adafruit.com/adafruit-data-logger-shield)

* Two digital output lines wired to a green and a red LED




It is up to the programmer to ensure that the data fields are in the correct
columns in the CSV file.



The minumum interval is one second.





Coding Style
* Use the // comment style
https://google.github.io/styleguide/cppguide.html




Hardware
########

We will use the Arduino MEGA2560 ass the microcontroller board over the UNO.
The reasoning is as follows:
- the data_logger library uses around 1500 bytes of RAM, 75% of what's available in the UNO.
 The MEGA has 8kB of RAM rather than 2kB
- the MEGA has 4 serial communications ports whereas the UNO has one.
  This lets us keep serial1 for USB usage and allocate one for an RS485 link

Using the Data Logger Shield
============================

The shield(s) use SPI for communication with the SD card and I2C for connection with the RTC.
As shipped these pins are mapped for the UNO

I2C
---

We have to connect SCL (pin 21 of mega) and SDA (pin 20 of mega)

SPI
---

