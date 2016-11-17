/*
Zombie nodes

 Debounced buttons (adapted from http://www.arduino.cc/en/Tutorial/Debounce)

 Each time the input pin goes from LOW to HIGH (e.g. because of a push-button
 press), the output pin is toggled from LOW to HIGH or HIGH to LOW.  There's
 a minimum delay of 1ms between toggles to debounce the circuit (i.e. to ignore
 noise).


*/
#include <XBee.h>
#include <SoftwareSerial.h>

// Define the relevant pins
#define BLUE_LED_PIN 5
#define GREEN_LED_PIN 6
#define RED_LED_PIN 7
#define BUTTON_PIN 8
#define MAX_LOOPS 20000

int round_number = 0;
XBee xbee = XBee();
XBeeResponse response = XBeeResponse();

// create reusable response objects for responses we expect to handle 
ZBRxResponse rx = ZBRxResponse();
ModemStatusResponse msr = ModemStatusResponse();

//AtCommandRequest atRequest = AtCommandRequest(dbCommand);
ZBTxStatusResponse txStatus = ZBTxStatusResponse();
AtCommandResponse atResponse = AtCommandResponse();
uint8_t dbCommand[] = {'D','B'};
uint8_t discoveryCommand[] = {'N','D'};
AtCommandRequest atRequest = AtCommandRequest(discoveryCommand);
uint8_t BEACON_ID = 1;

SoftwareSerial xbeeSerial(2,3);

// Variables will change:
int blueLedState  = HIGH;    // current state of output pin
int greenLedState = HIGH;    // current state of output pin
int redLedState   = HIGH;    // current state of output pin
int buttonState;             // current reading from input pin
int lastButtonState = LOW;   // the previous reading from input pin

// to measure time in ms
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 2;    // the debounce time; increase if the output flickers


// Generates unique ID on startup
// From http://stackoverflow.com/questions/21559264/unique-machine-id-for-arduino-project
int getUniqueID() {
  uint32_t checksum = 0;
  for(uint16_t u = 0; u < 2048; u++)
  {
    //checksum += the byte number u in the ram
    checksum += * ( (byte *) u );
  }
  return checksum;
}

// initialize digital pins
void initializePins() {
  // initialize digital pins with LEDs as an outputs.
  pinMode(BLUE_LED_PIN,  OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN,   OUTPUT);
  
  // initialize the button as an input.
  pinMode(BUTTON_PIN, INPUT);
  
  // set initial LED state
  digitalWrite(BLUE_LED_PIN,  blueLedState);
  digitalWrite(GREEN_LED_PIN, greenLedState);
  digitalWrite(RED_LED_PIN,   redLedState);

}

// Check to see if the button has been pressed since last loop
int checkButtonPress(){
    // read the state of the switch into a local variable:
  int reading = digitalRead(BUTTON_PIN);
  int buttonPressed = 0;
  
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
        buttonPressed = 1;
      }
    }
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;

  return buttonPressed;
}

// When button is pressed, handle it
void handleButtonPress() {
    blueLedState = !blueLedState;
    greenLedState = !greenLedState;
    redLedState = !redLedState;

    // set the LED:
    digitalWrite(BLUE_LED_PIN, blueLedState);
    digitalWrite(GREEN_LED_PIN, greenLedState);
    digitalWrite(RED_LED_PIN, redLedState);
}

//----------------------------------------------------------
//                    XBee Functions
//-----------------------------------------------------------
int sendATCommand(AtCommandRequest atRequest) {
  int counter = 0;
  int value = -1;
  Serial.println("Sending command to the XBee");

  // send the command
  xbee.send(atRequest);

  // Let it run this loop 5 times (=15 seconds)  
  while (counter++ < 5) {
    // wait up to 3 seconds for the status response
    if (xbee.readPacket(1000)) {
      // got a response!
  
      // should be an AT command response
      if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
        xbee.getResponse().getAtCommandResponse(atResponse);
  
        if (atResponse.isOk()) {
          Serial.print("Command [");
          Serial.print(atResponse.getCommand()[0]);
          Serial.print(atResponse.getCommand()[1]);
          Serial.println("] was successful!");
  
          if (atResponse.getValueLength() > 0) {
            Serial.print("Command value length is ");
            Serial.println(atResponse.getValueLength(), DEC);
  
            Serial.print("Command value: ");
            
            for (int i = 0; i < atResponse.getValueLength(); i++) {
              value = atResponse.getValue()[i];
              Serial.print(atResponse.getValue()[i]);
              Serial.print(" ");
            }
  
            Serial.println("");
          }
        } 
        else {
          Serial.print("Command return error code: ");
          Serial.println(atResponse.getStatus(), HEX);
        }
      } else {
        Serial.print("Expected AT response but got ");
        Serial.print(xbee.getResponse().getApiId(), HEX);
      }   
    } else {
      // at command failed
      if (xbee.getResponse().isError()) {
        Serial.print("Error reading packet.  Error code: ");  
        Serial.println(xbee.getResponse().getErrorCode());
      } 
      else {
        Serial.print("No response from radio"); 
        Serial.print(counter); 
      }
    }
  }
  
  return value;
}

void setup() {
  
  // start serial
  xbeeSerial.begin(9600);
  xbee.setSerial(xbeeSerial);
  Serial.println("Initializing beacon...");
  
  // Get my unique ID
  int ID = getUniqueID();

  // Initialize digital pins
  initializePins();
 
  Serial.begin(9600);
  Serial.println(ID, HEX);
}

void loop() {

  // ----------Button Handling---------- //  
  int was_pressed = checkButtonPress();
  
  if(was_pressed) {
    handleButtonPress();
  }
  int result = sendATCommand(atRequest);
  delay(5000);
  
}
