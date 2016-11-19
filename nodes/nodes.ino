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

// states
#define DISCOVERY 0
#define LISTEN_FOR_MESSAGES 1

#define DELAY_COUNTER_MAX 10000

int old_role = -1;            // To keep track of infections
int my_role = FOLLOWER_CLEAR; // Start as non-infected follower

int state = 0;
int delay_counter = DELAY_COUNTER_MAX;


// Xbee Declarations
AtCommandResponse atResponse = AtCommandResponse();
ZBRxResponse rx = ZBRxResponse();
uint8_t discoveryCommand[] = {'N','D'};
uint8_t myIdCommand[] = {'M', 'Y'};
uint8_t infectPayload[] = {1, 1};
uint8_t clearPayload[] = {2, 2};
uint8_t leaderPayload[] = {3, 3};
ZBTxRequest infectTx = ZBTxRequest(0xFFFF, infectPayload, sizeof(infectPayload));
ZBTxRequest clearTx = ZBTxRequest(0xFFFF, clearPayload, sizeof(clearPayload));
ZBTxRequest leaderTx = ZBTxRequest(0xFFFF, leaderPayload, sizeof(leaderPayload));
AtCommandRequest atRequest = AtCommandRequest(discoveryCommand);
AtCommandRequest myIdRequest = AtCommandRequest(myIdCommand);

// Arduino Declarations
XBee xbee = XBee();
SoftwareSerial xbeeSerial(2,3);
int buttonState = HIGH;       // current reading from input pin
int lastButtonState = HIGH;   // the previous reading from input pin
int myId;
std::vector<int> connected_nodes; // For storing the IDs
std::vector<int> old_connected_nodes; // For storing the IDs from previous discovery
int discovery_timeout = DISCOVERY_ROUNDS;
bool discovering = true;

// to measure time in ms
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 2;    // the debounce time; increase if the output flickers

unsigned long lastDiscoveryTime = 0;
unsigned long discoveryDelay = 10000;

unsigned long lastInfectTime = 0;
unsigned long infectDelay = 2000; //2 seconds

unsigned long lastLeaderTime = 0;
unsigned long leaderDelay = 10000;

unsigned long lastHeardFromLeaderTime = 0;
unsigned long hearFromLeaderDelay = 12000;

void add_node(int id) {
    if (id == 1 || id == 2 || id == 3)
      return; // just communication message
      
    // If not already connected
    //Serial.print("ID is:");
    //Serial.println(id);
    
    if (std::find(connected_nodes.begin(), connected_nodes.end(), id) == connected_nodes.end()) {
        Serial.println("pushing to vector");
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

// When button is pressed, handle it
void handleButtonPress() {
  Serial.println("Handle Button press");
  if(my_role == LEADER){
      Serial.println("Initiate Clear message");
      // The leader sends 1 CLEAR message
      xbee.send(clearTx);

  } else {
      // For any other my_role, you are now infected
      Serial.println("button infected");
      my_role = FOLLOWER_INFECTED;
      lastInfectTime = millis();
  }
}
 

//----------------------------------------------------------
//                    XBee Functions
//-----------------------------------------------------------

int check_for_messages(int wait_time, int index){
  int value = 0;
  if(xbee.readPacket(wait_time)) {  
    //Serial.println("parsing received w API id: ");
    //Serial.println(xbee.getResponse().getApiId());
    if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
      xbee.getResponse().getAtCommandResponse(atResponse);
  
      if (atResponse.isOk()) {
        if (atResponse.getValueLength() > 0) {           
          value = atResponse.getValue()[index]; // this is the value we are looking for
          value << 8;
          value += atResponse.getValue()[index + 1]; // get the next byte of the address as well.
          Serial.println(value);
        }
      }
    } else if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
      xbee.getResponse().getZBRxResponse(rx);
      Serial.print("RX: ");
      Serial.println(rx.getData(0));
      value = rx.getData(0);

      // Receive word from leader in middle of discovery
      if (discovering && value == 3) {
        if (old_role == -1)
          my_role = FOLLOWER_CLEAR; // haven't had a role yet
        else
          my_role = old_role;  // resume previous role

        discovering = false;
      }
      
    } else {
      //Serial.println("UNKNOWN RESPONSE");
      //Serial.println(xbee.getResponse().getApiId());
    }
    
  }
  return value;
}

// Get Node ID on Power On
int get_my_id() {
  int value = 0;
  Serial.println("Sending my command to the XBee");
  // send the command
  xbee.send(myIdRequest);
 
  // wait up to 1 second for the status response
  // This needs to be changed to address
  value = check_for_messages(400, 0);
  if(value != 0){
    add_node(value); // this is the ID    
  }
  return value;
}

