///////////////Library Includes////////////////////
// include the SD library:
#include <SD.h>
#include <SPI.h>
#include <TimeLib.h>
#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>
#include <i2c_t3.h>

// set up variables using the SD utility library functions:
//Sd2Card card;
//SdVolume volume;
//SdFile root;

//==============================================================================
/*
                      vvvv Global Variables/Declarations vvvv
*/
//==============================================================================
unsigned long myTime;
int HOWMANY = 0;
int datapointrows = 0; //How many times have we written to the csv, delete after ~2 weeks.
int eventsrows = 0;
int PermaON = 0;
int LED_Needed = 1; //Functional & Needed is not what we want.
int LED_Actual = 5;
int thiscount=0;
//////////////////////////////////////
float current_samples[400]; //
float * p_current_samples = current_samples;
float Max_Samples = 400; // ==Testing only==//
int SampleCount = 0;
//Time information for datapoints
String Time = "";
String * p_Time = &Time;
String Date = "";
String * p_Date = &Date;
String Device_Name = "MCMD 01";
String * p_Device_Name = &Device_Name;
//========================

//Pointer loaded with datapoint information
float sum; float * p_sum = &sum;
float average; float * p_avg = &average;
float peak; float * p_peak = &peak;
float datapoints[2];  //Number of columns for the datapoint. (Average, Peak) FOR LOCAL CSV
float * p_datapoints = datapoints;

String datastring = ""; // updated empty string ; will added samples
String * p_datastring = &datastring;
String datastring2 = "";
String eventstring = "" ; //empty
String * p_eventstring = &eventstring;

//========================================
//===========CURRENT SENSOR===============
//use pin A13 on Teensy
//#define Pin A13;
float RefVal = 3.3;
//These are the variable for the current sensor
float sensitivity = 1000.0 / 264.0; //1000mA per 264mV
float Vref = 327.49;   //This will auto-zero

float unitValue = (3.3 / 1024.0) * 1000 ;

float voltage;  //for ACS725
float * p_voltage = &voltage;

long int sensorValue = 0;
long int * p_sensorValue = &sensorValue;
// sensor value is in bit value
float current = 1 ;  // later 1 if for low current for testing
float * p_current = &current;

//========Mesurement Temp Sensor===========
int tempPin = A9;   //A9
int tempReading;
float temperatureC;
float MaxTemp = 80.00; //Trigger Safety Mode at this temperature.
//===========Operation Mode Testing  =====================//
/////////////////////////////////////////////////////////////
//Initialization of Global Variables
float LowCThresh = 2; // A   //Initialized to NORMAL
float HighCThresh = 1300; //A
const int NormalLowCThresh = 2;
const int NormalHighCThresh = 1300;
const int TestingLowCThresh = 1;
const int TestingHighCThresh = 5;

int lowcount = 0;         //Amount of sample where the current<2A.
int low_current_report = 0; //Flag to indicate when low current event is reported.
int * pLowcount = &lowcount;

int highcount = 0;
int * pHighcount = &highcount; // for 5 s interval

int healthycount = 0;
int * pHealthycount = &healthycount;
int * pReport = &low_current_report;
//////////////////////////Millis Variables/////////////////////////////////
const int MaxTime = 4294967295 ; //ms
const int SampleInterval = 800; //1 sec      //TO BE FINE-TUNED
unsigned long DeltaTSample;
unsigned long CurrentTimeSample = 0;
unsigned long PreviousTimeSample = 0;

int DataInterval = 300000; //5 min default
const int NormalDataInterval = 300000 ; // 5 min
const int TestingDataInterval = 300000 ; //5 minutes for OpModes (do not interfere)  
unsigned long DeltaTData;
unsigned long CurrentTimeData = 0;
unsigned long PreviousTimeData = 0;

//Reduced is required to see if we've been in this mode for 24 hours
const int ReducedInterval = 86400000 ; // 24 hrs
unsigned long DeltaTReduced;
unsigned long CurrentTimeReduced = 0;
unsigned long PreviousTimeReduced = 0;

unsigned long DeltaTFunctional;
unsigned long CurrentTimeFunctional = 0;
unsigned long PreviousTimeFunctional = 0;

const int BatteryCheckInterval = 15000 ; // 30s
unsigned long DeltaTBatteryCheck;
unsigned long CurrentTimeBatteryCheck = 0;
unsigned long PreviousTimeBatteryCheck = 0;

const int SafetyInterval = 10000; // wait 60 sec
unsigned long DeltaTSafety;
unsigned long CurrentTimeSafety = 0;
unsigned long PreviousTimeSafety = 0;

const int SerialTxInterval = 1000; // 1.5seconds per string
unsigned long DeltaTSerialTx;
unsigned long CurrentTimeSerialTx = 0;
unsigned long PreviousTimeSerialTx = 0;
// ================TRANSMISSION======================//
String OpMode[5];
int DataType[5];
String DTG[5];
String TXdata1[5]; //Columns for Serial Tx to BORON
String TXdata2[5];
int TxVal = 0;
int TxValCount = 0;
int ForwardTxCount = 0;
int TeensyWrite;

/////Email variable/////
int EmailFlag=0;
 String EmailField1[5];
 String EmailField2[5];
 String EmailField3[5];
 int EmailIndex=0;  //How many have we sent
 int EmailCount=0;  //How many are saved
 int EmailType[5];  //Will need to be set each time
 String EmailOpMode[5];  //Will need to be set each time
///////////////////////

int HighMBTemp = 0;
int CurrentState = 1 ; // initialized in functional
int PreviousState = 1;// initialized in functional

int TransmitNowFlag = 0;
int valuecount = 0; //Which element is to be sent over serial
//===========================HEATING PAD, BATTERY LEVEL AND COULOMB COUNTER=====================================//

// Heater Pad Pin
int HeatingPad = 31; // this is the wrong pin

// Coulomb counter
// Register definitions
#define FullChargeMSB 0xA0
#define FullChargeLSB 0xD8
#define LTC2943_ACCUM_CHARGE_MSB_REG 0x02
#define LTC2943_ACCUM_CHARGE_LSB_REG 0x03
#define LTC2943_VOLTAGE_MSB_REG 0x08
#define LTC2943_VOLTAGE_LSB_REG 0x09
#define LTC2943_CURRENT_MSB_REG 0x0E
#define LTC2943_CURRENT_LSB_REG 0x0F
#define LTC2943_TEMPERATURE_MSB_REG 0x14
#define LTC2943_TEMPERATURE_LSB_REG 0x15
#define LTC2943_I2C_ADDRESS 0x64
#define LTC2943_CONTROL_REG  0x01
#define LTC2943_AUTOMATIC_MODE 0xC0
#define LTC2943_PRESCALAR_M_1024 0x28


const float resistor = .025;  //shunt resistor value
const float BatterySize = 7000; //total battery capacity
const float lsb = 0.17; //mAh 0.34mAh * 50/25mohm * 1024/ 4096
float chargelevelmAh;
uint16_t prescalar_mode = LTC2943_PRESCALAR_M_1024;
uint16_t prescalarValue = 1024;
uint16_t alcc_mode = 0x00;
uint8_t status_code, hightemp_code, lowtemp_code;

const int FunctionalThreshold = 3500; // mAh 50%
const int ShutdownThreshold = 700;  // mAh 10%
const int P25Threshold = 1750 ; // mAh 25%
//===========================LEDs DECLARATION=====================================//
//Define Statements
#define WHITE 36
#define RED 33
#define GREEN 35
#define BLUE 34
//Yellow = RED & GREEN
//int LED_BUSY = 0; //This tells the system if the LED is being used for something else. Prevents odd colors from 2 things going at once.
//Initialization
int LED_INIT_USE = 0;
int LED_INIT_ON = 0;
unsigned long  CurrentTime_LED_INIT;
unsigned long  PreviousTime_LED_INIT;
unsigned long  DeltaT_LED_INIT;
unsigned long  Interval_LED_INIT = 500; //0.5s //Since ONtime=OFftime, this can be used for both.
int LED_INIT_Cycles = 2; //ON-OFF Twice
int LED_INIT_Count = 0; //Amount of time LED was cycled.

//Functional Mode
int LED_FUNCTIONAL_USE = 0;
int LED_FUNCTIONAL_ON = 0;
unsigned long CurrentTime_LED_FUNCTIONAL;
unsigned long PreviousTime_LED_FUNCTIONAL;
unsigned long DeltaT_LED_FUNCTIONAL;
unsigned long Interval_LED_FUNCTIONAL = 5000; //5s

//Reduced Mode
int LED_REDUCED_USE = 0;
int LED_REDUCED_ON = 0;
unsigned long CurrentTime_LED_REDUCED;
unsigned long PreviousTime_LED_REDUCED;
unsigned long DeltaT_LED_REDUCED;
unsigned long Interval_LED_REDUCED = 5000; //5s

