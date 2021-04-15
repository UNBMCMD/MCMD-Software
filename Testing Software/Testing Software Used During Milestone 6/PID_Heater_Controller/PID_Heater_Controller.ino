#define aref_voltage 3.3
//temp sensor is mcp9701a
//define variables
double ePrevious = 0;
double Upid = 0;
double e = 0;
double de = 0;
double te = 0;

int Ts = 10; 

int tempPin = A1; //pin1 on uno A1
int heaterPin = A2; //pin4 on uno A2
int H_D = 4; 
int H_D_read;
int temp;
int tempReading;
int temp_setpoint = 10; //Temperature in C

double Kp = 3;
double Ki = 0.05;
double Kd = 2;

unsigned long last_time;
unsigned long current_time;
int delta_time;

void setup() {
  //Serial.begin(9600);
  //analogReference(EXTERNAL); //only valid for UNO
  pinMode(heaterPin, OUTPUT);
  pinMode(H_D, INPUT);
  pinMode(tempPin, INPUT);
}

void loop() {
  H_D_read = digitalRead(H_D);

  if (H_D_read == HIGH){
    analogWrite(heaterPin, 0);
    }
  while (H_D_read == HIGH) {
    H_D_read = digitalRead(H_D);
    }

  current_time = millis();
  delta_time = current_time - last_time;
  if (delta_time >= Ts) {
    //Check temperature
    tempReading = analogRead(tempPin);
    float voltage = tempReading * aref_voltage;
    voltage /= 1024.0;
    float temp = (voltage - 0.5) * 100;

    e = temp_setpoint - temp;

    //find new values
    de = e - ePrevious;
    te = te + e;

    //error limits
    if (te > 255) {
      te = 255;
    }
    if (te < 0) {
      te = 0;
    }

    //calculate control signal
    Upid = (Kp * e) + (Ki * te * Ts) + ((Kd / Ts) * de);

    //control limits
    if (Upid > 255) {
      Upid = 255;
    }
    if (Upid < 0) {
      Upid = 0;
    }
    last_time = current_time;

    analogWrite(heaterPin, Upid);
    
    //  Serial.print("Heater Power ");
    //  int power = (Upid / 255) * 100;
    //  Serial.print(power);
    //  Serial.println(" %");
    
  }
  ePrevious = e;


}
