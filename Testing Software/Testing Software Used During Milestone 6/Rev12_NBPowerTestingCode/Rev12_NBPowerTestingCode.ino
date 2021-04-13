
///////////////Library Includes////////////////////
// include the SD library:
#include <SD.h>
#include <SPI.h>
#include <TimeLib.h>

// set up variables using the SD utility library functions:
//Sd2Card card;
//SdVolume volume;
//SdFile root;

//==============================================================================
/*
                      vvvv Global Variables/Declarations vvvv
*/
//==============================================================================

float current_samples[350]; //
float * p_current_samples = current_samples;
float Max_Samples = 350; // ==Testing only=========//

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
float datapoints[2];  //Number of columns for the datapoint. (Average, Peak)
float * p_datapoints = datapoints;

String datastring = ""; // updated empty string ; will added samples
String * p_datastring = &datastring;


String eventstring = "" ; //empty
String * p_eventstring = &eventstring;
//========================================
//===========CURRENT SENSOR===============

//use pin A13 on Teensy
//#define Pin A13;
float RefVal = 3.3;


//These are the variable for the current sensor
float sensitivity = 1000.0 / 264.0; //1000mA per 264mV
float Vref = 327.49;   //You need test it !!!

// The on-board ADC is 10-bits
// Different power supply will lead to different reference sources
// example: 2^10 = 1024 -> 5V / 1024 ~= 4.88mV
//          unitValue= 5.0 / 1024.0*1000 ;
float unitValue = (3.3 / 1024.0) * 1000 ;
// 2^10 accuracy step size , multiply 1000 = mV

//  float voltage = unitValue * sensorValue;
float voltage;
float * p_voltage = &voltage;

long int sensorValue = 0;
long int * p_sensorValue = &sensorValue;

// sensor value is in bit value


float current = 1 ;  // later 1 if for low current for testing
float * p_current = &current;

//=========================================
//========Mesurement Temp Sensor===========
int tempPin = A9;
int tempReading;
float temperatureC;
float MaxTemp = 80.00; //Trigger Safety Mode at this temperature.

//================================================
//===========Operation Mode Testing  =====================//

/////////////////Simulation Cases//////////////////

float temp=10; //simulated to be higher than 85 
int FunctionalDelay=1440; // minutes
int ReducedDelay=720;
int TR3=0;    //Simulates operation mode, is the shutdown trace

/////////////////////////////////////////////////////////////
//Initialization of Global Variables
float LowCThresh=2000; // A
float HighCThresh=1300;//A
int lowcount=0;           //Amount of sample where the current<2A.
int low_current_report = 0; //Flag to indicate when low current event is reported.
int * pLowcount=&lowcount;    

int highcount=0;
int * pHighcount = &highcount; // for 5 s interval

int healthycount=0;
int * pHealthycount= &healthycount;

int * pReport=&low_current_report;
int LEDState=0;           //LED currently initialized to Functional Mode.
int * pLED= &LEDState; //Pointer to indicate which LED state is used. May need to be merged with Thomas's way.
float TxDelay=720; // Can be changed for testing, but should be used to initalize the tx delay time.
float * p2= &TxDelay;    //Pointer to the Delay between transmissions.

//////////////////////////Millis Variables/////////////////////////////////

const int MaxTime = 4294967295 ; //ms
const int SampleInterval= 800; //1 sec
unsigned long DeltaTSample;
unsigned long CurrentTimeSample=0;
unsigned long PreviousTimeSample =0;

const int DataInterval= 300000 ; // 5 min
unsigned long DeltaTData;
unsigned long CurrentTimeData=0;
unsigned long PreviousTimeData=0;

const int ReducedInterval= 180000; //86400000 ; // 24 hrs 
unsigned long DeltaTReduced;
unsigned long CurrentTimeReduced=0;
unsigned long PreviousTimeReduced=0;

const int FunctionalInterval= 43200000; // 12 hrs 
unsigned long DeltaTFunctional;
unsigned long CurrentTimeFunctional=0;
unsigned long PreviousTimeFunctional=0;

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
int relay = 6;



//==============================================================================
/*
                      vvvvvvv Initializations vvvvvvvvv
*/
//==============================================================================

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
//==============================================================================
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
    *p_sensorValue += analogRead(A13); // take 1 sample at time for resolution
    //delay(2); //If we're taking the average, which is yet to be converted to RMS, we should not delay, let the Teensy's clock speed be the limiter.
  }
  *p_sensorValue = *p_sensorValue / numValue;
  *p_voltage = unitValue * (*p_sensorValue);
