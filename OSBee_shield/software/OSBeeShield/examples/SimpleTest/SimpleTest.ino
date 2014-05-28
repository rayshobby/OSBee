/* 
   Simple Test program for OSBee Shield
   This demo opens and closes each valve
   for 10 seconds, one after another
   
   http://bee.opensprinkler.com
   
   License: Creative Commons CC-SA 3.0
   
   Written by : Ray Wang (Rayshobby LLC)
   info@rayshobby.net
   
*/

#include <OSBee.h>

#define DELAY_TIME  5000

OSBee osb;

void setup() {
  
  // these are optional parameters
  //osb.setVoltage(18.0);
  //osb.setPulseLength(100);
  //osb.setDutyCycle(25);
  
  // must call the begin() function for proper initialization
  osb.begin();
}

void loop() {
  // you can either call the open / close
  // function by station name
  osb.open('A');
  delay(DELAY_TIME);
  osb.close('A');
  
  osb.open('B');
  delay(DELAY_TIME);
  osb.close('B');
  
  // or call the open / close function
  // by station index
  osb.open(2);        // same as open('C');
  delay(DELAY_TIME);
  osb.close(2);

  osb.open(3);        // same as open('D');
  delay(DELAY_TIME);
  osb.close(3);

}  
