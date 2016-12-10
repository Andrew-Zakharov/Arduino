#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <MFRC522.h>
#include <Wire.h>
#include <SPI.h>
#include "CardReader.h"

#define SS_PIN 10
#define RST_PIN 9

#define RX 8
#define TX 7
#define BLUETOOTH_STATE 6
#define AT_MODE 5

#define LCD_ADDRESS 0x3F
#define LCD_LINES 2
#define LCD_CHARS 16

#define MASTER_CARD 1949178666
#define ON "Power_On"
#define OFF "Power_Off"

MFRC522 cardReader(SS_PIN, RST_PIN);
SoftwareSerial bluetooth(RX, TX);
LiquidCrystal_I2C screen(LCD_ADDRESS, LCD_CHARS, LCD_LINES);

byte master[4] = {0x74, 0x2E, 0x1B, 0x2A};

unsigned long* cards = NULL;
byte numberOfCards = 0;

bool masterDetected = false;
bool bluetoothConnected = false;
bool power = false;

bool isMaster(unsigned long uid);
bool isAuthorised(unsigned long uid);
unsigned long toDecimal(byte uid[], byte size);
void printOnScreen(const char* first = NULL, const char* second = NULL);


void setup() {
  pinMode(RX, INPUT);
  pinMode(TX, OUTPUT);
  pinMode(BLUETOOTH_STATE,INPUT);
  bluetooth.begin(38400);

  screen.init();
  screen.backlight();

  SPI.begin();
  cardReader.PCD_Init();

  printOnScreen("Waiting for", "connection...");

  while(!bluetoothConnected){
    if(digitalRead(BLUETOOTH_STATE) == HIGH){
      bluetoothConnected = true;
    }
    delay(80);
  }
  printOnScreen("Waiting for a ", "card...");
}

void loop() {
  if (!cardReader.PICC_IsNewCardPresent()) {
    return;
  }

  if (!cardReader.PICC_ReadCardSerial()) {
    return;
  }

  if (cardReader.PICC_GetType(cardReader.uid.sak) != MFRC522::PICC_TYPE_MIFARE_1K &&
      cardReader.PICC_GetType(cardReader.uid.sak) != MFRC522::PICC_TYPE_MIFARE_4K) {
    return;
  }

  unsigned long cardUid = toDecimal(cardReader.uid.uidByte, cardReader.uid.size);

  if (masterDetected) {
    if (isMaster(cardUid)) {
      printOnScreen("Abort");
      masterDetected = false;
      delay(1000);
      printOnScreen("Waiting for a ", "card...");
      return;
    }
    if (isExist(cardUid)) {
      deleteCard(cardUid);
      printOnScreen("Card is deleted");
    } else {
      addNewCard(cardUid);
      printOnScreen("Card is added");
    }
    masterDetected = false;
    delay(1000);
    printOnScreen("Waiting for a ", "card...");
    return;
  }

  if (isMaster(cardUid)) {
    printOnScreen("Attach card");
    masterDetected = true;
  } else {
    if (isAuthorised(cardUid)) {
      printOnScreen("Access granted");
      if (!power) {
        bluetooth.println(ON);
        power = true;
      } else {
        if (power) {
          bluetooth.println(OFF);
          power = false;
        }
      }
    } else {
      printOnScreen("Access denied");
    }
    delay(1000);
    printOnScreen("Waiting for a ", "card...");
  }
  delay(1000);
}

bool isMaster(unsigned long uid) {
  if (uid == MASTER_CARD) {
    return true;
  } else {
    return false;
  }
}

bool isAuthorised(unsigned long uid) {
  for (byte i = 0; i < numberOfCards; i++) {
    if (*(cards + i) == uid) {
      return true;
    }
  }
  return false;
}

bool isExist(unsigned long uid) {
  for (byte i = 0; i < numberOfCards; i++) {
    if (*(cards + i) == uid) {
      return true;
    }
  }
  return false;
}

unsigned long toDecimal(byte uid[], byte size) {
  unsigned long decimalUid;
  for (byte i = 0; i < size; i++) {
    decimalUid = decimalUid * 256 + uid[i];
  }
  return decimalUid;
}

void printOnScreen(const char* first, const char* second) {
  if (!first && !second) {
    return;
  }

  screen.clear();
  screen.home();
  if (first) {
    screen.print(first);
  }
  if (second) {
    screen.setCursor(0, 1);
    screen.print(second);
  }
}

void addNewCard(unsigned long cardUid) {
  numberOfCards = numberOfCards + 1;
  cards = (unsigned long*)realloc(cards,numberOfCards * sizeof(unsigned long));

  *(cards + numberOfCards - 1) = cardUid;
}

void deleteCard(unsigned long cardUid) {
  unsigned long* temp = (unsigned long*)malloc(numberOfCards);
  for (byte i = 0; i < numberOfCards; i++) {
      *(temp + i) = *(cards + i);
  }
  free(cards);
  cards = (unsigned long*)malloc((numberOfCards - 1) * sizeof(unsigned long*));
  for (byte i = 0,j = 0; i < numberOfCards; i++) {
    if(*(temp + i) != cardUid){
      *(cards + j) = *(temp + i);
      j++;
    }
  }
  free(temp);
  temp = NULL;
}

