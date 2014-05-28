/* 
   An Arduino Library for OSBee Shield
   http://bee.opensprinkler.com
   
   License: Creative Commons CC-SA 3.0
   
   Written by : Ray Wang (Rayshobby LLC)
   info@rayshobby.net
   
*/

#ifndef _OSBEE_H
#define _OSBEE_H

// Default boost voltage, unit is Volt
#define DEFAULT_BOOST_VOLTAGE 16.0

// Default duty cycle (percentage)
#define DEFAULT_BOOST_DUTY    25

// Default pulse length, unit is millisecond
#define DEFAULT_PULSE_LENGTH  25

#define pinBOOST    9 // boost PWM pin
#define pinBOOST_FB 0 // boost feedback pin (analog pin)
#define pinBATT_FB  1 // battery voltage feedback pin (analog pin)


// Set / Rst pins for ports A, B, C, D
#define pinSET_A    3
#define pinRST_A    4
#define pinSET_B    5
#define pinRST_B    6
#define pinSET_C    7
#define pinRST_C    8
#define pinSET_D    10
#define pinRST_D    16

// Turn on serial debug messages 
//#define SERIAL_DEBUG 

class OSBee {
private:
  int voltage_level;  // analog value for boost converter feedback
  int pulse_length;   // pulse length (in milliseconds)
  int duty_cycle;     // duty cycle for PWM
  bool initialized;   
  void boost();       // boost voltage
public:
  OSBee();
  void begin();
  // set boost voltage, generally between 9 and 22
  void setVoltage(float v);
  // set pulse length (generally between 20 to 200)
  void setPulseLength(int l);
  // set duty cycle, generally between 5 and 50
  // increasing duty cycle results in higher current draw and faster boost time
  // decreasing duty cycle results in lower current draw and slower boost time
  void setDutyCycle(int c);
  void closeAll();    // close all valves
  void close(int i);  // close valve i (i = 0, 1, 2, 3, corresponding to ports A, B, C, D)
  void close(char p); // close valve p (p = 'A', 'B', 'C', 'D')
  void open(int i);   // open valve i (i = 0, 1, 2, 3, corresponding to ports A, B, C, D)
  void open(char p);  // open valve p (p = 'A', 'B, 'C', 'D')
  float getBattVoltage(); // get battery voltage
};

#endif
