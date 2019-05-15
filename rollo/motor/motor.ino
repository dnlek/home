#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <Wire.h>
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <LowPower.h>

#define DEFAULT_SPEED "2" // 1 or 2

int servoPin = 3;
int buttonPin = 2;


RF24 radio(7, 8); //CE, CSN
const byte address[6] = "00001";
Servo servo;
char receivedChar;
bool runServo = false;

void setup() {
  Serial.begin(9600);
  Serial.println("SETUP RECEIVER");
  
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  pinMode(buttonPin, INPUT_PULLUP);
  
  // Avoid Randomly Vibrating when turning on
  //XXX not helping too much
  //  servo.attach(servoPin);
  //  servo.write(90); 
  //  delay(200); 
  //  servo.detach();

  // watchdog config for 8s sleep
  //  MCUSR &= ~(1 <<WDRF);
  //  WDTCSR |= (1<<WDCE) | (1<<WDE);
  //  WDTCSR = 1<<WDP0 | 1<<WDP3; // 8.0 seconds
  //  WDTCSR |= _BV(WDIE);

  Serial.println("Initialization complete..");
  delay(100);
}

void wakeUp() {
    // Just a handler for the pin interrupt.
    Serial.println("Brutal wake up");
    runServo = true;
}

void loop() {

  // Serial.println("Battery level: " + getBatteryLevel());
  // delay(500);
  // Serial.println("Vcc: " + readVcc());
  attachInterrupt(digitalPinToInterrupt(buttonPin), wakeUp, FALLING);
  
  Serial.println("Enter sleep");
  delay(500);
  // enterSleep();
  LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);  
  delay(500);
  Serial.println("Wake up");

  detachInterrupt(digitalPinToInterrupt(buttonPin));
  
  if (radio.available()) {
    Serial.println("Message - on the radio");
    char text[32] = "";
    radio.read(&text, sizeof(text));
    Serial.println(text);
    radio.flush_rx();
    if (strcmp(text, "open") == 0) {
      openOrClose(0);
    } else if (strcmp(text, "close") == 0) {
      openOrClose(1);
    }
  }

  if (digitalRead(buttonPin) == LOW || runServo) {
    openOrClose(0);
    runServo = false;
  }
  
  delay(100);
}

void serialEvent() {
  if (Serial.available() > 0) {
    Serial.println("Message - on the serial");
    receivedChar = Serial.read();
    if (receivedChar == 'o') {
      openOrClose(1);
    } else if (receivedChar == 'c') {
      openOrClose(0);
    }
  }
}

void openOrClose(int direction) {
  Serial.println("openOrClose");
  int theSpeed = 2;
  int theDuration = 10 * 1000;
  int dutyCycle = calculateDutyCycleFromSpeedAndDirection(theSpeed, direction);

  servo.attach(servoPin);
  servo.write(dutyCycle); 
  delay(theDuration); 
  servo.detach();

}

int calculateDutyCycleFromSpeedAndDirection(int speed, int direction){
  if(speed == 1 && direction == 1){
    return 80;
  } else if(speed == 2 && direction == 1){
    return 0;
  } else if(speed == 1 && direction == 0){
    return 95;
  } else if(speed == 2 && direction == 0){
    return 180;
  } else{
    return 180;
  }
}

long readVcc() {
  long result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result;
  return result;
}

int getBatteryLevel() {
  int result = (readVcc() - 2000) / 10;

  if (result > 100)
    result = 100;
  if (result < 0)
    result = 0;
  return result;
}

//void enterSleep() {
//  set_sleep_mode(SLEEP_MODE_PWR_SAVE);
//  sleep_enable();
//  
//  sleep_disable();
//  power_all_enable();
//  
//}
