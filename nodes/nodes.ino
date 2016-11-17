/*
Zombie nodes

 Debounced buttons (adapted from http://www.arduino.cc/en/Tutorial/Debounce)

 Each time the input pin goes from LOW to HIGH (e.g. because of a push-button
 press), the output pin is toggled from LOW to HIGH or HIGH to LOW.  There's
 a minimum delay of 1ms between toggles to debounce the circuit (i.e. to ignore
 noise).


*/

//Define the relevant pins
#define blueLedPin 5
#define greenLedPin 6
#define redLedPin 7
#define buttonPin 8


// Variables will change:
int blueLedState = HIGH;     // the current state of the output pin
int greenLedState = HIGH;    // the current state of the output pin
int redLedState = HIGH;      // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// to measure time in ms
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 2;    // the debounce time; increase if the output flickers

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pins with LEDs as an outputs.
  pinMode(blueLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  
  // initialize the button as an input.
  pinMode(buttonPin, INPUT);

    // set initial LED state
  digitalWrite(blueLedPin, blueLedState);
  digitalWrite(greenLedPin, greenLedState);
  digitalWrite(redLedPin, redLedState);

  
}


void loop() {

// ----------Button Handling---------- //  
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        blueLedState = !blueLedState;
        greenLedState = !greenLedState;
        redLedState = !redLedState;
      }
    }
  }

  // set the LED:
  digitalWrite(blueLedPin, blueLedState);
  digitalWrite(greenLedPin, greenLedState);
  digitalWrite(redLedPin, redLedState);

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
}
