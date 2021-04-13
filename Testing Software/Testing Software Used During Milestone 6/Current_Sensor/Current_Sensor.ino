
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
    #define RefVal 3.3
    #define SER SerialUSB
#else
    #define RefVal 5.0
    #define SER Serial
#endif
//An OLED Display is required here
//use pin A0
#define Pin A0

// Take the average of 500 times
const int averageValue = 500;

long int sensorValue = 0;
float sensitivity = 1000.0 / 264.0; //1000mA per 264mV



float Vref = 327.15;   //You need test it !!!

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
    float unitValue = RefVal / 1024.0 * 1000 ;
    float voltage = unitValue * sensorValue;

    //When no load,Vref=initialValue
    SER.print("initialValue: ");
    SER.print(voltage);
    SER.println("mV");

    // Calculate the corresponding current
    float current = (voltage - Vref) * sensitivity;

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
    delay(1000);
}
