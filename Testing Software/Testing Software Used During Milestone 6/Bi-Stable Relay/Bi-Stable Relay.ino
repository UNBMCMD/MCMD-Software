
int relay = 6;
int MCMDsense = 7;
int value = 0;

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
pinMode(relay, OUTPUT);
pinMode(MCMDsense, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  value = digitalRead(MCMDsense);
  Serial.println(value);
  
digitalWrite(relay, HIGH);
delay(13);
digitalWrite(relay, LOW);
delay(13);
}