// After discover, set the node's role
void decide_role(std::vector<int> connected_nodes) {
  Serial.println("DECIDE ROLES");
  Serial.println(myId);
  Serial.println(*std::max_element(connected_nodes.begin(), connected_nodes.end()));
  if (connected_nodes.size() == 1) {
    my_role = LEADER;

  } else if (myId == *std::max_element(connected_nodes.begin(), connected_nodes.end())) {
    my_role = LEADER;

  } else {
    // Non-leader, only update if not infected
    if(old_role != FOLLOWER_INFECTED){
      my_role = FOLLOWER_CLEAR; 
    }
  }
}

int runDiscovery() {
    connected_nodes.clear();
    discovering = true;
    while(discovery_timeout > 0 && discovering){
      int discovery_counter = 0;
      int value = 0;
      int size_before = connected_nodes.size();
      //Serial.print("Size before:");
      //Serial.println(size_before);
      //Serial.println("Sending command to the XBee");
      xbee.send(atRequest);

      // Let it run this loop 5 times (= 2 seconds max)  
      while (discovery_counter++ < 5) {
        // wait up to 1 second for the status response
        value = check_for_messages(400, 0);
        if(value != 0 && value != 1 && value != 2 && value != 3){
          add_node(value); // this is the ID    
        }
      }
  
      int size_after = connected_nodes.size();

      // If the size has changed, reset the counter
      if (size_after - size_before > 0) {
        discovery_timeout = DISCOVERY_ROUNDS;
      } else {
        discovery_timeout--;
      }
    }
    lastDiscoveryTime = millis();
    
     // We now have enough consistent info to decide roles 
     discovering = false;
     decide_role(connected_nodes);
     
    // Print the values in our vector
    Serial.print("Num of connected_nodes is: ");
    Serial.println(connected_nodes.size());
    
    for (int i = 0; i < connected_nodes.size(); i++) {
      Serial.print(connected_nodes[i]);
      Serial.print(" ");
      Serial.println("Listen for messages state...");
     }
        
      // Switch to listening state
      state = LISTEN_FOR_MESSAGES;

      // reset for next discovery
      discovery_timeout = DISCOVERY_ROUNDS;
}

void setup() {
  // start serial
  xbeeSerial.begin(57600);
  xbee.setSerial(xbeeSerial);
  Serial.println("Initializing beacon...");

  // Initialize digital pins
  initializePins();

  // Get my ID  
  Serial.begin(9600);
  myId = get_my_id();
}

void loop() {
  // Check if button pressed
  int was_pressed = checkButtonPress();
  if (was_pressed) {
    handleButtonPress();
  }

  // Only update LEDs if the role has changed
  if(my_role != old_role) {
    output_to_leds(my_role);
    old_role = my_role; 
  }

   // Re-discover if haven't hard from leader in 15 seconds
   if (my_role != LEADER && ((millis() - lastHeardFromLeaderTime) > hearFromLeaderDelay)) {
    Serial.println("Haven't heard from leader--rediscovering");
    state = DISCOVERY;
   }
   // Infected nodes need to send infect message every 2 seconds
  if (my_role == FOLLOWER_INFECTED && (millis() - lastInfectTime) > infectDelay) {
    Serial.println("Sending infect message");
    xbee.send(infectTx);

    // Reset last infect time
    lastInfectTime = millis();
  }

     // Leader node need to send leader message every 1 seconds
  if (my_role == LEADER && (millis() - lastLeaderTime) > leaderDelay) {
    Serial.println("Sending leader message");
    xbee.send(leaderTx);

    // Reset last infect time
    lastLeaderTime = millis();
  }
    
    
  // Instead of delays in the messages, use counter
  if(1 || delay_counter-- < 0) {
    int value;
        
    if (state == DISCOVERY) {
      // If we are in a discovery state
      Serial.println("Discovering state activated");
      runDiscovery();
      
    } else if (state == LISTEN_FOR_MESSAGES) {
      //Serial.println(".");
      value = check_for_messages(50, 1); //  is just a temp placeholder for the value we want to grab from the packet
      
      if(value != 0){
        if(value == 1) { // INFECT MESSAGE
          if(my_role != LEADER){
            my_role = FOLLOWER_INFECTED; 
          }
        } else if (value == 2) { // CLEAR MESSAGE
          if(my_role != LEADER){
            my_role = FOLLOWER_CLEAR;
            //xbeeSerial.flush();
            // Make immune to infections for 3 seconds
            output_to_leds(my_role);
            delay(3000);
          }  
        } else if (value == 3) { // LEADER IS ALIVE 
            lastHeardFromLeaderTime = millis();
            lastDiscoveryTime = millis(); // reset the time we are waiting to initiate Discovery again 
            if (my_role == LEADER) {
              Serial.println("Seems to be 2 leaders, rediscovering...");
              state = DISCOVERY;
            }

        }
        Serial.println("GOT A MESSAGE"); // Need a convention for sending and receiving these types of messages    
        Serial.println(value); // this is the messageID, need to do something with it   
      }
      
    }
    
    // Reset the delay counter
    delay_counter = DELAY_COUNTER_MAX;
  }
}