//Shutdown Mode
int LED_SHUTDOWN_USE = 0;
int LED_SHUTDOWN_ON = 0;
unsigned long CurrentTime_LED_SHUTDOWN;
unsigned long PreviousTime_LED_SHUTDOWN;
unsigned long DeltaT_LED_SHUTDOWN;
unsigned long Interval_LED_SHUTDOWN = 5000; //5s

//Safety Mode
int LED_SAFETY_USE = 0;
int LED_SAFETY_ON = 0;
unsigned long CurrentTime_LED_SAFETY;
unsigned long PreviousTime_LED_SAFETY;
unsigned long DeltaT_LED_SAFETY;
unsigned long Interval_LED_SAFETY = 5000; //5s //Could be made to stay on much longer.

//Power ON
int LED_POWER_USE = 0;
int LED_POWER_ON = 0;
unsigned long CurrentTime_LED_POWER;
unsigned long PreviousTime_LED_POWER;
unsigned long DeltaT_LED_POWER;
unsigned long Interval_LED_POWER = 2000; //2s

unsigned long CurrentTimeEmail;
unsigned long PreviousTimeEmail;
unsigned long DeltaTEmail;
unsigned long EmailInterval = 600000; //10min for this demonstration 
//=====================TESTING====================================//

int DipSwitch4 = 0;
int DipSwitch3 = 0;
int DipSwitch2 = 0;  //Only used in the TEST CODE.
int DipSwitch1 = 0;  //Only used in the TEST CODE.
//====================Global RTC Declarations==============================

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}


/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message
unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1612681200; // Feb 7 2021
  if (Serial.find(TIME_HEADER)) {
    pctime = Serial.parseInt();
    return pctime;
    if ( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
      pctime = 0L; // return 0 to indicate that the time is not valid
    }
  }
  return pctime;
}
//==============================================================================
/*
                      ^^^^Global Variables/Declarations^^^^^
*/
//==============================================================================


//==============================================================================
/*
                      vvvvvvv Initializations vvvvvvvvv
*/
//==============================================================================
int relay = 6;  //Bi-stable relay declaration right thur
const int chipSelect = BUILTIN_SDCARD;
//==============================================================================
/*
                      ^^^^Initializations^^^^^
*/
//==============================================================================

//==============================================================================
/*
                      vvvvvv Functions vvvvvvv
*/
//==============================================================================//

//=============================================================================//
void refreshSerial() {
  Serial7.end();
  Serial7.begin(9600);
  Serial.end();
  Serial.begin(9600);
}
void Isolate() {
  int sense = digitalRead(7); // if sense =1 means it is already isolate
  if (sense == 0) {
    digitalWrite(relay, HIGH); //pulse
    delay(100);
    digitalWrite(relay, LOW);
  }
}



void Disolate() {
  int sense = digitalRead(7); // if sense=1 means it is already isolate
  if (sense == 1) {
    digitalWrite(relay, HIGH); //pulse
    delay(100);
    digitalWrite(relay, LOW);
  }
}

//==========================================================

//===============================================================================//
//Name: ACS Initialization
//Purpose: Provides auto-zeroing when it the MCMD is powered on.
void ACS_init() {
  Isolate();

  const int numValue = 500;
  for (int i = 0; i < numValue; i++) {
    *p_sensorValue += analogRead(A13); // take 1 sample at time for resolution
    delay(1);
  }
  *p_sensorValue = *p_sensorValue / numValue;
  *p_voltage = unitValue * (*p_sensorValue);
  Vref = *p_voltage;
  Serial.print("Autozero to reference voltage (mV): ");
  Serial.println(Vref);

  Disolate();
}
///////////////////////////////////////////////////////////

//------------------------------------------------------///

///////////////////////////////////////////////////////////
//Name: Samples
//Purpose: this function takes 500 rapid samples for the AVERAGE (and not RMS) current.
void Samples() {

  const int numValue = 500; //To be tuned for 1 period.
  //   should change it 400 ?

  for (int i = 0; i < numValue; i++) {
    *p_sensorValue += analogRead(A13); // take 1 sample at time for resolution //can add small delay if needed
  }
  *p_sensorValue = *p_sensorValue / numValue;
  *p_voltage = unitValue * (*p_sensorValue);
  *p_current = (*p_voltage - Vref) * sensitivity;   // Calculate the corresponding current
  *p_current = ((*p_current) * 1.11 / 1000) + 1; //-0.2;//*200/1000; //Offset is 10 is still needed?


  if(thiscount<5){
  *p_current = ((*p_current)*1.11/1000)+2;//-0.2;//*200/1000; //Offset is 10 is still needed?
  thiscount+=1;
  }
  else if(thiscount < 10){
  *p_current = ((*p_current)*1.11/1000)+10;//-0.2;//*200/1000; //Offset is 10 is still needed?
  thiscount+=1;   
  }
  else{
  *p_current = ((*p_current)*1.11/1000)+2;//-0.2;//*200/1000; //Offset is 10 is still needed?
  }

  
  SampleCount += 1;
  *p_current_samples = (*p_current); //RMS(1.11) & primary(200) value NOW IN AMPS    // every time sample() is called, the updated sample is added to an array current samples
  *p_datastring += String(*p_current_samples) + ",";
  *p_current_samples++; //Will say the last one is not used, this is true, but necessary.





} // end Samples
///////////////////////////////////////////////////////////

//------------------------------------------------------///

///////////////////////////////////////////////////////////
//================================================================================
//Name: temperature
//Purpose: Get the temperature from the measurement board.
void temperature() {
  tempReading = analogRead(tempPin);
  float voltage_temp = tempReading * 3.3;
  voltage_temp /= 1024.0;
  temperatureC = (voltage_temp - 0.5) * 100;
  // Serial.print("->Temperature: ");
  // Serial.print(temperatureC);
  // Serial.println("°C");
}
///////////////////////////////////////////////////////////

//------------------------------------------------------///

///////////////////////////////////////////////////////////
//================================================================================
//Name: TempControl
//Purpose: Function to be called when the MCMD senses the measurement board is over 80C.
void TempControl() {
  while (temperatureC > MaxTemp) {
    temperature(); //Good code to be uncommented
    delay(10);

    //Test Case OnLY//
    //Serial.println("Temperature is tooo high");
    //delay(5000);
    //temperatureC = 60;// lower temp
    /////////////////

  }
  return;
}
//================================================================================
///////////////////////////////////////////////////////////

//------------------------------------------------------///

///////////////////////////////////////////////////////////
//Name: Samples2CSV
//Purpose: With use of a pointer to the sample datastring, this function
//creates a CSV file of all current samples to be used in the datapoint.
//Note: In a later revision, we may want this function to include the pointers,
//in the same way Data2CSV works.
//Transfer #Max_Samples# samples from dummy into current_samples
void Samples2CSV() {

  SD.remove("SAMPLES.csv"); //clear file first before adding anything to the csv file ********THIS NEEDS TO BE UNCOMMENTED AFTER TESTING
  File dataFile = SD.open("SAMPLES.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(*p_datastring);
    delay(100);
    Serial.println(*p_datastring);
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening SAMPLES.csv");
  }
  datastring = "";  //Clear the datastring for something else
  return;
}// end transfer
//==================================================================================
///////////////////////////////////////////////////////////

//------------------------------------------------------///

///////////////////////////////////////////////////////////
//Name: Average
//Purpose: With use of pointers, this function calculates
///both the sum and the average of an array with samples and
//stores it within the pointer p_avg.
//
// Taking the sum of the elements
void Average() {

  p_current_samples = current_samples; //Reset pointer to first elemnent
  //Serial.print("Reset to first element: ");
  //Serial.println(*p_current_samples);
  // Summing
  float total = 0; // Needs to be a float too
  for (int i = 0; i < SampleCount; i++) {
    //Serial.print("Checking Value: ");
    //Serial.println(i);
    total += *p_current_samples;
    //Serial.print("The total is now: ");
    //Serial.println(total);
    p_current_samples++;
  }
  p_current_samples = current_samples; //Reset pointers
  *p_sum = total;

  //Serial.print("The toal is: ");
  //Serial.println(total);

  /*float size = sizeof(current_samples) / sizeof(current_samples[0]);
    Serial.print("The SampleCount is: ");
    Serial.println(SampleCount);
     Serial.print("The Size is: ");
    Serial.println(size);   */

  //float avg = sum / size;
  float avg = sum / SampleCount;
  //=======increment p_datapoints =======//
  *p_datapoints = avg;
  p_datapoints++;
  *p_avg = avg;

  Serial.print("The average current is ");
  Serial.print(avg);
  Serial.println("A");

  return;
} //end Average
///////////////////////////////////////////////////////////

//------------------------------------------------------//

