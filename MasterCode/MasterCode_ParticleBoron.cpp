//This revision is a complete code of all content for the Boron. 
//The latest update adds the ability to determine if the OpMode has changed, and therefore the TxInterval


//Each time the Teensy sends Data over Serial, the following fields MUST be transmitted. 
//Datapoints 
//1) OpMode=_; Integer for current mode 
//2) DataType=1;    
//3) AvgRms = "___"
//4) PeakRMS = "___"

//Events 
//1) OpMode=_; Integer for current mode 
//2) DataType=2;
//3) EventType= "Went from ___ Mode to ___ Mode"
//4) Event Prompt="Battery ___"


//Works with Arduino Code: "DataTransfer5"

//Constant Label For CSV Files
const char * DTGColumn = "Date/Time Recorded" ;
// Global variables
////////////////////////Reading Data From the Arduino///////////////////////////
const size_t READ_BUF_SIZE = 64;
char readBuf[READ_BUF_SIZE];
size_t readBufOffset = 0;

int ColumnCount=0;   //Which "column" does this belong to

String DataType;    //What type of data is being Tx (Datapoints, Events)
int doneWriting=0;  //Finished uploading everything in the arrays to IoT
int doneReading=0;  //Finished reading everything from the Teensy                   //This is not currently being used 

////////////Arrays to save Datapoints/Events until Tx at EOD./////////////////
//DataArray
String DTG_Array[250];  // ~144 day + spares 
String AvgRMS_Array[250];
String PeakRMS_Array[250];
int stampindex=0;
//EventsArray
String DTGEvent_Array[250];
String EventType_Array[250];
String EventPrompt_Array[250];
int eventindex=0;
/////////////////////////////////////////////////////////////////////////////////////
//Op Modes on Particle
String OpMode; //Safety Mode?
int TxAllowed=1;            //Initiliazed to 1 for Tetsing
//These variables are not currently used. ///////////*********************************
int FunctionalMode = 1; //Maybe we should initialize to FunctionalMode???????????????
int ReducedMode = 0;
int ShutdownMode = 0;
int SafetyMode = 0;	
/////////////////////////////////////////////////////////////////////////////////////
//Timer Variables for Resetting Serial Read 
const int SerialResetInterval= 10000 ; // 10 seconds without data on serial line means reset the ColumnCount
unsigned long DeltaTSerialTx;
unsigned long CurrentTimeSerialTx=0;
unsigned long PreviousTimeSerialTx=0;
/////////////////////////////////////////////////////////////////////////////////////
//int i=0; //Being used to format the spreadsheets on our first publish 
int PublishIndex=0;    //How many datapoints are required to be send 

int Datapointrowcount=0;      //How many rows are in the datapoint spreadsheet 
int Eventrowcount=0; //How many rows are in the events spreadsheet 

int datapointcount=0; //How many datapoints are saved on the Boron for Tx at EOD
int eventcount=0;     //How many events are saved on the Boron for Tx at EOD
/////////////////////////////////////////////////////////////////////////////////////

////////////////TimeTracking///////////////////////////////////////////////////////
const int MaxTime = 4294967295 ; //ms
unsigned long SendtoIoTInterval= 43200000 ; // ~1 min  WILL NEED TO BE EVERY 12/24HOURS set by the OpModes function once uncommented
unsigned long DeltaTSendtoIoT;
unsigned long CurrentTimeSendtoIoT=0;
unsigned long PreviousTimeSendtoIoT=0;
///////////////////////////////////////////////////////////////////////////////////
// Forward declarations
//void processBuffer();
//////////////////////////////////////////////////////////////////////////////////
void setup() {
    Serial.begin(9600);         //For PuTTy Terminal
    Time.zone(-4);
    Serial1.begin(9600);        //For Teensy Tx/Rx
    pinMode(D8,OUTPUT);
    pinMode(D1,OUTPUT);
  // Subscribe to the integrations required
  Particle.subscribe("hook-response/MCMDDatapoints", myHandler, MY_DEVICES);
  Particle.subscribe("hook-response/MCMDStatus", myHandler, MY_DEVICES);
}
///////////////////////////////FUNCTIONS/////////////////////////////////////

