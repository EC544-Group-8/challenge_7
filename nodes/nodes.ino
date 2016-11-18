#include <XBee.h>
#include <SoftwareSerial.h>
#include <ArduinoSTL.h>
#include <algorithm>

// Define the relevant pins
#define BLUE_LED_PIN 5
#define GREEN_LED_PIN 6
#define RED_LED_PIN 7
#define BUTTON_PIN 8

// roles
#define FOLLOWER_CLEAR 0
#define LEADER 1
#define FOLLOWER_INFECTED 2

// discovery parameters
#define DISCOVERY_ROUNDS 3

int old_role = -1;            // To keep track of infections
int my_role = FOLLOWER_CLEAR; // Start as non-infected follower


// Xbee Declarations
AtCommandResponse atResponse = AtCommandResponse();
uint8_t discoveryCommand[] = {'N','D'};
uint8_t myIdCommand[] = {'N', 'I'};
AtCommandRequest atRequest = AtCommandRequest(discoveryCommand);
AtCommandRequest myIdRequest = AtCommandRequest(myIdCommand);

// Arduino Declarations
XBee xbee = XBee();
SoftwareSerial xbeeSerial(2,3);
int buttonState = HIGH;       // current reading from input pin
int lastButtonState = HIGH;   // the previous reading from input pin
int myID;
std::vector<int> connected_nodes; // For storing the IDs
int discovery_timeout = DISCOVERY_ROUNDS;

// to measure time in ms
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 2;    // the debounce time; increase if the output flickers

unsigned long lastDiscoveryTime = 0;
unsigned long discoveryDelay = 10000;

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
  digitalWrite(BLUE_LED_PIN,  HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN,   HIGH);

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

// When button is pressed, handle it
void handleButtonPress() {
  if(my_role == LEADER){
      Serial.println("Initiate Clear message");
      // The leader sends 1 CLEAR message
      // TODO! broadcast(CLEAR); 

  } else {
      // For any other my_role, you are now infected
      my_role = FOLLOWER_INFECTED;
  }
}

// The current my_role determines the LEDs to display
void output_to_leds(int my_role){
  int leds[3] = {BLUE_LED_PIN, GREEN_LED_PIN, RED_LED_PIN};
  int led_outputs[3][3] = {
    {0,1,0},   // FOLLOWER_CLEAR
    {1,1,0},   // LEADER
    {0,0,1}    // FOLLOWER_INFECTED
  };
  
  for (int i = 0; i <= 2; i++) {
      digitalWrite(leds[i], led_outputs[my_role][i]);
  }
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

  return value;
}

// Get Node ID on Power On
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
            value = atResponse.getValue()[1];
          }
        } 
      } 
    } 
  return value;
}

// After discover, set the node's role
void decide_role(std::vector<int> connected_nodes) {
  Serial.println("DECIDE ROLES");
  Serial.println(myID);
  Serial.println(*std::max_element(connected_nodes.begin(), connected_nodes.end()));
  if (connected_nodes.size() == 1) {
    my_role = LEADER;

  } else if (myID == *std::max_element(connected_nodes.begin(), connected_nodes.end())) {
    my_role = LEADER;

  } else {
    // Non-leader, only update if not infected
    if(old_role != FOLLOWER_INFECTED){
      my_role = FOLLOWER_CLEAR; 
    }
  }
}

int runDiscovery() {
    int size_before = connected_nodes.size();
    Serial.print("Size before:");
    Serial.println(size_before);
  
    int result = sendATCommand(atRequest);

    int size_after = connected_nodes.size();
    Serial.print("Size after:");
    Serial.println(size_after);
    Serial.print("disc timeout:");
    Serial.println(discovery_timeout);
    
    // If the size has changed, reset the counter
    if (size_after - size_before > 0) {
      discovery_timeout = DISCOVERY_ROUNDS;
    } else {
      discovery_timeout--;
    }
  
    // Print the values in our vector
    Serial.print("Num of connected_nodes is: ");
    Serial.println(connected_nodes.size());
    for (int i = 0; i < connected_nodes.size(); i++) {
      Serial.print(connected_nodes[i]);
      Serial.print(" ");
    }
    lastDiscoveryTime = millis();
    Serial.println();
    if(discovery_timeout == 0){
       // We now have enough info to decide roles 
       decide_role(connected_nodes);
    }
}

void setup() {
  // start serial
  xbeeSerial.begin(9600);
  xbee.setSerial(xbeeSerial);
  Serial.println("Initializing beacon...");

  // Initialize digital pins
  initializePins();

  // Get my ID  
  Serial.begin(9600);
  myID = get_my_id(myIdRequest);
}

void loop() {
  // Only update LEDs if the role has changed
  if(my_role != old_role) {
    output_to_leds(my_role);
    old_role = my_role; 
  }
  
  // Check if button pressed
  int was_pressed = checkButtonPress();
  if (was_pressed) {
    handleButtonPress();
  }

  // Check if its time to scan the network again
  if ((millis() - lastDiscoveryTime) > discoveryDelay) {
    discovery_timeout = DISCOVERY_ROUNDS;
  }

  // If we are in a discovery state
  if (discovery_timeout > 0) {
    runDiscovery();
  }
  
  
}