///////////////////////////////////////////////////////////
//Name: Peak
//Purpose: With use of pointers, this function finds
///and assigns the highest value within the samples array to p_peak.
void Peak() {

  int peak_index = 0 ; // is the peak current index
  //float peak_val; // is the peak current value
  for (int i = 0; i < SampleCount; i++) {
    if (current_samples[i] > current_samples[peak_index]) {
      peak_index = i; //max will store the maximum values of the current samples
    } // end if
  } //end for

  //  peak_val = current_samples[peak_index];  //made a pointer switch here
  *p_peak = current_samples[peak_index];
  Serial.print("The peak current is ");
  Serial.print(*p_peak);
  Serial.println("A");
  //=======decrement pointer p_datapoints =======//
  //  *p_datapoints = peak_val;  //Ken Commented out
  *p_datapoints = *p_peak;
  p_datapoints--;
  SampleCount = 0;
  return;
}//end Peak
///////////////////////////////////////////////////////////

//------------------------------------------------------//

///////////////////////////////////////////////////////////
//Name: Data2CSV
//Purpose: With use of pointers, this function places all the
///appropriate pointers within a string to be appended to dataOUT.csv.
void Data2CSV() {
  getTime();// call Time and Date
  getDate();
  datastring = "" ; //empty
  datastring = String(*p_Device_Name) + "," + String(*p_Date) + "," + String(*p_Time) + "," + String(*p_avg) + "," + String(*p_peak); // for dummy 1
  File dataFile = SD.open("dataOUT.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(datastring);
    delay(100);
    dataFile.close();
    // print to the serial port too:
    Serial.println(datastring); // means it is true
  }
  else {
    Serial.println("error opening dataOUT.csv"); // if the file isn't open, pop up an error:
  }
  datastring = "" ; //empty
}// end DATA2CSV

///////////////////////////////////////////////////////////

//------------------------------------------------------//

///////////////////////////////////////////////////////////
//Name: format CSV
//Purpose: With use of pointers, this function places all the
///appropriate headers within a string to be the first column of dataOUT.csv.
void formatDataCSV() {
  String datastring = "" ; //empty
  datastring = String("Device Name") + "," + String("Date (dd/mm/yyyy)") + "," + String("Time (24hrs)") + "," + String("Average RMS Current (A)") + "," + String("Peak RMS Current (A)"); // putting the headers
  // print to the serial port too:
  Serial.println(datastring);
  File dataFile = SD.open("dataOUT.csv", FILE_WRITE);

  if (dataFile) {
    dataFile.println(datastring);
    delay(1000);

    dataFile.close();

    // print to the serial port too:
    Serial.println(datastring); // means it is true
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening dataOUT.csv");
  }
}//end of formatDataCSV
//=======================================================
void formatEventsCSV() {
  String datastring = "" ; //empty
  datastring = String("Device Name") + "," + String("Date (dd/mm/yyyy)") + "," + String("Time (24hrs)") + "," + String("Event Message"); //the headers

  File dataFile = SD.open("EVENTS.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(datastring);
    delay(100);
    dataFile.close();
    Serial.println(datastring); // means it is true
  }
  else {   // if the file isn't open, pop up an error:
    Serial.println("error opening Events.csv");
  }
}//end of formatEventsCSV
//========================================================

void Events2CSV() {
  getTime();  // call Time and Date
  getDate();
  // SD.remove("EVENTS.csv"); //All events to be logged until a determined period

  datastring2 = "" ; //empty
  datastring2 = String(*p_Device_Name) + "," + String(*p_Date) + "," + String(*p_Time) + ","  ; // add eventstring
  datastring2 += eventstring;

  // print to the serial port too:
  //Serial.println(datastring);
  File dataFile = SD.open("Events.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(datastring2);
    delay(100); //delay is needed for timing
    dataFile.close();
    Serial.println(datastring2); // means it is true
  }
  else {  // if the file isn't open, pop up an error:
    Serial.println("error opening Events.csv");
  }
  datastring2 = "" ; //empty
  eventstring = "" ; //empty

  eventsrows += 1;
}// end Events2CSV
///////////////////////////////////////////////////////////

//------------------------------------------------------//

///////////////////////////////////////////////////////////
//Name: getTime
//Purpose: With use of pointers, this function places the
//current time in a string pointed to by p_Time.
void getTime() {
  *p_Time = "";
  int hour_val = hour();
  *p_Time += String(hour_val) + ":";

  int minute_val = minute();
  if (minute_val < 10) {
    *p_Time += String("0") + String(minute_val) + ":";
  }
  else {
    *p_Time += String(minute_val) + ":";
  }
  int second_val = second();
  if (second_val < 10) {
    *p_Time += String("0") + String(second_val);
  }
  else {
    *p_Time += String(second_val);
  }
}// end getTime
///////////////////////////////////////////////////////////
//------------------------------------------------------//
///////////////////////////////////////////////////////////
//Name: getDate
//Purpose: With use of pointers, this function places the
//current date in a string pointed to by p_Date.
void getDate() {
  *p_Date = "";
  int day_val = day();
  if (day_val < 10) {
    *p_Date += String("0") + String(day_val) + "/";
  }
  else {
    *p_Date += String(day_val) + "/";
  }
  int month_val = month();
  if (month_val < 10) {
    *p_Date += String("0") + String(month_val) + "/";
  }
  else {
    *p_Date += String(month_val) + "/";
  }

  int year_val = year();

  *p_Date += String(year_val);

}// end getDate
///////////////////////////////////////////////////////////
//------------------------------------------------------//
///////////////////////////////////////////////////////////
//Name: initRTC
//Purpose: This function initializes the Teensy's RTC.
void initRTC() {
  if (Serial.available()) {
    time_t t = processSyncMessage();
    if (t != 0) {
      Teensy3Clock.set(t); // set the RTC
      setTime(t);
    }
  }
}// end initRTC

//=======================================================
//function returns DeltaT time
unsigned long getDeltaT(unsigned long CurrentReading, unsigned long PreviousReading) {
  if (CurrentReading < PreviousReading) {
    // Time Elaspe
    unsigned long  DeltaT = (MaxTime - PreviousReading) + CurrentReading;
    return (DeltaT);
  }
  else {
    unsigned long   DeltaT = CurrentReading - PreviousReading;
    return (DeltaT);
  }
}//end of getDeltaT
//==============================================================================


////////////////////////////////////////////////////////////////
// MCMD Operational Modes
// Purpose: Set all required pointers to change operation modes.
// Input: None
// Output: None
void FunctionalMode() {
  PreviousTimeFunctional = millis();
  // *pLED=1;
  // *p2=FunctionalDelay;

  //================ LED STUFF HERE==========================//
  LED_FUNCTIONAL_USE = 1; //This one needs to be used.
  LED_Needed = 1;
  LED_FUNCTIONAL_ON = 0; //This one is currently OFF.
  //=========================================================
  PreviousState = CurrentState;
  //Serial.println(PreviousState);
  CurrentState = 1; // 1=FunctionalMode
  //Serial.println(CurrentState);
Serial.print("Writing Functional Mode Event: ");
Serial.print(TxValCount);  

  getDate();
  getTime();
  DataType[TxValCount]=2;
  DTG[TxValCount] = "";
  DTG[TxValCount] = String(*p_Date) + "," + String(*p_Time);
  TXdata1[TxValCount] = "Entered Functional Mode";
  if (PreviousState == 2) { //Last State was Reduced Power Mode
    TXdata2[TxValCount] = "Battery Charged Above 50%";
  }
  else if (PreviousState == 3) { //From Shutdown to Functional
    TXdata2[TxValCount] = "Battery Successfully Charged From Below 10% to Above 50%";
  }
  else if (PreviousState == 4) { //Last State was Safety Mode
    TXdata2[TxValCount] = "Current is Safe and Internal Temperature is Below 80C";
  }
  TxValCount += 1;
 // Serial.print("Done Writing Reduced Mode Event: ");
 // Serial.print(TxValCount);
  TransmitNowFlag = 1;

    //Email Content
 /* if (PreviousState == 2) {//Last State was Reduced Power Mode
    EmailFlag=1;
    EmailField1[EmailCount]= String(*p_Date)+" at "+String(*p_Time)+", ";
    EmailField1[EmailCount] += "the MCMD returned to Functional Mode because the battery charged above 50%";
    EmailField2[EmailCount]="";
    EmailField3[EmailCount]="";
    EmailType[EmailCount]=3;  //Will need to be set each time
    EmailOpMode[EmailCount]=3;  //Will need to be set each time
    EmailCount+=1;  //How many are saved
    Serial.println("Functional Email Prepped");
    //SerialEmailTx();
  }*/
   
  return;
}

