/*
Zombie nodes
*/
int BlueLedPin = 5;
int GreenLedPin = 6;
int RedLedPin = 7;


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pins with LEDs as an outputs.
  pinMode(BlueLedPin, OUTPUT);
  pinMode(GreenLedPin, OUTPUT);
  pinMode(RedLedPin, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(BlueLedPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(BlueLedPin, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
  digitalWrite(GreenLedPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(GreenLedPin, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);    
  digitalWrite(RedLedPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(RedLedPin, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);    
}
