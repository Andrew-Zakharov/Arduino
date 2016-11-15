#include <SoftwareSerial.h>
#include <Wire.h>

#define RX 7
#define TX 8
#define RELAY 5

SoftwareSerial bluetooth(RX, TX);
bool active = false;

void setup() {
  pinMode(RX,INPUT);
  pinMode(TX,OUTPUT);
  bluetooth.begin(38400);
  while (!bluetooth);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);
}

void loop() {
  if (bluetooth.available() > 0) {
    byte command = bluetooth.read();
    blurtooth.clear();
    if (command == '1') {
      if (active) {
        digitalWrite(RELAY, HIGH);
        active = ! active;
      }
      if (!active) {
        digitalWrite(RELAY, LOW);
        active = ! active;
      }
    }
  }
}