void ReducedMode() {
  PreviousTimeReduced = millis();
  digitalWrite(HeatingPad, LOW);  // Heater Active
  //    *pLED=2;
  //    *p2=ReducedDelay;
  //================ LED STUFF HERE==========================//
  LED_REDUCED_USE = 1; //This one needs to be used.
  LED_Needed = 2;
  LED_REDUCED_ON = 0; //This one is currently OFF.
  //=========================================================
  PreviousState = CurrentState;
  CurrentState = 2; // 2=Reduced Mode
  getDate();
  getTime();
  //Serial.print("Writing Reduced Mode Event: ");
  //Serial.print(TxValCount);
  DataType[TxValCount] = 2;
  DTG[TxValCount] = "";
  DTG[TxValCount] = String(*p_Date) + "," + String(*p_Time);
  TXdata1[TxValCount] = "Entered Reduced Mode";
  if (PreviousState == 1) { //Last State was Functional Mode
    TXdata2[TxValCount] = "Battery Discharged Below 50%";
  }
  else if (PreviousState == 3) { //Last State was Shutdown Mode
    TXdata2[TxValCount] = "Battery Charged Above 25%";
  }
  else if (PreviousState == 4) { //Last State was Safety Mode
    TXdata2[TxValCount] = "Current is Safe and Internal Temperature is Below 80C";
  }
  TxValCount += 1;
  //Serial.print("Done Writing Reduced Mode Event: ");
  //Serial.print(TxValCount);
  TransmitNowFlag = 1;
  Serial.println("Set up Reduced Mode");

    //Prep Email
    //Email Content
 /* EmailFlag=1;
  EmailField1[EmailCount]= String(*p_Date)+" at "+String(*p_Time)+", ";
  if (PreviousState == 1) { //Last State was Functional Mode
    EmailField1[EmailCount] += "the MCMD entered Reduced Mode because the battery discharged below 50%";
    EmailField2[EmailCount]="If the device charges properly,";
    EmailField3[EmailCount]="it should soon rise above this level and return to Functional Mode";
  EmailType[EmailCount]=3;  //Will need to be set each time
  EmailOpMode[EmailCount]=3;  //Will need to be set each time
  EmailCount+=1;  //How many are saved
  Serial.println("Reduced Email Prepped");
 // SerialEmailTx();
  }
  else if (PreviousState == 3) { //Last State was Shutdown Mode
    EmailField1[EmailCount] += "the MCMD returned to Reduced Mode because the battery charged above 25%";
    EmailField2[EmailCount]="If the device continues charging properly,";
    EmailField3[EmailCount]="it should soon return to Functional Mode without issue";
      EmailType[EmailCount]=3;  //Will need to be set each time
  EmailOpMode[EmailCount]=3;  //Will need to be set each time
  EmailCount+=1;  //How many are saved
  Serial.println("Reduced Email Prepped");
  SerialEmailTx();
  }*/
  ////////////////////////////
  return;
}

void ShutdownMode() {
  Isolate();                                                                                           
  digitalWrite(HeatingPad, HIGH);  // Heater Inactive
  //================ LED Shutdown===================================//
  int LED_SHUTDOWN_USE = 1; //This one needs to be used.
  LED_Needed = 3;
  int LED_SHUTDOWN_ON = 0; //This one is currently OFF.

  while (LED_SHUTDOWN_USE && DipSwitch3) {  //LED1ON means the LED1 must be turned ON/OFF
    if (!LED_SHUTDOWN_ON) {
      digitalWrite(WHITE, LOW);
      digitalWrite(GREEN, LOW);
      digitalWrite(RED, HIGH);  //Turn ON.
      LED_SHUTDOWN_ON = 1;        //This one is now ON.
      PreviousTime_LED_SHUTDOWN = millis(); //Represents the time it went ON.
    }

    //Is it time to turn off?
    CurrentTime_LED_SHUTDOWN = millis();
    DeltaT_LED_SHUTDOWN = getDeltaT(CurrentTime_LED_SHUTDOWN, PreviousTime_LED_SHUTDOWN);

    //IF we go to a shutdown loop, we need to turn off the LED in that loop because it wont return here in time./////////////********************
    if (DeltaT_LED_SHUTDOWN > Interval_LED_SHUTDOWN) {
      digitalWrite(RED, LOW); //Turn OFF.
      PreviousTime_LED_SHUTDOWN = millis();
      Serial.println("LED go OFF");
      LED_SHUTDOWN_USE = 0; //This one doesnt need to be used anymore.
      LED_SHUTDOWN_ON = 0; //Mark state as OFF
    }
  }
  DipSwitch3 = digitalRead(38); // CHECK LOGIC
  if (!DipSwitch3 && (LED_Needed != LED_Actual)) { //Test Mode and a Change is needed
    PermaON = 1;
    digitalWrite(GREEN, LOW);
    digitalWrite(WHITE, LOW);
    digitalWrite(RED, HIGH);
    LED_Actual = 3;
  }
  //===============================================================//
  PreviousState = CurrentState;
  CurrentState = 3; // 3=Shutdown Mode
  getDate();
  getTime();
  DataType[TxValCount] = 2;
  DTG[TxValCount] = "";
  DTG[TxValCount] = String(*p_Date) + "," + String(*p_Time);
  TXdata1[TxValCount] = "Entered Shutdown Mode";
  if (PreviousState == 2) { //Last State was Reduced Power Mode
    TXdata2[TxValCount] = "Battery Discharged Below 10%";
  }
  else if (PreviousState == 1) {
    TXdata2[TxValCount] = "Battery Level Dropped to Below 10%";
  }
  else if (PreviousState == 4) { //Last State was Safety Mode            //CAN THIS EVEN HAPPEN  -- Should monitor the board temperature, but we can isolate to prevent this
    TXdata2[TxValCount] = "This Should Not Have Occurred"; //This implies that we can go to safety from shutdown
  }
  eventstring = "Entering Shutdown Mode";
  Events2CSV();
  TxValCount += 1;
  //////////////////
  /////////////////////////// This will force the Shutdown message to be pushed before we go to ReducedPower Mode.
  //This allows us to tell the BORON that transmissions are NOT allowed during this time.
  while (valuecount < 5) {
    TxVal = digitalRead(12);
    //Serial.print("Reading Pin 12 as: ");
    //Serial.println(TxVal);
    delay(10);
    if (!TxVal) {    //Should be if(!TxVal)
      digitalWrite(11, HIGH);
      CurrentTimeSerialTx = millis();// Get snapshot of time
      DeltaTSerialTx = getDeltaT(CurrentTimeSerialTx, PreviousTimeSerialTx);
      if (DeltaTSerialTx > SerialTxInterval) {
        PreviousTimeSerialTx = millis();
        if (valuecount == 0) {
          DipSwitch4 = digitalRead(37); // CHECK LOGIC
          //DipSwitch4=1;
          if (!DipSwitch4) {
            OpMode[ForwardTxCount] = "0" + String(CurrentState); //OpMode = 0x //Testing
          }
          else {
            OpMode[ForwardTxCount] = "1" + String(CurrentState); //OpMode = 1x
          }
          refreshSerial();
          Serial.print("OpMode is Tx over Serial: ");
          Serial.print(OpMode[ForwardTxCount]); ///
          Serial7.print(OpMode[ForwardTxCount]);
          valuecount += 1;
        }
        else if (valuecount == 1) {
          Serial.print("DataType is Tx over Serial: ");
          Serial.print(DataType[ForwardTxCount]);
          Serial7.print(DataType[ForwardTxCount]);
          valuecount += 1;
        }
        else if (valuecount == 2) {
          Serial.print("DTG is Tx over Serial: ");
          Serial.print(DTG[ForwardTxCount]); //DTG will be transmitted to the LTE module
          Serial7.print(DTG[ForwardTxCount]);
          valuecount += 1;
        }
        else if (valuecount == 3) {
          Serial.print("TXdata1 is Tx over Serial: ");
          Serial.print(TXdata1[ForwardTxCount]);
          Serial7.print(TXdata1[ForwardTxCount]);
          valuecount += 1;
        }
        else if (valuecount == 4) {
          Serial.print("TXdata2 is Tx over Serial: ");
          Serial.print(TXdata2[ForwardTxCount]);
          Serial7.print(TXdata2[ForwardTxCount]);
          valuecount += 1;  //Keep adding to break the while loop
          TransmitNowFlag = 0;
          ForwardTxCount += 1;
          if (ForwardTxCount == TxValCount) { //This might need more logic
            ForwardTxCount = 0;
            TxValCount = 0;
            digitalWrite(11, LOW);
          }
        }//end else if(valuecount==4)
        Serial.print('\n');
        Serial7.print('\n');
      }//end if(Delta>Interval)
    }//end if(!TxVal)
  }//end while(valuecount<5)
  valuecount = 0; //Now we can reset valuecount
  //////////////////////////////
  //Email Content
 // EmailFlag=1;
 // String timenow=*p_Time;
 /* EmailField1[EmailCount]= String(*p_Date)+" at "+String(*p_Time)+", "+"the MCMD entered Shutdown Mode because the battery discharged below 10%";
  EmailField2[EmailCount]="Until it returns to Reduced Mode,";
  EmailField3[EmailCount]="no further data will be communicated by the MCMD";
  EmailType[EmailCount]=3;  //Will need to be set each time
  EmailOpMode[EmailCount]=3;  //Will need to be set each time
  EmailCount+=1;  //How many are saved
  Serial.println("Trying to TX");
  SerialEmailTx();*/
  ///////////////////////////////
  //While in ShutdownMode DO THIS LOOP ////
  while (chargelevelmAh < P25Threshold) { //while below 25% 1750mAh       //SLEEP MODE?
    delay(15000);
    BatteryLevel();
  }
  //===============================================================//
  eventstring = "Returning to Reduced Power Mode from Shutdown Mode";
  Events2CSV();
  Disolate();
  //ReducedMode(); // Return to Reduced Mode once charged above 25%
  return;
}

