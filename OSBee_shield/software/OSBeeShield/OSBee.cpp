/* 
   An Arduino Library for OSBee Shield
   http://bee.opensprinkler.com
   
   License: Creative Commons CC-SA 3.0
   
   Written by : Ray Wang (Rayshobby LLC)
   info@rayshobby.net
   
*/
#include <wiring_private.h>
#include <Serial.h>
#include "OSBee.h"

// Constructor function
OSBee::OSBee() {
  voltage_level = 0;
  pulse_length = 0;
  duty_cycle = 0;
  initialized = false;
}

// Must call this function first
void OSBee::begin() {
  
#ifdef SERIAL_DEBUG
  Serial.begin(9600);
#endif

  // !!!!!! IMPORTANT !!!!!!
  // Must set ADC to use internal reference
  analogReference(INTERNAL); 
    
  setVoltage(DEFAULT_BOOST_VOLTAGE);
  setPulseLength(DEFAULT_PULSE_LENGTH);
  setDutyCycle(DEFAULT_BOOST_DUTY);


  // !!!!!! IMPORTANT !!!!!!
  // Set pins in output mode, and clear all pins
  pinMode(pinSET_A, OUTPUT);
  digitalWrite(pinSET_A, LOW);
  
  pinMode(pinRST_A, OUTPUT);
  digitalWrite(pinRST_A, LOW);
  
  pinMode(pinSET_B, OUTPUT);
  digitalWrite(pinSET_B, LOW);
  
  pinMode(pinRST_B, OUTPUT);
  digitalWrite(pinRST_B, LOW);

  pinMode(pinSET_C, OUTPUT);
  digitalWrite(pinSET_C, LOW);
  
  pinMode(pinRST_C, OUTPUT);
  digitalWrite(pinRST_C, LOW);

  pinMode(pinSET_D, OUTPUT);
  digitalWrite(pinSET_D, LOW);
  
  pinMode(pinRST_D, OUTPUT);
  digitalWrite(pinRST_D, LOW);
      
  // The following sets 62.5Khz PWM for pin 9 (timer 1)
  // The ideal frequency depends on the inductor, but it'll only be important if you need alot more power.
  // set prescaler to 1
  // (sbi means "set bit register", cbi means "clear bit register")
  cbi(TCCR1B, CS12);
  cbi(TCCR1B, CS11);
  sbi(TCCR1B, CS10);
  
  // set fast PWM
  cbi(TCCR1B, WGM13);
  sbi(TCCR1B, WGM12);
        
  initialized = true;
  
  // close all valves
  closeAll();
  
#ifdef SERIAL_DEBUG
  Serial.println("Initialized.");
#endif  
}

// Boost voltage
void OSBee::boost() {
  if (!initialized) return;
  
  // !!!!!! IMPORTANT !!!!!!
  // Must set ADC to use internal reference
  analogReference(INTERNAL);  

#ifdef SERIAL_DEBUG
  Serial.print("Boosting...");
#endif

  unsigned int measure;
 
  analogWrite(pinBOOST, 0);
  
  measure = analogRead(pinBOOST_FB);
  if (measure < voltage_level)
    analogWrite(pinBOOST, duty_cycle * 256 / 100);  // set duty cycle
    
  unsigned long t = millis();
  while (measure < voltage_level) {
    measure = analogRead(pinBOOST_FB);
    // set a time limit : 30 seconds
    if (millis() > t + 30000)  break;
  }
  analogWrite(pinBOOST, 0);


#ifdef SERIAL_DEBUG
  Serial.println("done.");
#endif
}

// Calculate the desired analog input reading, for a given voltage
void OSBee::setVoltage(float v) {

  // On OSBee, the boost feedback divider is 100K top, 4.7K bottom.
  // So the analog reading should be:
  // voltage * 47 / (1000 + 47) / 1.1 * 1024
  // where 1.1 is ATmega328's internal voltage reference
  voltage_level = (int)(v*41.8);

#ifdef SERIAL_DEBUG
  Serial.print("Voltage level: ");
  Serial.println(voltage_level);
#endif
}

// Set pulse length (usually between 50 to 200)
void OSBee::setPulseLength(int l) {
  pulse_length = l;
}

void OSBee::setDutyCycle(int c) {
  if (c>0 && c<=50)
    duty_cycle = c;
}

// Close all valves
void OSBee::closeAll() {
  close(0);
  close(1);
  close(2);
  close(3);
}

static byte osbee_pins[] = {pinSET_A, pinRST_A,
                            pinSET_B, pinRST_B,
                            pinSET_C, pinRST_C,
                            pinSET_D, pinRST_D};
  
// Close valve i (i = 0, 1, 2, 3, corresponding to ports A, B, C, D)
void OSBee::close(int i) {
  if(i<0 || i>3)  return;

  int set_pin = osbee_pins[i*2+0];
  int rst_pin = osbee_pins[i*2+1];
  
  digitalWrite(set_pin, LOW);
  digitalWrite(rst_pin, LOW);
  
  boost();
  
  digitalWrite(rst_pin, HIGH);
  delay(pulse_length);
  digitalWrite(rst_pin, LOW);
  
#ifdef SERIAL_DEBUG
  Serial.print("Valve ");
  Serial.print(i);
  Serial.println(" closed.");
#endif  
}

// Close valve p (p = 'A', 'B', 'C', 'D'; or 'a', 'b', 'c', 'd')
void OSBee::close(char p) {
  if (p>='A' && p<='D')
    close((int)(p - 'A'));
  else if (p>='a' && p<='d')
    close((int)(p - 'a'));
}

// Open valve i (i = 0, 1, 2, 3, corresponding to ports A, B, C, D)
void OSBee::open(int i) {
  if(i<0 || i>3)  return;
  int set_pin = osbee_pins[i*2+0];
  int rst_pin = osbee_pins[i*2+1];
  
  digitalWrite(set_pin, LOW);
  digitalWrite(rst_pin, LOW);
  
  boost();
  
  digitalWrite(set_pin, HIGH);
  delay(pulse_length);
  digitalWrite(set_pin, LOW);
  
#ifdef SERIAL_DEBUG
  Serial.print("Valve ");
  Serial.print(i);
  Serial.println(" opened.");
#endif  
}

// Open valve p (p = 'A', 'B, 'C', 'D'; or 'a', 'b', 'c', 'd')
void OSBee::open(char p) {
  if (p>='A' && p<='D')
    open((int)(p - 'A'));
  else if (p>='a' && p<='d')
    open((int)(p = 'a'));
}

// get battery voltage
float OSBee::getBattVoltage() {
  // !!!!!! IMPORTANT !!!!!!
  // Must set ADC to use internal reference
  analogReference(INTERNAL); 
  
  // On OSBee, the battery feedback divider is 470K top, 100K bottom.
  // analog reading = voltage * 100 / (100 + 470) / 1.1 * 1024
  // where 1.1 is ATmega328's internal voltage reference
  return analogRead(pinBATT_FB) * 0.00612;
}

