
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
    #define RefVal 3.3
    #define SER SerialUSB
#else
    #define RefVal 5.0
    #define SER Serial
#endif
//An OLED Display is required here
//use pin A0
#define Pin A13
int readIndex = 0;
const int numReadings = 10;
int readings[numReadings];
// Take the average of 500 times
const int averageValue = 500;

long int sensorValue = 0;
float sensitivity = 1000.0 / 264.0; //1000mA per 264mV



float Vref = 325.49;   //You need test it !!!

void setup() {
    SER.begin(9600);
}

void loop() {
    // Read the value 10 times:
    for (int i = 0; i < averageValue; i++) {
        sensorValue += analogRead(Pin);

        // wait 2 milliseconds before the next loop
        delay(2);

    }

    sensorValue = sensorValue / averageValue;


    // The on-board ADC is 10-bits
    // Different power supply will lead to different reference sources
    // example: 2^10 = 1024 -> 5V / 1024 ~= 4.88mV
    //          unitValue= 5.0 / 1024.0*1000 ;
    float unitValue = 3.3 / 1024.0 * 1000 ;
    float voltage1 = unitValue * sensorValue;

    //When no load,Vref=initialValue
    SER.print("initialValue: ");
    SER.print(voltage1);
    SER.println("mV");

    // Calculate the corresponding current
    float current = (voltage1 - Vref) * sensitivity;
    
    // Print display voltage (mV)
    // This voltage is the pin voltage corresponding to the current
    /*
        voltage = unitValue * sensorValue-Vref;
        SER.print(voltage);
        SER.println("mV");
    */

    // Print display current (mA)
    SER.print("current:");
    SER.print(current);
    SER.println("mA");

    SER.print("\n");

    // Reset the sensorValue for the next reading
    sensorValue = 0;
    // Read it once per second

  readings[readIndex] = current;
  While(current >0){
    if(current >6.5){
      Isolate();
    }
    if(current<0){
      Disolate();
    }
    return 0;
  }
  delay(1000);
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


    
