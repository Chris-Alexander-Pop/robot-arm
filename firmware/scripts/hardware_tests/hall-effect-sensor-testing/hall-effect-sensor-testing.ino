const int hallPin = A0;

void setup() {
  Serial.begin(9600);
}

void loop() {

  int sensorValue = analogRead(hallPin);

  Serial.print("Hall Sensor Value: ");
  Serial.println(sensorValue);

  delay(1000);
}