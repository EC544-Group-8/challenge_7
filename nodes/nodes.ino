#include <XBee.h>
#include <SoftwareSerial.h>
#include <ArduinoSTL.h>
#include <algorithm>

// Define the relevant pins
#define BLUE_LED_PIN 5
#define GREEN_LED_PIN 6
#define RED_LED_PIN 7
#define BUTTON_PIN 8

// Xbee Declarations
AtCommandResponse atResponse = AtCommandResponse();
uint8_t dbCommand[] = {'D','B'};
uint8_t discoveryCommand[] = {'N','D'};
uint8_t myIdCommand[] = {'N', 'I'};
AtCommandRequest atRequest = AtCommandRequest(discoveryCommand);
AtCommandRequest myIdRequest = AtCommandRequest(myIdCommand);

// Arduino Declarations
XBee xbee = XBee();
SoftwareSerial xbeeSerial(2,3);
int buttonState;             // current reading from input pin
int lastButtonState = LOW;   // the previous reading from input pin
int myID;
std::vector<int> connected_nodes; // For storing the IDs
int discovery_timeout = 3;

// to measure time in ms
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 2;    // the debounce time; increase if the output flickers

void insertID(int id) {
    // If not already connected
    Serial.print("ID is:");
    Serial.println(id);
    
    if (std::find(connected_nodes.begin(), connected_nodes.end(), id) == connected_nodes.end()) {
        Serial.println("pusing to vector");
        connected_nodes.push_back(id);
    }
    return;
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
  digitalWrite(BLUE_LED_PIN,  LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN,   LOW);

}

int checkButtonPress(){
  // Check to see if the button has been pressed since last loop
  int reading = digitalRead(BUTTON_PIN);
  int buttonPressed = 0;
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        buttonPressed = 1;
      }
    }
  }
  lastButtonState = reading;
  return buttonPressed;
}

void set_leader_led() {
    digitalWrite(BLUE_LED_PIN, HIGH);
    return;
}

void set_pleb_led() {
    digitalWrite(BLUE_LED_PIN, LOW);
    return;
}

// When button is pressed, handle it
//void handleButtonPress() {
//    blueLedState = !blueLedState;
//    greenLedState = !greenLedState;
//    redLedState = !redLedState;
//
//    // set the LED:
//    digitalWrite(BLUE_LED_PIN, blueLedState);
//    digitalWrite(GREEN_LED_PIN, greenLedState);
//    digitalWrite(RED_LED_PIN, redLedState);
//}

//----------------------------------------------------------
//                    XBee Functions
//-----------------------------------------------------------
int sendATCommand(AtCommandRequest atRequest) {
  int counter = 0;
  int value = -1;
  int size_before = connected_nodes.size();
  Serial.print("Size before:");
  Serial.println(size_before);
  
  if (discovery_timeout > 0) {
    Serial.println("Sending command to the XBee");
  
    // send the command
    xbee.send(atRequest);
  
    // Let it run this loop 5 times (= 5 seconds)  
    while (counter++ < 5) {
      // wait up to 1 second for the status response
      if (xbee.readPacket(1000)) {  
        if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
          xbee.getResponse().getAtCommandResponse(atResponse);
    
          if (atResponse.isOk()) {
            if (atResponse.getValueLength() > 0) {
              // Store the ID in the vector if it isn't already there
              
              insertID(atResponse.getValue()[11]); // this is the ID

              
            }
          }
        }
      }
    }
  }
  int size_after = connected_nodes.size();
  Serial.print("Size after:");
  Serial.println(size_after);
  Serial.print("disc timeout:");
  Serial.println(discovery_timeout);
  // If the size has changed, reset the counter
  if (size_after - size_before > 0) {
    discovery_timeout = 3;
  } else {
    discovery_timeout--;
  }
  return value;
}

int get_my_id(AtCommandRequest atRequest) {
  int counter = 0;
  int value = -1;
  Serial.println("Sending my command to the XBee");

  // send the command
  xbee.send(atRequest);

    // wait up to 3 seconds for the status response
    if (xbee.readPacket(3000)) {
      // got a response!
  
      // should be an AT command response
      if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
        xbee.getResponse().getAtCommandResponse(atResponse);
  
        if (atResponse.isOk()) {
          if (atResponse.getValueLength() > 0) {
            insertID(atResponse.getValue()[1]); // this is the ID
          }
        } 
      } 
    } 
  return value;
}

void decide_role(std::vector<int> connected_nodes) {
  if (connected_nodes.size() == 1) {
    set_leader_led();
  } else if (myID == *std::max_element(connected_nodes.begin(), connected_nodes.end())) {
    // Leader LED
    set_leader_led();
  } else {
    // Non-leader LED
    set_pleb_led();
  }
}

void setup() {
  
  // start serial
  xbeeSerial.begin(9600);
  xbee.setSerial(xbeeSerial);
  Serial.println("Initializing beacon...");

  // Initialize digital pins
  initializePins();
 
  Serial.begin(9600);
  myID = get_my_id(myIdRequest);
}

void loop() {
  decide_role(connected_nodes);

  // ----------Button Handling---------- //  
  int was_pressed = checkButtonPress();
  
  if(was_pressed) {
    //handleButtonPress();
  }
  if (discovery_timeout > 0)
    int result = sendATCommand(atRequest);

  // Print the values in our vector
  Serial.print("Num of connected_nodes is: ");
  Serial.println(connected_nodes.size());
  for (int i = 0; i < connected_nodes.size(); i++) {
    Serial.print(connected_nodes[i]);
    Serial.print(" ");
  }
  decide_role(connected_nodes);
  Serial.println();
  delay(5000);
  
}