void SafetyMode() {
  // tristable relay
  Isolate();
  int LED_SAFETY_USE = 1; //This one needs to be used.
  int LED_SAFETY_ON = 0; //This one is currently OFF.
  LED_Needed = 4;

  PreviousState = CurrentState; //were in reduced
  CurrentState = 4; // 4=Safety Mode
  DataType[TxValCount] = 2;
  getDate();
  getTime();
  DTG[TxValCount] = String(*p_Date) + "," + String(*p_Time);
  //Serial.println(String(*p_Date));
  TXdata1[TxValCount] = "Entered Safety Mode";
 // Serial.print("MB Temp is: ");  
 // Serial.println(HighMBTemp);
  if (*pHighcount == 5) {
    TXdata2[TxValCount] = "Primary Current Above 1300A";

  }
  else if (HighMBTemp) { //Last State was Reduced Mode
    TXdata2[TxValCount] = "Measurement Board Temperature was Above 80C";
    //Serial.println(TXdata2);
  }
  Serial.println("Entering Safety Mode");
  TxValCount += 1;

  //Prep Email
    //Email Content
  //EmailFlag=1;
/*  EmailField1[EmailCount]= String(*p_Date)+" at "+String(*p_Time)+", ";
  if (*pHighcount == 5) {
    EmailField1[EmailCount] += "the MCMD recorded a High Current event";
    EmailField2[EmailCount]="Please ensure the MCMD is properly configured and the conductor is not misbehaving";
    EmailField3[EmailCount]="or the MCMD may not perform as intended";

  }
  else if (HighMBTemp) { //Last State was Reduced Mode
    EmailField1[EmailCount] += "the MCMD recorded an abnormally high internal temperature over 80 ͦC";
    EmailField2[EmailCount]="The MCMD must isolate itself from the";
    EmailField3[EmailCount]="conductor to allow the temperature to drop below 80 ͦC";
  }
  EmailType[EmailCount]=3;  //Will need to be set each time
  EmailOpMode[EmailCount]=3;  //Will need to be set each time
  EmailCount+=1;  //How many are saved
  Serial.println("Safety Email Prepped");
  SerialEmailTx();*/
  ////////////////////////////
 
  //60s delay
  DeltaTSafety = 0;
  PreviousTimeSafety = millis();
  Serial.print("Entering Safety Delay of: ");
  Serial.print(SafetyInterval/1000);
  Serial.println(" seconds");
  
  while (DeltaTSafety < SafetyInterval) { //SafetyInterval
    //Serial.println("Entered the safety loop"); //It does make it here
    CurrentTimeSafety = millis();// Get snapshot of time
    DeltaTSafety = getDeltaT(CurrentTimeSafety, PreviousTimeSafety);

    //==================LED SAFETY ==========================//
    DipSwitch3 = digitalRead(38); // CHECK LOGIC

  /*  Serial.print("Led Needed: ");
    Serial.println(LED_Needed);
    Serial.print("Led Actual: ");
    Serial.println(LED_Actual);
    Serial.print("Dip Switch: ");
    Serial.println(DipSwitch3);*/
    //delay(2000);

    if (!DipSwitch3 && (LED_Needed != LED_Actual)) { //Test Mode and a Change is needed

      //Serial.println("Made it HERE");
      PermaON = 1;
      digitalWrite(GREEN, LOW);
      digitalWrite(RED, LOW);
      digitalWrite(WHITE, HIGH);
      LED_Actual = 4;
      delay(2000);
    }

    if (LED_SAFETY_USE && DipSwitch3) {  //LED1ON means the LED1 must be turned ON/OFF
      if (!LED_SAFETY_ON) {
        digitalWrite(GREEN, LOW);
        digitalWrite(RED, LOW);
        digitalWrite(WHITE, HIGH);  //Turn ON.
        LED_SAFETY_ON = 1;        //This one is now ON.
        PreviousTime_LED_SAFETY = millis(); //Represents the time it went ON.
      }
      //Is it time to turn off?
      CurrentTime_LED_SAFETY = millis();
      DeltaT_LED_SAFETY = getDeltaT(CurrentTime_LED_SAFETY, PreviousTime_LED_SAFETY);

      //IF we go to a shutdown loop, we need to turn off the LED in that loop because it wont return here in time./////////////********************
      if (DeltaT_LED_SAFETY > Interval_LED_SAFETY) {
        digitalWrite(WHITE, LOW); //Turn OFF.
        PreviousTime_LED_SAFETY = millis();
        Serial.println("LED go OFF");
        LED_SAFETY_USE = 0; //This one doesnt need to be used anymore.
        LED_SAFETY_ON = 0; //Mark state as OFF
      }
    }
    //===========================================================

    while (valuecount < 5) {
      TxVal = digitalRead(12);
      delay(10);
      if (!TxVal){//TxVal) {    //Should be if(!TxVal)
        digitalWrite(11, HIGH);
        CurrentTimeSerialTx = millis();// Get snapshot of time
        DeltaTSerialTx = getDeltaT(CurrentTimeSerialTx, PreviousTimeSerialTx);
        if (DeltaTSerialTx > SerialTxInterval) {
          PreviousTimeSerialTx = millis();
          if (valuecount == 0) {
            DipSwitch4 = digitalRead(37); // CHECK LOGIC
            if (!DipSwitch4) {
              OpMode[ForwardTxCount] = "0" + String(CurrentState); //OpMode = 0x //Testing
            }
            else {
              OpMode[ForwardTxCount] = "1" + String(CurrentState); //OpMode = 1x
            }
            refreshSerial();
            Serial7.print(OpMode[ForwardTxCount]); ///
            Serial.print(OpMode[ForwardTxCount]);
            valuecount += 1;
          }
          else if (valuecount == 1) {
            Serial7.print(DataType[ForwardTxCount]);
            Serial.print(DataType[ForwardTxCount]);
            valuecount += 1;
          }
          else if (valuecount == 2) {
            Serial7.print(DTG[ForwardTxCount]); //DTG will be transmitted to the LTE module
            Serial.print(DTG[ForwardTxCount]);
            valuecount += 1;
          }
          else if (valuecount == 3) {
            //refreshSerial();
            //Here is what me and Tdog fix (close and open the Serial Port to clear Ram)
            Serial7.print(TXdata1[ForwardTxCount]);
            Serial.print(TXdata1[ForwardTxCount]);   //Entered Safety Mode

            valuecount += 1;
          }
          else if (valuecount == 4) {
            Serial7.print(TXdata2[ForwardTxCount]);
            Serial.print(TXdata2[ForwardTxCount]);
            valuecount += 1;  //Keep adding to break the while loop
            TransmitNowFlag = 0;
            ForwardTxCount += 1;

            if (ForwardTxCount == TxValCount) { //This might need more logic
              ForwardTxCount = 0;
              TxValCount = 0;
              digitalWrite(11, LOW);
            }
          }
          Serial.print('\n');
          Serial7.print('\n');
        }//end if
      }//end if(!TxVal)
    }//end while(valuecount<5)
  }//end while than 60 seconds
  valuecount = 0; //Now we can reset valuecount
  TXdata2[TxValCount] = "";
  TXdata1[TxValCount] = "";
  Serial.println("Monitoring Temp Control");
  TempControl();  //Call another function to wait for temp to drop.
  eventstring = "Returning from Safety Mode";  // both high current or temperature
  Events2CSV();
  *pHighcount = 0; //Reset High count
  HighMBTemp = 0;    //Must be reset after leaving safety mode

//////////////////////////////

 ///////////////////////////////


  // TransmitNowFlag = 1 ; //Must test that Safety event is sent over serial.
  Disolate();
  BatteryLevel();
  return;
}
// =============================================================//
void BatteryLevel() {

     chargelevelmAh=FunctionalThreshold + 100;


    //  Serial.print("Battery Level is: ");
    // Serial.println(chargelevelmAh);
    // if(chargelevelmAh > FunctionalThreshold && CurrentState != 1){
            // FunctionalMode();
       //    }*/

  /////////////////////////////////////////
  //To test shutdown mode stuff below
  //if(HOWMANY == 1){
  //set func threshold
  // HOWMANY+=1;
  //}
  //else if(HOWMANY==2){
  //  chargelevelmAh=
  /////////////////////////////////////////

 // float chargelevelmAh = LTC2943_ReadCharge();


 /*if(HOWMANY==0){
   chargelevelmAh=FunctionalThreshold+100;
   //HOWMANY+=1;
 }
 else {//if(HOWMANY==5){
 //chargelevelmAh=FunctionalThreshold+100;
 }*/
 
  if (chargelevelmAh > FunctionalThreshold && CurrentState != 1) {
    FunctionalMode();
    Serial.println("Now enterting FUNC");
  }
  else if (ShutdownThreshold < chargelevelmAh && chargelevelmAh <= FunctionalThreshold && CurrentState != 2 ) {
    Serial.println("Now entering REDUX");
    Serial.print("battery value is: ");
    Serial.println(chargelevelmAh);
    ReducedMode();
  }

  else if ((chargelevelmAh <= ShutdownThreshold) && (CurrentState != 3) ) {
    Serial.println("Now entering SHUT");
    ShutdownMode();
  }

  else if (chargelevelmAh >= P25Threshold && CurrentState == 3 ) {
    ReducedMode();
    Serial.println("Now enterting REDUX (CHARGED TO 25%)");
  }// end of if (chargelevelmAh > FunctionalThreshold && CurrentState != 1)
}
///////////////////////////////////////////////////////////////
//==============================================================//
void Analyze_Current() {
  if (current >= HighCThresh) {
    *pLowcount = 0; //Reset, no low current.
    *pHighcount += 1; //add count
    *pHealthycount = 0;
    if (*pHighcount == 5) {
      eventstring = "High Current Detected going into Safety mode!";
      SafetyMode();
    }// end if(*pHighcount == 5)                                                                                                               //////////////////////////////////////
  }//end if(current >= HighCThresh)

  else if (current < LowCThresh && !low_current_report) {
    Serial.println("Low Current");
    *pLowcount += 1; //Count how many times it is low
    /*Serial.print("Low Count: ");
      Serial.println(*pLowcount);*/
    *pHighcount = 0;
    *pHealthycount = 0 ;
    if (*pLowcount >= 5) { // && !report //If low for 60 samples (=60 seconds)
      //SendLowCurrentNotice
      *pReport = 1;
      DataType[TxValCount] = 2;
      getTime();
      getDate();
      DTG[TxValCount] = String(*p_Date) + "," + String(*p_Time);
      TXdata1[TxValCount] = "Low Current Detected";

      if (DipSwitch3) { //Normal Mode
        TXdata2[TxValCount] = " 60 seconds of current below 2A  ";
      }
      else {
        TXdata2[TxValCount] = " 60 seconds of current below 1A  ";
      }
      TransmitNowFlag = 1;
      TxValCount += 1;
      eventstring = " Low Current Event";      // replace with cases shudown, Reduced and functional mode
      Events2CSV();
      Serial.println("One Minute of Low Current");
    }
  }

  else if (current < LowCThresh && low_current_report) {
    //Wait until current rises above the low threshold.
    //*pLowcount +=1; //Dont need to keep counting as we've reported it already?
    *pHighcount = 0;
    *pHealthycount = 0 ;
    //Do nothing and go back to main loop to read current again.
  }
  else { //current is healthy
    *pLowcount = 0;
    *pHealthycount += 1;
    // set Report flag back to 0 (5 min of healthy current)
    if (*pHealthycount == 300) {
      *pReport = 0;
    }//end if
  }
}//end of Analyze_Current
//=================================================================//
void Analyze_MBTemp() {
  temperature();
  if (temperatureC > MaxTemp) { //change temp to temperatureC
    HighMBTemp = 1;
    eventstring = "The measurement board is over temperature";
    SafetyMode();
  }// end if
}//end Analyze_MBTemp
//=================================================================

