/* 
   Sprinkler Timer Demo for OSBee Shield
   This demo allows you to create sprinkler programs
   through Serial input, and it opens / closes valves
   based on the program data. Up to 4 independent zones
   are supported. Program data stored in internal EEPROM.
   
   http://bee.opensprinkler.com
   
   License: Creative Commons CC-SA 3.0
   
   Written by : Jonathan Goldin (Rayshobby LLC)
   info@rayshobby.net
   
*/

#include <avr/eeprom.h>
#include <Time.h>
#include <Wire.h>

#include <OSBee.h>

// States
#define IS_RUNNING 0
#define CHOOSE_PORT 1
#define CHOOSE_SLOT 2
#define CHOOSE_DELE 3
#define CHOOSE_NEWE 4
#define CHOOSE_STAR 5
#define CHOOSE_RPTC 6
#define CHOOSE_INTV 7
#define CHOOSE_TIME 8
#define CHOOSE_SAVE 9

#define NUM_PROGRAMS 4 // number of sprinkler programs

OSBee osb;

int state;
int port;
int slot;

// Program data structure
struct entry{
  int hours;
  int minutes;
  int repeatCount;
  int interval;
  int waterTime;
};

int minuteHand;

void setup() {
  // Must call this before calling other OSBee functions
  osb.begin();

  state = IS_RUNNING;
  minuteHand = -1;

  Serial.begin(9600);
  Serial.println("OSBee Sprinkler Timer Demo");

  Serial.println("");
  Serial.println("Current state: running mode.");

  printHelp();
}

entry e;
char buf[32];

unsigned long timeToClose[4] = {0,0,0,0};

void loop() {
  switch(state){
    case IS_RUNNING:
      if(minuteHand != minute()){
        time_t t = now();      
        Serial.print("\r\nCurrent time: ");
        Serial.print(hour(t));
        Serial.print(":");
        if(minute(t) < 10){
          Serial.print("0");
        }
        Serial.println(minute(t));

        checkSchedule();
        minuteHand = minute();
      }
      checkPorts();
      if(Serial.available()){
        Serial.readBytesUntil('\n',buf,32);
        if(!strncmp(buf,"program",7)){
          state = CHOOSE_PORT;
          osb.closeAll();
          Serial.println("Current state: programming mode.");
          Serial.println("Schedule paused.");
          Serial.print("Choose port (A, B, C, or D):");
        } else if(!strncmp(buf,"clear all",9)){
          byte empty[4*NUM_PROGRAMS*sizeof(entry)];
          memset(empty,0,sizeof(empty));
          eeprom_write_block(empty,0,4*NUM_PROGRAMS*sizeof(entry));
          Serial.println("Cleared");
        } else if(!strncmp(buf,"list all",8)){
          for(int i = 0; i < 4; i++){
            printSlots(i);
          }
        } else if(!strncmp(buf, "help", 4)) {
          printHelp();
        }
      }
      break;
    case CHOOSE_PORT:
      if(Serial.available() == 1){
        port = Serial.read();
        if(port >= 'a' && port <= 'd'){
          port -= 'a';
          state = CHOOSE_SLOT;
        } else if(port >= 'A' && port <= 'D'){
          port = port-'A';
          state = CHOOSE_SLOT;
        } else {
          port = -1;
        }
        if(port != -1){
          Serial.println((char)(port+'A'));
          printSlots(port);
          Serial.print("Choose program (0-");
          Serial.print(NUM_PROGRAMS-1);
          Serial.print("): ");
        }
      }
      break;
    case CHOOSE_SLOT:
      if(Serial.available() == 1){
        slot = Serial.read();
        if(slot >= '0' && slot <= ('0'+NUM_PROGRAMS-1)){
          slot -= '0';
          state = CHOOSE_DELE;
          Serial.println(slot);
          Serial.println("");
          printSlot(port,slot);
          Serial.println("");
          Serial.print("Would you like to clear this entry (y/n)? ");
        } else {
          slot = -1;
        }
      }
      break;
    case CHOOSE_DELE:
      if(Serial.available() == 1){
        char answer = Serial.read();
        if(answer == 'y'){
          Serial.println(answer);
          e.hours = 0;
          e.minutes = 0;
          e.repeatCount = 0;
          e.interval = 0;
          e.waterTime = 0;
          int loc = (port*NUM_PROGRAMS*sizeof(entry))+(slot*sizeof(entry));
          eeprom_write_block(&e,(void*)loc,sizeof(entry));
          Serial.println("Deleted");
          state = CHOOSE_NEWE;
        } else if(answer == 'n'){
          Serial.println(answer);
          state = CHOOSE_NEWE;
        }
      }
      if(state == CHOOSE_NEWE){
        Serial.print("Would you like to create a new entry for program: ");
        Serial.print(slot);
        Serial.print(" (y/n): ");
      }
      break;
    case CHOOSE_NEWE:
      if(Serial.available() == 1){
        char response = Serial.read();
        if(response == 'y'){
          Serial.println(response);
          state = CHOOSE_STAR;
          Serial.println("NOTE: leading zero needed for start time");
          Serial.print("choose start time (XX:XX):");
        } else if(response == 'n'){
          Serial.println(response);
          Serial.println("Canceled.");
          state = IS_RUNNING;
          Serial.println("Running.");
        }
      }
      break;          
    case CHOOSE_STAR:
      if(Serial.available() == 5){
        Serial.readBytesUntil(':',buf,2);
        e.hours = atoi(buf);
        Serial.read();
        Serial.readBytesUntil('\n',buf,2);
        e.minutes = atoi(buf);
        state = CHOOSE_RPTC;
        Serial.print(e.hours);
        Serial.print(":");
        if(e.minutes < 10){
          Serial.print("0");
        }
        Serial.println(e.minutes);
        Serial.print("Input repeat count (0-999):");       
      }
      break;
    case CHOOSE_RPTC:
      if(Serial.available()){
        memset(buf,0,sizeof(buf));
        Serial.readBytesUntil('\n',buf,3);
        e.repeatCount = atoi(buf);
        Serial.println(e.repeatCount);
        state = CHOOSE_INTV;
        Serial.print("Input interval time (0-999) in minutes:");
      }
      break;
    case CHOOSE_INTV:
      if(Serial.available()){
        memset(buf,0,sizeof(buf));
        Serial.readBytesUntil('\n',buf,3);
        e.interval = atoi(buf);
        Serial.println(e.interval);
        state = CHOOSE_TIME;
        Serial.print("Input water time (0-999) in seconds:");
      }
      break;
    case CHOOSE_TIME:
      if(Serial.available()){
        memset(buf,0,sizeof(buf));
        Serial.readBytesUntil('\n',buf,3);
        e.waterTime = atoi(buf);
        Serial.println(e.waterTime);
        state = CHOOSE_SAVE;
        Serial.println("save/cancel?");
      }
      break;
    case CHOOSE_SAVE:
      if(Serial.available()){
        memset(buf,0,sizeof(buf));
        Serial.readBytesUntil('\n',buf,32);
        if(!strncmp(buf,"cancel",6)){
          Serial.println("Canceled");
          state = IS_RUNNING;
          Serial.println("Running.");
        } else if(!strncmp(buf,"save",4)){
          int loc = (port*NUM_PROGRAMS*sizeof(entry))+(slot*sizeof(entry));
          eeprom_write_block(&e,(void*)loc,sizeof(entry));
          Serial.println("Saved");
          state = IS_RUNNING;
          Serial.println("Running.");
        }
      }
      break;
    }
}