///////////////////////////////Cell Module/////////////////////////////////////
void CellOFF(){
        Cellular.off();
        digitalWrite(D1,LOW);
        delay(5);
}
void CellON(){
    Cellular.on();
   // Serial.println(Time.timeStr());
   digitalWrite(D1,HIGH);
    Serial.println("Cellular Module Turning ON");
    while(!Cellular.ready()){
        //Cellular Initializing
        digitalWrite(D8,HIGH);
        delay(20);
        digitalWrite(D8,LOW);
        delay(80);
    }
    digitalWrite(D8,HIGH);  //Cell TX IN USE 
	delay(11000);    //Delay is required to be very high once cellular is technically connected before it can publish properly. 
    //Serial.println(Time.timeStr());
    Serial.println("Cellular Module Turned ON ");
}
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////getDeltaT////////////////////////////////////////////
//function returns DeltaT time
unsigned long getDeltaT(unsigned long CurrentReading,unsigned long PreviousReading){
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
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void FormatCSVStatus(){
    
    const char * EventColumn1 = "Event Type" ;    
    const char * PromptColumn1 = "Reason for Event" ;    
    char eventreport[256];       // Character array for the snprintf Publish Payload
    snprintf(eventreport, sizeof(eventreport),
       "{\"DTG\":\"%s\"         ,    \"Event\":\"%s\"     ,  \"Prompt\":\"%s\"}" ,  // this is what the integration field needs in its entries
          DTGColumn             ,     EventColumn1          ,       PromptColumn1 )   ;

    Particle.publish("MCMDStatus", eventreport , PRIVATE);
    delay(1250);
    Eventrowcount+=1;
}

//Formatting only occurs when a new spreadsheet is created 
void FormatCSVDatapoints(){ 

    const char * AvgRMSColumn = "Average RMS Current" ;    
    const char * PeakRMSColumn = "Peak RMS Current" ;    
        char HeaderFile[256];       // Character array for the snprintf Publish Payload
    snprintf(HeaderFile, sizeof(HeaderFile),
    // ThingSpeak Field #1  , ThingSpeak Field #2   ,  
       "{\"DTG\":\"%s\"  ,    \"AvgRMS\":\"%s\"     ,  \"PeakRMS\":\"%s\"}" ,  // this is what the integration field needs in its entries
           DTGColumn     ,      AvgRMSColumn        ,    PeakRMSColumn          )   ;
            
    Particle.publish("MCMDDatapoints", HeaderFile , PRIVATE);
    delay(1250);
    //i++;
    Datapointrowcount++;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//This is to write an event confirming that the 12/24hr Tx has been succeeded 
void DailyDumpSuccess(){
    //Confirm Data uploaded with a report///////////////////////////////////
    const char * EventColumn = "Data Uploaded" ;    
    const char * PromptColumn = "Schedule Data Upload" ; 
     DTG_Array[1]=Time.timeStr();//"00/05/2021,10:16:23";
    int nDTG = DTG_Array[1].length();
    char char_arrayDTG[nDTG + 1];
    strcpy(char_arrayDTG, DTG_Array[1].c_str());
    
    char Event[256];   
    snprintf(Event, sizeof(Event),
    // ThingSpeak Field #1  , ThingSpeak Field #2   ,  
       "{\"DTG\":\"%s\"  ,    \"Event\":\"%s\"     ,  \"Prompt\":\"%s\"}" ,  // this is what the integration field needs in its entries
           char_arrayDTG ,      EventColumn        ,    PromptColumn         )   ; //char_arrayDTG
    delay(500);
    Particle.publish("MCMDStatus", Event , PRIVATE);
    delay(1250);
}
////////////////////////////////////////////////////////////////////////////////////////////////
void OpModes(){         //0=Testing , 1=Normal
	
	if(OpMode=="01"){       //Testing Functional
		//Functional Mode
		SendtoIoTInterval = 70000; // (45seconds) some amount of testing time 
		TxAllowed = 1;
		FunctionalMode = 1;	//are not currently being used 
    }
	else if(OpMode=="11"){		//Normal Functional  
		//Functional Mode
		SendtoIoTInterval = 43200000; // 12 Hours
		TxAllowed = 1;
		FunctionalMode = 1;	//are not currently being used 
    }
	else if(OpMode=="02"){  //Testing Reduced
		//ReducedPower Mode
		SendtoIoTInterval = 240000; // 4 min
		TxAllowed = 1;
		ReducedMode = 1;
	}
	else if(OpMode=="12"){  //Normal Reduced
		//ReducedPower Mode
		SendtoIoTInterval =86400000; // 24 Hours 
		TxAllowed = 1;
		ReducedMode = 1;
	}
	else if(OpMode=="03" || OpMode=="13"){	//These next two do not require a testing mode to go with them. 
		//Shutdown Mode
		// No Tx Delay, but must prevent TX
		ShutdownMode = 1;
		TxAllowed = 0;
	}
    else if(OpMode=="04" || OpMode=="14"){
		SafetyMode = 1;
		TxAllowed = 1;          //Might need to be set to 1 since we can technically publish in safety mode, just not sample. 
	}
}

/////////////////////////////////PROCESS INCOMING DATA////////////////////////////////////////////////
//This function will take a character array and save it as a sting within either datapoints or events
void processBuffer() {      //Will not transmit on time if serial is still available, currently not a problem
    
	if(ColumnCount==0){
		OpMode=String(readBuf);
        Serial.print("Operation Mode: ");
        Serial.println(OpMode);
        ColumnCount+=1;
		OpModes();                                ///**********************************************************************

	/*	Serial.print("Column Count is: ");
		Serial.println(ColumnCount);*/
	}
    else if(ColumnCount==1){		//This needs to be an elseif, otherwise it will increment columncount and immediately do this. 
        DataType=String(readBuf);
        Serial.print("Data Type: ");
        Serial.println(DataType);
		ColumnCount+=1;
    }
    
	
	else if(DataType=="1"){              //
	    if(ColumnCount==2){
	    	DTG_Array[stampindex]=String(readBuf);
	    	Serial.println("Newest DTG");
		    Serial.println(DTG_Array[stampindex]);
		    ColumnCount+=1;
	    }
	    else if(ColumnCount==3){
		    AvgRMS_Array[stampindex]=String(readBuf);
	    	Serial.println("Newest Avg");
		    Serial.println(AvgRMS_Array[stampindex]);
		    ColumnCount+=1;
	    }
	    else if(ColumnCount==4){
		    PeakRMS_Array[stampindex]=String(readBuf);
	    	Serial.println("Newest Peak");
		    Serial.println(PeakRMS_Array[stampindex]);
		    ColumnCount=0;   //Next value will be a DTG
		    stampindex++;   //Saved as next datapoint 
		    doneReading=1;  //We only read 1 datapoint every 5 minutes, we are done after reading the peak 
		    datapointcount+=1;  //How many will need to be sent at EOD. 
		    Serial.print("Data Row Count: ");
		    Serial.println(datapointcount);
	    }
	}
	else if(DataType=="2"){
	    //Enter Events content here 
	    if(ColumnCount==2){
	    	DTGEvent_Array[eventindex]=String(readBuf);
	    	//Serial.println("Latest DTG");
		    Serial.println(DTGEvent_Array[eventindex]);
		    ColumnCount+=1;
	    }
	    else if(ColumnCount==3){
		    EventType_Array[eventindex]=String(readBuf);
	    	//Serial.println("Latest Avg");
		    Serial.println(EventType_Array[eventindex]);
		    ColumnCount+=1;
	    }
	    else if(ColumnCount==4){
		    EventPrompt_Array[eventindex]=String(readBuf);
	    	//Serial.println("Latest Peak");
		    Serial.println(EventPrompt_Array[eventindex]);
		    ColumnCount=0;   //Next value will be a DTG
		    eventindex++;   //Saved as next datapoint 
		    doneReading=1;  //We only read 1 datapoint every 5 minutes, we are done after reading the peak 
		    eventcount+=1;  //How many will need to be sent at EOD. 
		    Serial.print("Event Row Count: ");
		    Serial.println(eventcount);
	    }
	}
}       

////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////MAIN FUNCTION////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
//This apparently cannot be a global declaration
////////////////////////////////////////////////////////////////////////////////////////////////
//Do loop once to format a new CSV  (Note that it will eventually add DTG to a spreadsheet, 
// and so everytime it boots a new spreadsheet is created) This allows it to work smoothly.
if(Datapointrowcount==0){
    FormatCSVDatapoints(); //Do once every 2000 columns
}
if(Eventrowcount==0){
    FormatCSVStatus(); //Do once every 2000 columns
    //delay(1250);
    CellOFF();
}

//////////////////////////////Read in Data when Serial is available////////////////////////////////////////
//This function should be the only one without a condition, so it only ever wants to do this, once done, it do another part. 
	while(Serial1.available()) {
	    PreviousTimeSerialTx=millis();
		if (readBufOffset < READ_BUF_SIZE) {
			char c = Serial1.read();
			if (c != '\n') {
				// Add character to buffer
				readBuf[readBufOffset++] = c;
			}
			else {
			     //Serial.println(DeltaTSendtoIoT);
				// End of line character found, process line
				readBuf[readBufOffset] = 0;
				//Serial.println("Newest Incoming Datapoint");
		    	//Serial.println(readBuf);
		    	processBuffer();
				readBufOffset = 0;
			}
		}
		else {
			Serial.println("readBuf overflow, emptying buffer");
			readBufOffset = 0;
		}
	}
//After reading a few, but not 5 value, if nothing happens for 10 seconds, it must mean that something was interrupted. 
//We will reset the column count for the next Serial Tx.
if(!Serial.available() && ColumnCount!=0){	
	CurrentTimeSerialTx = millis();
	//delay(50);
	DeltaTSerialTx=getDeltaT(CurrentTimeSerialTx,PreviousTimeSerialTx);
	//Serial.println(DeltaTSerialTx);

		if(DeltaTSerialTx > SerialResetInterval){ 
			ColumnCount=0; 									//Reset ColumnCount for next Tx, but must include the OpMode as well. 
		}
}

////////////////////////////////////////////////////////////////////////////////////////
//Sending Datapoints here
 CurrentTimeSendtoIoT = millis();
 delay(50);
 DeltaTSendtoIoT=getDeltaT(CurrentTimeSendtoIoT,PreviousTimeSendtoIoT);
 Serial.println(DeltaTSendtoIoT);
if(TxAllowed){
    if(DeltaTSendtoIoT > SendtoIoTInterval){        //Will print both the datapoints and the event saved locally. 
    	
    	if(datapointcount || eventcount){
    	CellON();//Turn ON the cellular modem. This will take ~15 seconds
    	Particle.publish("Buffer","This one might get lost");
    	}
    	stampindex=0;
        while(datapointcount>0){             //Daily transfer time ADD THE COUNT FOR TX
        
        //Format as character arrays to be formatted in a parsable format
        int nDTG = DTG_Array[PublishIndex].length();
        char char_arrayDTG[nDTG + 1];
        strcpy(char_arrayDTG, DTG_Array[PublishIndex].c_str());
        
        int nAvgRMS = AvgRMS_Array[PublishIndex].length();
        char char_arrayAvgRMS[nAvgRMS + 1];
        strcpy(char_arrayAvgRMS, AvgRMS_Array[PublishIndex].c_str());
        
        int nPeakRMS = PeakRMS_Array[PublishIndex].length();
        char char_arrayPeakRMS[nPeakRMS + 1];
        strcpy(char_arrayPeakRMS, PeakRMS_Array[PublishIndex].c_str());
        
            char datapoint[256];       // Character array for the snprintf Publish Payload
            snprintf(datapoint, sizeof(datapoint),
            "{\"DTG\":\"%s\"  ,    \"AvgRMS\":\"%s\"     ,  \"PeakRMS\":\"%s\"}" ,  // this is what the integration field needs in its entries
                char_arrayDTG ,      char_arrayAvgRMS    ,       char_arrayPeakRMS         )   ;
            
            delay(1500);
            Serial.println(datapoint);
            Particle.publish("MCMDDatapoints", datapoint , PRIVATE);
            delay(1500);
            PublishIndex++;
            Datapointrowcount++; //For spreadsheet row counting
    		//Reset when spreadsheet is full
            if(Datapointrowcount==2000){
                delay(1000);
                FormatCSVDatapoints();
                delay(1000);
                Datapointrowcount=0;
            }//end if(rowcount==2000)
    
            datapointcount--;
    		Serial.print("Datapoint Row Count: ");
    		Serial.println(datapointcount);
        } //end while(datapointcount>0)
        PublishIndex=0;                             //PublishIndex is index for sending values from saved arrays
       //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
        while(eventcount>0){            //Is within if(DeltaTSendtoIoT > SendtoIoTInterval) loop
            //print events
            int nDTGEvent = DTGEvent_Array[PublishIndex].length();
            char char_arrayDTGEvent[nDTGEvent + 1];
            strcpy(char_arrayDTGEvent, DTGEvent_Array[PublishIndex].c_str());
        
            int nEventType = EventType_Array[PublishIndex].length();
            char char_arrayEventType[nEventType + 1];
            strcpy(char_arrayEventType, EventType_Array[PublishIndex].c_str());
        
            int nEventPrompt = EventPrompt_Array[PublishIndex].length();
            char char_arrayEventPrompt[nEventPrompt + 1];
            strcpy(char_arrayEventPrompt, EventPrompt_Array[PublishIndex].c_str());
        
            char Event[256];   
            snprintf(Event, sizeof(Event),
        // ThingSpeak Field #1  , ThingSpeak Field #2   ,  
           "{\"DTG\":\"%s\"    ,    \"Event\":\"%s\"       ,    \"Prompt\":\"%s\"}" ,  // this is what the integration field needs in its entries
            char_arrayDTGEvent ,      char_arrayEventType  ,    char_arrayEventPrompt         )   ; //char_arrayDTG
            
            Serial.println(Event);
            Particle.publish("MCMDStatus", Event , PRIVATE);
            delay(1500);
            PublishIndex++;
            Eventrowcount++; //For spreadsheet only
    		//Reset when spreadsheet is full
            if(Eventrowcount==2000){
                delay(1000);
                FormatCSVStatus();
                delay(1000);
                Eventrowcount=0;
            }//end if(rowcount==2000)
    
            eventcount--;
    		Serial.print("Event Row Count: ");
    		Serial.println(eventcount);
            
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    	PreviousTimeSendtoIoT=millis();
        doneWriting=1;  //Data Tx is complete
        doneReading=0;  //Ready to read again
        PublishIndex=0; //PublishIndex is index for sending values from saved arrays
        digitalWrite(D8,LOW);
    }//end if(DeltaTSendtoIoT > SendtoIoTInterval)
}//end if(TxAllowed)
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//Event: Done Uploading Datapoints
	if(doneWriting){
        DailyDumpSuccess();
        doneWriting=0;
        CellOFF(); //Turn off the cellular modem until it is needed again
        
        int cellledcount=0;
        while(cellledcount<5){
        //5s pulse for COMPLETE CELL TX 
        digitalWrite(D8,HIGH);
        delay(400);
        digitalWrite(D8,LOW);
        delay(600);
        cellledcount++;
        }
    }
//////////////////////////////////////////////////////////////////////
    
}   //end VOID LOOP() 


//////////////////////////////////////////////////////////////////////
/////////////////This does something, probably////////////////////////
void myHandler(const char *event, const char *data) {
  // Handle the integration response
}
          