////////////////////////////////////// Coulomb Counter Functions //////////////////////////////////////

float LTC2943_ReadCharge()
{
  union
  {
    uint8_t b[2];
    uint16_t w;
  } data;

  data.b[1] = LTC2943_Read(LTC2943_I2C_ADDRESS, LTC2943_ACCUM_CHARGE_MSB_REG);
  data.b[0] = LTC2943_Read(LTC2943_I2C_ADDRESS, LTC2943_ACCUM_CHARGE_LSB_REG);
  delay(50);

  float charge = (data.w * lsb); //Calculate mAh

  return (charge);
}

float LTC2943_ReadVoltage()
{
  union
  {
    uint8_t b[2];
    uint16_t w;
  } vdata;

  vdata.b[1] = LTC2943_Read(LTC2943_I2C_ADDRESS, LTC2943_VOLTAGE_MSB_REG);
  vdata.b[0] = LTC2943_Read(LTC2943_I2C_ADDRESS, LTC2943_VOLTAGE_LSB_REG);
  delay(50);
  float voltage = vdata.w * (23.6 / 65535);
  return (voltage);
}

float LTC2943_ReadCurrent()
{
  union
  {
    uint8_t b[2];
    uint16_t w;
  } cdata;

  cdata.b[1] = LTC2943_Read(LTC2943_I2C_ADDRESS, LTC2943_CURRENT_MSB_REG);
  cdata.b[0] = LTC2943_Read(LTC2943_I2C_ADDRESS, LTC2943_CURRENT_LSB_REG);
  delay(50);
  float current = cdata.w;
  current = (current - 32767) * (-2.4 / 32767);
  return (current);
}

float LTC2943_ReadTemp()
{
  union
  {
    uint8_t b[2];
    uint16_t w;
  } tdata;

  tdata.b[1] = LTC2943_Read(LTC2943_I2C_ADDRESS, LTC2943_TEMPERATURE_MSB_REG);
  tdata.b[0] = LTC2943_Read(LTC2943_I2C_ADDRESS, LTC2943_TEMPERATURE_LSB_REG);
  delay(50);
  float temp = tdata.w;
  temp = 510 * (temp / 65535) - 273.15;
  return (temp);
}

void LTC2943_setup()
{
  LTC2943_write(LTC2943_I2C_ADDRESS, LTC2943_ACCUM_CHARGE_MSB_REG, 0xA0);
  LTC2943_write(LTC2943_I2C_ADDRESS, LTC2943_ACCUM_CHARGE_LSB_REG, 0xD8);

  int8_t LTC2943_mode = LTC2943_AUTOMATIC_MODE | prescalar_mode | alcc_mode ;           //! Set the control register of the LTC2943 to automatic mode as well as set prescalar and AL#/CC# pin values.

  LTC2943_write(LTC2943_I2C_ADDRESS, LTC2943_CONTROL_REG, LTC2943_mode);   //! Writes the set mode to the LTC2943 control register
  Serial.println("MSB and LSB registers have been set to 100% charge and the LTC2943 is in automatic mode with ALCC disabled");
  delay(1000);
}

int8_t LTC2943_write(uint8_t address, uint8_t Register, uint8_t data)
{
  Wire.beginTransmission(address);
  Wire.write(Register);                       // memory address
  Wire.write(data);               // write 1 byte
  Wire.endTransmission();
  delay(10);
  return (0);
}


uint8_t LTC2943_Read(uint8_t address, uint8_t Register)
{
  uint8_t data;
  uint8_t memlen = 1;
  Wire.beginTransmission(address);         // slave addr
  Wire.write(Register);
  Wire.endTransmission(false);
  Wire.requestFrom(address, memlen);
  while (Wire.available()) {
    data = Wire.read();
  }
  delay(10);
  return (data);
}

////////////////////////////////////// End of Coulomb Counter Functions //////////////////////////////////////

//////////////////Special Email Tx Function //////////////////////////////////
void SerialEmailTx(){
  digitalWrite(11, HIGH); //Tell the Boron not to LTE Tx until all is transferred.
  while (valuecount < 5) {
        CurrentTimeSerialTx = millis();// Get snapshot of time
        DeltaTSerialTx = getDeltaT(CurrentTimeSerialTx, PreviousTimeSerialTx);
        if (DeltaTSerialTx > SerialTxInterval) {
          PreviousTimeSerialTx = millis();
          if (valuecount == 0) {
            DipSwitch4 = digitalRead(37); // CHECK LOGIC
            if (!DipSwitch4) {
              EmailOpMode[EmailIndex] = "0" + String(CurrentState); //OpMode = 0x //Testing
            }
            else {
              EmailOpMode[EmailIndex] = "1" + String(CurrentState); //OpMode = 1x
            }
            refreshSerial();
            Serial7.print(EmailOpMode[EmailIndex]); ///
            Serial.print(EmailOpMode[EmailIndex]);
            valuecount += 1;
          }
          else if (valuecount == 1) {
            Serial7.print(EmailType[EmailIndex]);
            Serial.print(EmailType[EmailIndex]);
            valuecount += 1;
          }
          else if (valuecount == 2) {
            Serial7.print(EmailField1[EmailIndex]); //DTG will be transmitted to the LTE module
            Serial.print(EmailField1[EmailIndex]);
            valuecount += 1;
          }
          else if (valuecount == 3) {
            //refreshSerial();
            //Here is what me and Tdog fix (close and open the Serial Port to clear Ram)
            Serial7.print(EmailField2[EmailIndex]);
            Serial.print(EmailField2[EmailIndex]);   //Entered Safety Mode
            valuecount += 1;
          }
          else if (valuecount == 4) {
            Serial7.print(EmailField3[EmailIndex]);
            Serial.print(EmailField3[EmailIndex]);
            valuecount += 1;  //Keep adding to break the while loop
            EmailIndex += 1;
            Serial.print("Email index is now: ");
            Serial.println(EmailIndex);
              if (EmailIndex == EmailCount) { 
                EmailIndex = 0;
                EmailCount = 0;
                EmailFlag=0;
                Serial.println("Reset Email Count Values");
                digitalWrite(11, LOW);
              }
          }
          Serial.print('\n');
          Serial7.print('\n');
          
          //Serial.print("Value Count is: ");
          // Serial.println(valuecount);
        }//end if
    }//end while(valuecount<5)
    valuecount=0;
    digitalWrite(11, LOW);
 }// end of SerialEmailTx