void printSlots(int port) {
  Serial.println("");
  Serial.print("Programs for port ");
  Serial.println((char)(port + 'A'));
  for(int i = 0; i < NUM_PROGRAMS; i++){
    printSlot(port,i);
  }
  Serial.println("");
}

void printHelp() {
  Serial.println("");
  Serial.println("Choose command:");
  Serial.println("- program: create, modify, or delete program entries");
  Serial.println("- list all: lists all existing program entries");
  Serial.println("- clear all: deletes all program entries");
  Serial.println("- help: print command list");
  Serial.println("");  
}

void printSlot(int port, int slot) {
  Serial.print("Program: ");
  Serial.print(slot);
  int loc = (port*NUM_PROGRAMS*sizeof(entry))+(slot*sizeof(entry));
  eeprom_read_block(&e,(void*)loc,sizeof(entry));
  Serial.print("  start time: ");
  Serial.print(e.hours);
  Serial.print(":");
  if(e.minutes < 10){
    Serial.print("0");
  }
  Serial.print(e.minutes);
  Serial.print("  repeats: ");
  Serial.print(e.repeatCount);
  Serial.print("  interval: ");
  Serial.print(e.interval);
  Serial.print("  water time: ");
  Serial.println(e.waterTime);
}
  
void checkPorts() {
  for(int port = 0; port < 4; port++){
    if(timeToClose[port] != 0){
      if(millis() >= timeToClose[port]){
        osb.close(port);
        timeToClose[port] = 0;
      }
    }
  }
}

void checkSchedule() {
  Serial.println("Checking program data...");
  time_t t = now();
  bool found = false;
  for(int port = 0;  port < 4; port++){
    for(int slot = 0; slot < NUM_PROGRAMS; slot++){
      int loc = (port*NUM_PROGRAMS*sizeof(entry))+(slot*sizeof(entry));
      eeprom_read_block(&e,(void*)loc,sizeof(entry));
      if(e.repeatCount != 0){
        int time_in_minutes = hour(t)*60+minute(t);
        int start_time_minutes = e.hours*60 + e.minutes;
        if(e.repeatCount == 1 && start_time_minutes == time_in_minutes){
        
          Serial.print("Entry found: port ");
          Serial.print((char)(port+'A'));
          Serial.print(", program ");
          Serial.print(slot);
          Serial.print(", turning on for ");
          Serial.print(e.waterTime);
          Serial.println(" seconds.");
          
          osb.open(port);
          found = true;
          timeToClose[port] = max(timeToClose[port],millis()+(unsigned long)e.waterTime*1000);
        } else {
          if(time_in_minutes < start_time_minutes){
            time_in_minutes += 24*60;
          }
          if((time_in_minutes-start_time_minutes)%e.interval == 0 && (time_in_minutes-start_time_minutes)/e.interval < e.repeatCount){
            
            Serial.print("Entry found: port ");
            Serial.print((char)(port+'A'));
            Serial.print(", program ");
            Serial.print(slot);
            Serial.print(", turning on for ");
            Serial.print(e.waterTime);
            Serial.println(" seconds.");

            osb.open(port);
            found = true;
            timeToClose[port] = max(timeToClose[port],millis()+(unsigned long)e.waterTime*1000);
          }
        } 
      }
    }
  }
  if (!found) {
    Serial.print("None scheduled to open at this minute.");
  }
}
