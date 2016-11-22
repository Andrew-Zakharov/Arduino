#include <SoftwareSerial.h>
#include <Wire.h>

#define RX 8
#define TX 7
#define RELAY 5

#define ON "Power-1\r\n"
#define OFF "Power-0\r\n"
#define KEY_SIZE 9

SoftwareSerial bluetooth(RX, TX);

void setup() {
  pinMode(RX,INPUT);
  pinMode(TX,OUTPUT);
  bluetooth.begin(38400);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);
}

void loop() {
  byte i = 0;
  char received[KEY_SIZE + 1] = {0};
  bool dataExist = bluetooth.available();
  
  if(dataExist){
    do{
      if(i > KEY_SIZE){
        break;
      }
      received[i] = bluetooth.read();
      i++;
      delay(10);
    }while(bluetooth.available() > 0);

    if(strcmp(received,ON) == 0){
      digitalWrite(RELAY,LOW);
    }else{
      if(strcmp(received,OFF) == 0){
        digitalWrite(RELAY,HIGH);
      }
    }
  }
  delay(1000);
}