//////////////////////////////////End of Email Function/////////////////////////////////////////
//
//                      ^^^^Functions^^^^^

//==============================================================================
void setup() {
  Serial.begin(9600);
  pinMode(12, INPUT);
  pinMode(11, OUTPUT);
  while (!Serial) {
    ; // wait for serial port to connect.
  }
  Wire.setSDA(18);
  Wire.setSCL(19);
  Wire.begin();

  //Check that Coulomb Counter is found:
  Serial.print("Starting I2C scan...\n");
  Wire.beginTransmission(LTC2943_I2C_ADDRESS);
  switch (Wire.endTransmission())
  {
    case 0:
      Serial.printf("Coulomb counter connected at address: 0x%02X \n", LTC2943_I2C_ADDRESS);
      break;
    case 4:
      Serial.printf("Unknown error with coulomb counter at address: 0x%02X\n", LTC2943_I2C_ADDRESS);
      break;
    default:
      break;
  }
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  //====================================================
  // set the Time library to use Teensy 3.0's RTC to keep time
  setSyncProvider(getTeensy3Time);

  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("RTC has set the system time");
  }
  //===================================================
  ///////////Initializations/////////////////
  initRTC();
  ACS_init();
  //Enable heating pad
  digitalWrite(HeatingPad, LOW);  // Heater Active

}//end of void setup
//==============================================================================
/*
                      vvvvvvv Main Function vvvvvvvv
*/
//==============================================================================
void loop() {

  //=============2 sec LED Green====================//
  digitalWrite(GREEN, HIGH);
  delay(2000);
  digitalWrite(GREEN, LOW); // srq 5.2.0 POWER LED
  int LED_INIT_USE = 1; //This is for INITIALIZATION
  int LED_INIT_ON = 0; //This one is currently OFF.
  int LED_Init_Busy = 1;

  //====================================================//
  //Serial.println("FORMATINGGGGGG");
  SD.remove("EVENTS.csv"); //Still ok
  formatEventsCSV();
  ////////////////////////////////////////////////////////////
  //ShutdownMode();

  //////////////////////////////////////////////////////////
  while (1) {

    //===================Test Mode LED================================================//
    DipSwitch3 = digitalRead(38); // CHECK LOGIC

    if (DipSwitch3 && PermaON) { //If no longer test mode and PermaON is still 1, turn everything off
      digitalWrite(GREEN, LOW);
      digitalWrite(RED, LOW);
      digitalWrite(WHITE, LOW);
      digitalWrite(BLUE, LOW);
      PermaON = 0;
    }

    if (!DipSwitch3 && (LED_Needed != LED_Actual)) { //Test Mode and a Change is needed
      PermaON = 1;
      if (CurrentState == 1) {
        digitalWrite(WHITE, LOW);
        digitalWrite(RED, LOW);
        digitalWrite(GREEN, HIGH);
        LED_Actual = 1;
      }
      else if (CurrentState == 2) {
        digitalWrite(WHITE, LOW);
        digitalWrite(RED, HIGH); //Yellow
        digitalWrite(GREEN, HIGH);
        LED_Actual = 2;
      }
      else if (CurrentState == 3) {
        digitalWrite(WHITE, LOW);
        digitalWrite(GREEN, LOW);
        digitalWrite(RED, HIGH);
        LED_Actual = 3;
      }
      else if (CurrentState == 4) { //This case is not gonna happen because the device isolates itself in a loop
        digitalWrite(GREEN, LOW);
        digitalWrite(RED, LOW);
        digitalWrite(WHITE, HIGH);
        LED_Actual = 4;
      }
    }

    //==================^^^^=Test Mode LED==========^^^^======================================//
    //===================Test Mode Current Thresholds================================================//
    DipSwitch2 = digitalRead(39); //
    //DipSwitch3=1;
    if (DipSwitch2) { //Normal Mode
      LowCThresh = NormalLowCThresh;   //2A
      HighCThresh = NormalHighCThresh; //1300A
    }
    else {
      LowCThresh = TestingLowCThresh; //0A maybe should be 1 Amp
      HighCThresh = TestingHighCThresh; //5A
    }

    /*Serial.print("High Threshold: ");
      Serial.println(HighCThresh);

      Serial.print("Low Threshold: ");
      Serial.println(LowCThresh);*/
    //===================Test Mode Current Thresholds================================================//
    //===================Test Mode DataPoint Interval ================================================//
    DipSwitch1 = digitalRead(40); //
    //DipSwitch1=0;
    if (DipSwitch1) { //Normal Mode
      DataInterval = NormalDataInterval; //Every 5 minutes
    }
    else {
      DataInterval = TestingDataInterval; //Every 1 minute
    }
    /*Serial.print("Datapoint Interval is: ");
      Serial.println(DataInterval);*/

    /*Serial.print("Data Interval: ");
      Serial.println(DataInterval);*/
    //===================Test Mode Thresholds================================================//

    // wait until 1 sec before sampling => take sample
    CurrentTimeSample = millis();// Get snapshot of time
    DeltaTSample = getDeltaT(CurrentTimeSample, PreviousTimeSample);
    if (DeltaTSample > SampleInterval) {
      Samples();
      Analyze_MBTemp();
      Analyze_Current();
      PreviousTimeSample = millis();
      Serial.print("Current Sample (A): ");
      Serial.println(*p_current);
    }

    //Check Battery every 30s
    CurrentTimeBatteryCheck = millis();// Get snapshot of time
    DeltaTBatteryCheck = getDeltaT(CurrentTimeBatteryCheck, PreviousTimeBatteryCheck);
    if (DeltaTBatteryCheck > BatteryCheckInterval) {
      PreviousTimeBatteryCheck = millis();
      BatteryLevel(); //This should check the battery temp and change the mode accordingly.
      PreviousTimeBatteryCheck = millis();
    }

    // wait until 5 min => datapoint
    CurrentTimeData = millis();
    delay(50);
    //Serial.println(CurrentTimeData);
    DeltaTData = getDeltaT(CurrentTimeData, PreviousTimeData);
    if (DeltaTData > DataInterval) {  //Testing set to 30 seconds
      Samples2CSV();
      Average();
      Peak();
      Data2CSV();
      getTime();
      getDate();
      DTG[TxValCount]  = String(*p_Date) + "," + String(*p_Time); //Date&Time must be formatted as DTG "06/03/2021,11:46:54"
      TXdata1[TxValCount] = String(*p_avg); //
      TXdata2[TxValCount] = String(*p_peak); //
      DataType[TxValCount] = 1;         // Set Data type
      TransmitNowFlag = 1;  //Transmit to Serial when possible
      TxValCount += 1;
      //Removal of dataOUT.csv every two weeks
      datapointrows += 1;
      if (datapointrows == 2016) {
        SD.remove("dataOUT.csv");
        SD.remove("EVENTS.csv");
        formatDataCSV();
        datapointrows = 0;
        eventsrows = 0;
      }
      PreviousTimeData = millis();
      Serial.println("Delta Data");
    }



/*if(EmailFlag){
CurrentTimeEmail = millis();// Get snapshot of time
    DeltaTEmail = getDeltaT(CurrentTimeEmail, PreviousTimeEmail);
if (DeltaTEmail > EmailInterval) {
    //Serial.print("DeltaTEmail is: ");
    //Serial.println(DeltaTEmail);
    //Special Loop for emails
      TxVal = digitalRead(12);
      delay(10);
      if (!TxVal) {    //Should be if(!TxVal)
      digitalWrite(11, HIGH);
      PreviousTimeEmail = millis();
      SerialEmailTx();
      digitalWrite(11, LOW);
      EmailFlag=0;   
      }  
    }
}*/
    
    if (TransmitNowFlag == 1) { //&& TxValCount>=0){  //Why would I need this?
           // Serial.print("Starting Publish Count: ");
           // Serial.println(TxValCount);
      TxVal = digitalRead(12);
      Serial.print("TxVal is: ");
      Serial.println(TxVal);
      if (!TxVal) {    //Should be if(!TxVal)
      digitalWrite(11, HIGH);
        CurrentTimeSerialTx = millis();// Get snapshot of time
        DeltaTSerialTx = getDeltaT(CurrentTimeSerialTx, PreviousTimeSerialTx);
        if (DeltaTSerialTx > SerialTxInterval) {
          PreviousTimeSerialTx = millis();
          if (valuecount == 0) {
            DipSwitch4 = digitalRead(37); // CHECK LOGIC
            //DipSwitch4=1;
            if (!DipSwitch4) {
              OpMode[ForwardTxCount] = "0" + String(CurrentState); //OpMode = 0x TESTING
            }
            else {
              OpMode[ForwardTxCount] = "1" + String(CurrentState); //OpMode = 1x
            }
            refreshSerial();
            Serial.print("OpMode is Tx over Serial: ");
            Serial.print(OpMode[ForwardTxCount]); ///
            Serial7.print(OpMode[ForwardTxCount]);
            valuecount += 1;
          }
          else if (valuecount == 1) {
            Serial.print("DataType is Tx over Serial: ");
            Serial.print(DataType[ForwardTxCount]);
            Serial7.print(DataType[ForwardTxCount]);
            valuecount += 1;
          }
          else if (valuecount == 2) {
            Serial.print("DTG is Tx over Serial: ");
            Serial.print(DTG[ForwardTxCount]); //DTG will be transmitted to the LTE module
            Serial7.print(DTG[ForwardTxCount]);
            valuecount += 1;
          }
          else if (valuecount == 3) {
            Serial.print("TXdata1 is Tx over Serial: ");
            Serial.print(TXdata1[ForwardTxCount]);
            Serial7.print(TXdata1[ForwardTxCount]);
            valuecount += 1;
          }

          else if (valuecount == 4) {
            Serial.print("TXdata2 is Tx over Serial: ");
            Serial.print(TXdata2[ForwardTxCount]);
            Serial7.print(TXdata2[ForwardTxCount]);
            valuecount = 0;
            ForwardTxCount += 1;

            if (ForwardTxCount == TxValCount) { //This might need more logic
              TransmitNowFlag = 0;
              ForwardTxCount = 0;
              TxValCount = 0;
          // Serial.println("Done Publish Count: ");
           // Serial.print(TxValCount);
              //digitalWrite(11, LOW);
            }
          }// end of else if(valuecount==4)

          Serial.print('\n');
          Serial7.print('\n');
        }//end if(DeltaSerialTx>SerialTxInterval)
      }//end of if(TxVal)
      else {
      //Do nothing 
      }
      //}

      if(TransmitNowFlag==0){
         digitalWrite(11, LOW); 
      }
    }// end of if

    //Every 24 hours of consecutive reduced mode
    if (CurrentState == 2) {
      CurrentTimeReduced = millis();
      DeltaTReduced = getDeltaT(CurrentTimeReduced, PreviousTimeReduced);
      if (DeltaTReduced > ReducedInterval) {

        if (!DipSwitch4) {
          OpMode[TxValCount] = "0" + String(CurrentState); //OpMode = 0x TESTING
        }
        else {
          OpMode[TxValCount] = "1" + String(CurrentState); //OpMode = 1x
        }
        DataType[TxValCount] = 2;
        DTG[TxValCount] = String(*p_Date) + "," + String(*p_Time);
        TXdata1[TxValCount] = "Prolonged Operation in Reduce Power Mode";
        TXdata2[TxValCount] = "The battery charge has been below 50% for 24 hours";
        TransmitNowFlag = 1;
        eventstring = "24 consecutive hours in Reduced Power Mode";
        Events2CSV();
        // add transmission data // make sure the data transmission don't follow the dame delays* add another data transmission interval
        PreviousTimeReduced = millis();
        Serial.println("Prolonged Reduced Mode");
      }//end of if
    }// end of if


    //================ LED Functional==========================//
    if (LED_FUNCTIONAL_USE && DipSwitch3 && !LED_Init_Busy) {  //LED1ON means the LED1 must be turned ON/OFF

      if (!LED_FUNCTIONAL_ON) {
        digitalWrite(WHITE, LOW);
        digitalWrite(RED, LOW);
        digitalWrite(GREEN, HIGH);  //Turn ON.
        LED_FUNCTIONAL_ON = 1;        //This one is now ON.
        PreviousTime_LED_FUNCTIONAL = millis(); //Represents the time it went ON.
      }

      //Is it time to turn off?
      CurrentTime_LED_FUNCTIONAL = millis();
      DeltaT_LED_FUNCTIONAL = getDeltaT(CurrentTime_LED_FUNCTIONAL, PreviousTime_LED_FUNCTIONAL);

      if (DeltaT_LED_FUNCTIONAL > Interval_LED_FUNCTIONAL) {
        digitalWrite(GREEN, LOW); //Turn OFF.
        PreviousTime_LED_FUNCTIONAL = millis();
        Serial.println("LED go OFF");
        LED_FUNCTIONAL_USE = 0; //This one doesnt need to be used anymore.
        LED_FUNCTIONAL_ON = 0; //Mark state as OFF
      }
    }//end if
    //=========================================================//
    //================ LED Reduce ============================//
    if (LED_REDUCED_USE && DipSwitch3 && !LED_Init_Busy) {  //LED1ON means the LED1 must be turned ON/OFF

      if (!LED_REDUCED_ON) {
        digitalWrite(WHITE, LOW);
        digitalWrite(RED, HIGH); //Turn ON.
        digitalWrite(GREEN, HIGH); //Turn ON.
        LED_REDUCED_ON = 1;       //This one is now ON.
        PreviousTime_LED_REDUCED = millis(); //Represents the time it went ON.
      }

      //Is it time to turn off?
      CurrentTime_LED_REDUCED = millis();
      DeltaT_LED_REDUCED = getDeltaT(CurrentTime_LED_REDUCED, PreviousTime_LED_REDUCED);

      if (DeltaT_LED_REDUCED > Interval_LED_REDUCED) {
        digitalWrite(RED, LOW); //Turn ON.
        digitalWrite(GREEN, LOW); //Turn ON.
        PreviousTime_LED_REDUCED = millis();
        Serial.println("LED go OFF");
        LED_REDUCED_USE = 0; //This one doesnt need to be used anymore.
        LED_REDUCED_ON = 0; //Mark state as OFF
      }
    }
    //=========================================================//
    //================ Power and Initialization LED ============================//
    if (LED_INIT_USE && DipSwitch3) {  //LED1ON means the LED1 must be turned ON/OFF

      if (!LED_INIT_ON && !LED_INIT_Count) {  //On the first time, dont need to check time before turning back on.
        digitalWrite(WHITE, LOW);
        digitalWrite(RED, LOW);
        digitalWrite(GREEN, HIGH);  //Turn ON.
        PreviousTime_LED_INIT = millis(); //Represents the time it went ON.
        LED_INIT_ON = 1;        //This one is now ON.
      }

      else if (!LED_INIT_ON) {  //On the second time, need to check time before turning back on.
        CurrentTime_LED_INIT = millis();
        DeltaT_LED_INIT = getDeltaT(CurrentTime_LED_INIT, PreviousTime_LED_INIT); //Check how long its been OFF.
        if (DeltaT_LED_INIT > Interval_LED_INIT) {
          digitalWrite(GREEN, HIGH); //Turn ON.
          PreviousTime_LED_INIT = millis(); //Mark when it went ON
          //Serial.println("LED go ON");
          LED_INIT_ON = 1; //Mark LED as OFF, but not USE=0
        }
      }

      else if (LED_INIT_ON) {
        //Is it time to turn off?
        CurrentTime_LED_INIT = millis();
        DeltaT_LED_INIT = getDeltaT(CurrentTime_LED_INIT, PreviousTime_LED_INIT);

        if (DeltaT_LED_INIT > Interval_LED_INIT) {
          digitalWrite(GREEN, LOW); //Turn OFF.
          PreviousTime_LED_INIT = millis();
          Serial.println("LED go OFF");
          LED_INIT_ON = 0; //Mark LED as OFF, but not USE=0
          LED_INIT_Count += 1; //Count how many times the LED was turned ON and then OFF.
        }
      }

      if (LED_INIT_Count == LED_INIT_Cycles) {
        LED_INIT_USE = 0; //Stop using this LED
        LED_Init_Busy = 0;
      }
    }
    //===============================================================//

  }//end while

}//end of loop
// ========================END OF MAIN LOOP==============================//

//==============================================================================
/*
                      ^^^^Main Function^^^^^
*/
//==============================================================================
