// functions.h
#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include <Arduino.h>
#include <XBee.h>
#include <SoftwareSerial.h>
#include <ArduinoSTL.h>
#include <algorithm>

void insertID(int);
void initializePins();
int checkButtonPress();
void set_leader_led();
void set_pleb_led();
void handleButtonPress();
void decide_role(std::vector<int>);
int sendATCommand(AtCommandRequest atRequest);
int get_my_id(AtCommandRequest atRequest);

#endif
