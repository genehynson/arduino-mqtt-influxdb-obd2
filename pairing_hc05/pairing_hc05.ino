void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
}

void loop() {
  // write commands to hc-05
  if (Serial.available()) {
    Serial1.write(Serial.read());
  }

  // read response from hc-05
  if (Serial1.available()) {
    Serial.write(Serial1.read());
  }
}