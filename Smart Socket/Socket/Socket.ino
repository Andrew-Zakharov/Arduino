#include <SoftwareSerial.h>
#include <Wire.h>

#define RX 8
#define TX 7
#define RELAY 5

#define ON "Power_On\r\n"
#define OFF "Power_Off\r\n"

SoftwareSerial bluetooth(RX, TX);
bool dataExist = false;

void setup() {
  pinMode(RX, INPUT);
  pinMode(TX, OUTPUT);
  bluetooth.begin(38400);
  Serial.begin(9600);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);
}

void loop() {
  dataExist = bluetooth.available();

  if (dataExist) {
    String received;
    do {
      if(received.endsWith("\n")){
        break;
      }
      received += (char)bluetooth.read();
      delay(50);
    } while (bluetooth.available() > 0);

    Serial.println(received);
    if (received.equals(String(ON))) {
      digitalWrite(RELAY, LOW);
    }
    if (received.equals(String(OFF))) {
      digitalWrite(RELAY, HIGH);
    }
  }
}

