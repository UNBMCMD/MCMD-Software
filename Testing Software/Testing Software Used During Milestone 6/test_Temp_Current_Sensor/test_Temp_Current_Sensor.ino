const int numReadings = 10;
int readIndex = 0;
int readings[numReadings];
//Analog Reference Voltage
#define aref_voltage 3.3

int tempPin = A9;
int tempReading;


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
// Take the average of 500 times
const int averageValue = 500;
long int sensorValue = 0;
float sensitivity = 1000.0 / 264.0; //1000mA per 264mV
float Vref = 325.49;   //You need test it !!


void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
//analogReference(EXTERNAL);
}

void loop() {
  // Read voltage value from sensor
  tempReading = analogRead(tempPin);
  //Print to serial monitor
  Serial.print("RAW Input = ");
  Serial.print(tempReading);

  // Convert to voltage
  float voltage = tempReading * aref_voltage;
  voltage /= 1024.0;

  //Print voltage to serial monitor
  Serial.print(" - ");
  Serial.print(voltage);
  Serial.print(" volts ");

  // Print the temperature
  // Convert from 10 mv per degree with 500 mv offset to degree celsius
  // (voltage - 500mv) * 10)
  float temperatureC = (voltage - 0.5) * 100;
  Serial.print(" --> Temperature ");
  Serial.print(temperatureC);
  Serial.print(" degrees C  ");
readings[readIndex] = temperatureC;
Serial.println(readings[readIndex]);
readIndex = readIndex + 1;
if (readIndex >= numReadings) // if we're at the end of the array...
  {
    Serial.println("The 10 values in the array:");
    for(int i=0;i<readIndex;i=i+1)
    {
      Serial.println(readings[i]);
    }
    readIndex = 0;
  // Delay for sensor to stabilize
  }

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
    SER.print("mA");

    SER.print("\n");

    // Reset the sensorValue for the next reading
    sensorValue = 0;
    // Read it once per second
  delay(2000);
  
}