//  Serial.print ("->Voltage: ");
//  Serial.print(*p_voltage);
//  Serial.println( " mV");
  // Calculate the corresponding current
  *p_current = (*p_voltage - Vref) * sensitivity;
  *p_current = *p_current - 10;
  *p_datastring += String(*p_current) + ",";

  // very time sample() is called updated sample is added to an array current samples
  *p_current_samples = *p_current;
  *p_current_samples++;

  // makes sure that there isn't more than 300 datapoints
  // String datastring = "";//create empty string for data
  //    for(int i=0;i<Max_Samples;i++){
  //          *p_current_samples=(*p_dummy) *200 ; // scale up values
  //
  //           //SD Card addition
  //            datastring += String(*p_current_samples) +",";
  //
  //    ////////////////Write to File//////////////////////
  //     Serial.print("Element ");
  //     Serial.print( i);
  //     Serial.print(" is ");
  //     Serial.println(*p_current_samples);
  //      delay(300);
  //          p_current_samples++;
  //          p_dummy++;
  //          //pointers go to the next
  //        }//end for
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
  //float temperatureC =(voltage_temp -0.5) * 100;
  temperatureC = (voltage_temp - 0.5) * 100;
  Serial.print("->Temperature: ");
  Serial.print(temperatureC);
  Serial.println("°C");
}
///////////////////////////////////////////////////////////

//------------------------------------------------------///

