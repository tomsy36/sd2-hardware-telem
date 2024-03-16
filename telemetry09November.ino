/* 
William Toms 
started on 04/12/2022
SD^2 bike telemetry project 

red = recording 
green LED = waiting to record 
blue = error file and sd card system
yellow = low battery
purple = problem with RTC or accelerometer

3.3V analog reference
roughly 3.2V lipo cut of voltage 
adding const puts variables into flash instead of RAM
useful information (especially for optermisation): https://learn.adafruit.com/adafruit-feather-m0-adalogger/adapting-sketches-to-m0
7mA per pin
https://forum.arduino.cc/t/increasing-sd-library-buffer-size/345358/7
//float mappedValue = map(linearPotRearPos,479,1023,0,1000); // mapping currently not used as plan to do on software side
*/

/* to do:
tuesday to do - make file name formatting day/month/run number, add IMU data saving - save all RTC data to start of each file - look into how to upload data
 IMU, , formatting, filename based of rtc, multi files, calibration, upload mode
extra - sleep mode woken up by interrupt of button press?
binary files?

year 2 ideas:
more protection if sd card not opened etc so battery isnt wasted and user knows


*/


#include <SD.h>      //SD card libary may need to change
#include "RTClib.h"  // RTC library for time
RTC_DS3231 rtc;
//#include "ArduinoLowPower.h"

const int linearPotFront = A0;
const int linearPotRear = A1;  // for measuring suspension
const int frontBrake = A2;
const int rearBrake = A3;
const int batteryPin = A7;       // measures battery life
const int SDcardMechanical = 7;  // shows if SD card is present
const int startButton = 10;      // make sure correct pin - used to start and stop a run
String fileName = "run1.txt";    // file where data stored
const int RGB_RED = 13;
const int RGB_GREEN = 12;
const int RGB_BLUE = 11;
const int chipSelect = 4;
volatile unsigned long startTime = 0;  // timing variables
volatile unsigned long stopTime = 0;
volatile unsigned long totalTime = 0;
volatile int buttonPressCount = 1;
File dataLog;
char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

void setup() {
  Serial.begin(115200);  // used for debugging
  pinMode(linearPotFront, INPUT);
  pinMode(linearPotRear, INPUT);
  pinMode(frontBrake, INPUT);
  pinMode(rearBrake, INPUT);
  pinMode(batteryPin, INPUT);
  pinMode(SDcardMechanical, INPUT);
  pinMode(startButton, INPUT);
  pinMode(RGB_RED, OUTPUT);
  pinMode(RGB_GREEN, OUTPUT);
  pinMode(RGB_BLUE, OUTPUT);
  if (digitalPinToInterrupt(startButton) != -1) {
    Serial.println("interrupt Pin setup correctly");
  }
  attachInterrupt(digitalPinToInterrupt(startButton), buttonPressed, RISING);  // sometimes falling doesnt seem to work consistently
  flashLED();                                                                  // test if LED still works
  setUpRTC();
  SDcardSetUp();
  batteryVoltage();
  delay(5000);
}

void loop() {
  if (buttonPressCount % 2 == 0)  // if pressed at the start then interrupt enabled and buttonPressCount will be incremented hence == 2
  {
    Serial.println("interrupt");
    setRGBColour(1, 0, 0);
    record();  // start a recording
  } else {
    setRGBColour(0, 1, 0);
    Serial.println("no interrupt");
    Serial.println(digitalRead(startButton));
    // Serial.println("Battery voltage: " + String(batteryVoltage()));
  }
  delay(10);  // set so sampled at a rate of 40Hz could use LowPower.sleep(3000); or idle should be 2.5 but increased for testing
}

void record() {
  float linearPotRearPos = analogRead(linearPotRear);
  float linearPotFrontPos = analogRead(linearPotFrontPos);
  Serial.println("Rear position: " + String(linearPotRearPos));
  Serial.println("Front position: " + String(linearPotFrontPos));
  dataLog.println(String(linearPotRearPos) + "," + String(linearPotFrontPos));  // three strings on one line error? - seems to work
}

void buttonPressed()  // attached to interrupt
{
  buttonPressCount++;
  if (buttonPressCount % 2 != 0) {
    stopTime = millis();               // stop timer
    totalTime = stopTime - startTime;  // calculate total run time
    dataLog.println(String(totalTime));
    dataLog.println("Run finished");
    dataLog.flush();  // flush all data to the SD card
    dataLog.close();
  } else {
    //Serial.println("interrupt enabled"); problem opening new file each run - why button wasn't working
    //fileName = checkIfFileExists(fileName); // check works by setting new file for each run
    //dataLog = SD.open(fileName, FILE_WRITE); // opens file
    //openFile();
    delay(1000);
    startTime = millis();  // starts timer
  }
}

float batteryVoltage()  // calculates battery voltage - 3.2v cut off
{
  float batteryVolt = analogRead(batteryPin);
  batteryVolt *= 6.6;  // 3.3v logic and then set to a 100k potential divider so double
  batteryVolt /= 1024;
  if (batteryVolt < 3.3)  // battery low
  {
    setRGBColour(1, 1, 0);
    delay(2000);
    setRGBColour(0, 0, 0);
  }
  return batteryVolt;
}

bool checkSDcardPresent()  // mechanical check
{
  return digitalRead(SDcardMechanical);
}

void SDcardSetUp() {
  if (!SD.begin(chipSelect))  // if SD start up failed
  {
    Serial.println("card failed");
    Serial.println("restart device");
    setRGBColour(0, 0, 1);
    delay(3000);
    setRGBColour(0, 0, 0);
  } else {
    Serial.println("SD card initialised");
  }
  if (checkSDcardPresent() == false) {
    Serial.println("NO SD CARD");
    setRGBColour(0, 0, 1);
    delay(3000);
    setRGBColour(0, 0, 0);
  } else {
    Serial.println("SD card present");
  }
}

void openFile()  // check that the file opened correctly
{
  if (dataLog) {
    Serial.println("File opened");
  } else {
    Serial.println("File open failed");
    setRGBColour(0, 0, 1);
    delay(3000);
    setRGBColour(0, 0, 0);
  }
}

String checkIfFileExists(String newFileName)  // function to create a new file if fileName alrady exists so that every time the device is restarted a new file can be created
{
  int i = 1;
  String txt = ".txt";
  while (SD.exists(newFileName)) {
    Serial.println("File already exists");
    newFileName.remove(3);  // remove numbering characters will this only work for numebrs up to 9? might need to be more robust
    i++;
    newFileName = (newFileName + String(i));  // add the new count find out whether .txt required
    newFileName = (newFileName + txt);
  }
  Serial.println("New file created");
  return newFileName;
  // use return values as local variables
}

void flashLED()  // used for flashing the LED white for testing
{
  for (int x = 0; x < 20; x++) {
    setRGBColour(1, 1, 1);
    delay(50);
    setRGBColour(0, 0, 0);
    delay(50);
  }
}

void setRGBColour(int red, int green, int blue) {
  digitalWrite(RGB_RED, !red);  // ! because common anode
  digitalWrite(RGB_GREEN, !green);
  digitalWrite(RGB_BLUE, !blue);
}

void setUpRTC() {
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    setRGBColour(1, 1, 0);
    delay(3000);
    setRGBColour(0, 0, 0);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void getRTCData() {
  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.print("Temperature: ");
  Serial.print(rtc.getTemperature());
  Serial.println(" C");
}