///////////////////////////////////////////////////////////
//================================================================================
//Name: TempControl
//Purpose: Function to be called when the MCMD senses the measurement board is over 80C.
void TempControl() {
    while (temp > MaxTemp) {

      Serial.println("Temperature is tooo high");
      delay(5000);
      
      temp = 60;// lower temp
      
      //Isolate CT
      //Send Emergency Message
      //Wait 1 minute
      //Check Temp Again
  
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

  //SD.remove("SAMPLES.csv"); //clear file first before adding anything to the csv file


  File dataFile = SD.open("SAMPLES.csv", FILE_WRITE);

  if (dataFile) {
    dataFile.println(*p_datastring);
   delay(100);
    Serial.println(*p_datastring);
    dataFile.close();

    // print to the serial port too:
    //Serial.println(*p_datastring);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening SAMPLES.csv");
  }

  datastring = "";

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

  // Summing
  float total = 0; // Needs to be a float too
  for (int i = 0; i < Max_Samples; i++) {
    total += *p_current_samples;
    p_current_samples++;
  }
  // p_dummy=dummy; //Reset p to the first avriable, otherwise it increases into random memory,
  p_current_samples = current_samples; //Reset pointers
  *p_sum = total;

  float size = sizeof(current_samples) / sizeof(current_samples[0]);
  float avg = sum / size;

  //=======increment p_datapoints =======//
  *p_datapoints = avg;
  p_datapoints++;
  *p_avg = avg;

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
  for (int i = 0; i < Max_Samples; i++) {
    if (current_samples[i] > current_samples[peak_index]) {
      peak_index = i; //max will store the maximum values of the current samples
    } // end if
  } //end for

  //  peak_val = current_samples[peak_index];  //made a pointer switch here
  *p_peak = current_samples[peak_index];
  Serial.print("The peak current is ");
  Serial.print(*p_peak);
  Serial.println(" mA");



  //=======decrement pointer p_datapoints =======//
  //  *p_datapoints = peak_val;  //Ken Commented out
  *p_datapoints = *p_peak;
  p_datapoints--;
  return;
}//end Peak
///////////////////////////////////////////////////////////

//------------------------------------------------------//

///////////////////////////////////////////////////////////
//Name: Data2CSV
//Purpose: With use of pointers, this function places all the
///appropriate pointers within a string to be appended to dataOUT.csv.
void Data2CSV() {

  // call Time and Date
  getTime();
  getDate();

  // SD.remove("dataOUT.csv");

  datastring = "" ; //empty
  datastring = String(*p_Device_Name) + "," + String(*p_Date) + "," + String(*p_Time) + "," + String(*p_avg) + "," + String(*p_peak); // for dummy 1
  // print to the serial port too:
  //                              Serial.println(datastring);
  File dataFile = SD.open("dataOUT.csv", FILE_WRITE);

  if (dataFile) {
    dataFile.println(datastring);
    delay(100);

    dataFile.close();

    // print to the serial port too:
    Serial.println(datastring); // means it is true
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening dataOUT.csv");
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

    // print to the serial port too:
    Serial.println(datastring); // means it is true
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening Events.csv");
  }


}//end of formatEventsCSV




//========================================================

void Events2CSV() {

  // call Time and Date
  getTime();
  getDate();


  // SD.remove("dataOUT.csv");


  datastring = "" ; //empty
  datastring = String(*p_Device_Name) + "," + String(*p_Date) + "," + String(*p_Time) + ","  ; // add eventstring

  datastring += eventstring;
  
  // print to the serial port too:
  //                              Serial.println(datastring);
  File dataFile = SD.open("Events.csv", FILE_WRITE);

  if (dataFile) {
    dataFile.println(datastring);
    delay(100);

    dataFile.close();

    // print to the serial port too:
    Serial.println(datastring); // means it is true
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening Events.csv");
  }

  datastring = "" ; //empty

  eventstring = "" ; //empty

  
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

////////////////////////////////////////////////////////////////
// MCMD Operational Modes
// Purpose: Set all required pointers to change operation modes.
// Input: None
// Output: None
//
void FunctionalMode(){
  PreviousTimeFunctional =millis();
  *pLED=1;
  *p2=FunctionalDelay;
  return;
}

void ReducedMode(){
  PreviousTimeReduced =millis();
  *pLED=2;
  *p2=ReducedDelay;
  return;
}

void ShutdownMode(){
  *pLED=3;

  eventstring = "Entering Shutdown Mode";
  Events2CSV();

  
    while(TR3 == 0){   //while below 25% //Will have to be updated based on our new SMC.
   
    //do nothing, except charge.

      ///////////////////Test Case Only//////////////////
       delay(2000); //delay to simulate waiting      //
       TR3=1;                                        //
       Serial.print("Battery has charged above 25%");//
      ///////////////////////////////////////////////////
    }

    eventstring = "Returning to Reduced Power Mode from Shutdown Mode";
    Events2CSV();
    ReducedMode(); // Return to Reduced Mode once charged above 25%
  return;
}

void SafetyMode(){
  //switch(6);     //Hard coded the flash so it returns to its last state after.
  //SendSafetyMessage();
    Serial.println("Entering Safety Mode");
  delay(6000);  //60s delay (Stepped down to 6s for TinkerCAD

       // trip bistable relay
       Isolate();
      Events2CSV();
  Serial.println("Monitoring Temp Control");  
  TempControl();  //Call another function to wait for temp to drop.

  eventstring = "Returning from Safety Mode";  // both high current or temperature
  Events2CSV();
 *pHighcount = 0; //Reset High count
 
}

// =============================================================//
//==============================================================//
          void Analyze_Current(){
          
               if(current >= HighCThresh){              
                 *pLowcount=0;  //Reset, no low current.
                 *pHighcount+= 1; //add count
                 *pHealthycount=0;
          
                 if(*pHighcount == 5){                
                  eventstring = "High Current Detected going into Safety mode!";   
                  SafetyMode();
                  
               }// end if
                    
                  ///////////TEst Case Only ////////////
                    Serial.println("High Current"); //
                                
                  //////////////////////////////////////
                }//end if
              
              else if(current < LowCThresh && !low_current_report){
                  Serial.println("Low Current");
                  *pLowcount +=1; //Count how many times it is low                                         
                  *pHighcount=0;                                 
                  *pHealthycount=0 ;                   
                  Serial.println(*pLowcount); //Track how many consecutive samples were too low
               
                    if(*pLowcount >= 60){  // && !report //If low for 60 samples (=60 seconds)
                  //SendLowCurrentNotice
                  *pReport = 1;
         
                           // determine when to set Report back to 0 (5 min of healthy current)
                           
                      eventstring = " Low Current Event !";      // replace with cases shudown, Reduced and functional mode
                      Events2CSV();
                      
                     
                      Serial.println("One Minute of Low Current");
                    // delay(2000);
                  }
              }
               
             else if(current < LowCThresh && low_current_report){
                   //Wait until current rises above the low threshold.
                  
                  //*pLowcount +=1;
                  *pHighcount=0;
                 *pHealthycount=0 ;
                  //Read current
                    Serial.println("Incident Reported");   // change to report once*****
                    delay(50); //
                  
              }
               
              else{  //current is healthy
               *pLowcount=0;
               *pHealthycount+=1;
                  Serial.println("Current is healthy");
                    //delay(2000);
                  
                 // set Report flag back to 0 (5 min of healthy current)
                  if(*pHealthycount == 300){
                   *pReport=0;
                  }//end if
                    
              }
                
          }//end of Analyze_Current

//=================================================================//
 void Analyze_MBTemp(){
   // temperature(); 
    if(temp>MaxTemp){  //change temp to temperatureC
         eventstring = "The measurement board is over temperature";
        SafetyMode();
      }// end if
  
 }//end Analyze_MBTemp



//=================================================================

void Isolate(){

                        int sense = digitalRead(7); // if sense =1 means it is already isolate
                      if (sense == 0) {
                        digitalWrite(relay, HIGH); //pulse
                        delay(100);
                        digitalWrite(relay, LOW);
                      }
                      
}



void Disolate(){

                        int sense = digitalRead(7); // if sense=1 means it is already isolate
                      if (sense == 1) {
                        digitalWrite(relay, HIGH); //pulse
                        delay(100);
                        digitalWrite(relay, LOW);
                      }
  
}

//==========================================================
//=======================================================
//function returns DeltaT time
unsigned long getDeltaT(unsigned long CurrentReading,unsigned long PreviousReading){
 
  //unsigned long DeltaT;
  
  if(CurrentReading<PreviousReading){
    // Time Elaspe 
   unsigned long  DeltaT = (MaxTime - PreviousReading)+CurrentReading;
   return(DeltaT);
  }

  else{
 unsigned long   DeltaT =CurrentReading - PreviousReading;
 return(DeltaT);
  }


}//end of getDeltaT

//==============================================================================
/*
                      ^^^^Functions^^^^^
*/
//==============================================================================


void setup() {
                //  //relayinitial
                //  pinMode(relay, OUTPUT);
                //  digitalWrite(relay, LOW);
                //  int sense = digitalRead(7);
                //  if (sense == 1) {
                //    digitalWrite(relay, HIGH);
                //    delay(100);
                //    digitalWrite(relay, LOW);
                //  }
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect.
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
}//end of void setup



//==============================================================================
/*
                      vvvvvvv Main Function vvvvvvvv
*/
//==============================================================================
void loop() {
 
  Serial.println("FORMATINGGGGGG");
  SD.remove("EVENTS.csv");
  formatEventsCSV();
////////////////////////////////////////////////////////////
  ShutdownMode();

//////////////////////////////////////////////////////////
 while(1) {
 
      // wait until 1 sec before sampling => take sample
      CurrentTimeSample = millis();// Get snapshot of time
      DeltaTSample=getDeltaT(CurrentTimeSample,PreviousTimeSample);
        if(DeltaTSample > SampleInterval){
          //do something
             Samples();
             Analyze_MBTemp();
             Analyze_Current(); 
          PreviousTimeSample = millis();
          Serial.println("Delta Sample");
        }
      // wait until 5 min => datapoint
      CurrentTimeData = millis();
      delay(50);
      Serial.println(CurrentTimeData);
       DeltaTData=getDeltaT(CurrentTimeData,PreviousTimeData);
          if(DeltaTData > DataInterval){
          //do something
            Samples2CSV();
            Average();
            Peak();
            Data2CSV();
            // Data2Particle(){
            getTime
            getDate 
            String DTG = String() +","+String()
             Average();
            Peak();
          PreviousTimeData = millis();
           Serial.println("Delta Data");
        }
      // wait until 24 hrs => 

       if(1){ // replace 1 with a variable that will determine if we are in the state or not
        CurrentTimeReduced = millis();
         DeltaTReduced=getDeltaT(CurrentTimeReduced,PreviousTimeReduced);
         
            if(DeltaTReduced > ReducedInterval){
            //do something
              eventstring = "24 consecutive hours in Reduced Power Mode";
              Events2CSV();
               // add transmission data // make sure the data transmission don't follow the dame delays* add another data transmission interval
            PreviousTimeReduced = millis();
             Serial.println("Prolonged Reduced Mode");
          }//end of if 
       }// end of if 
       
                  /////////////////
                  //Transmission 12 hr
                  
                  //Transmission 24 hr
                  ///////////////////////////////////////
    }//end while

}//end of loop
// ========================END OF MAIN LOOP==============================//

//==============================================================================
/*
                      ^^^^Main Function^^^^^
*/
//==============================================================================
