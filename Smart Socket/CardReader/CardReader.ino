#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <MFRC522.h>
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>

#define SS_PIN 10
#define RST_PIN 9

#define RX 8
#define TX 7
#define BLUETOOTH_STATE 6
#define AT_MODE 5

#define LCD_ADDRESS 0x3F
#define LCD_LINES 2
#define LCD_CHARS 16

#define ON "Power_On"
#define OFF "Power_Off"

#define EMPTY 0xFF

struct card {
  byte* cardUid;
  byte uidSize;
  byte valid;
  byte memoryAddress;
} card;

struct memoryBlock {
  byte beginning;
  byte blockSize;
} memoryBlock;

bool isMaster(struct card card);
bool isAuthorised(struct card card);
bool isExist(struct card card);
bool compare(const struct card* first, const struct card* second);
void printOnScreen(const char* first = NULL, const char* second = NULL);
void readCardsFromMemory();
void writeCardToMemory(struct card* card);
void deleteCardFromMemory(struct card card);
void addNewCard(struct card card);
void deleteCard(struct card card);
void copyCard(struct card* destination, const struct card* source);

MFRC522 cardReader(SS_PIN, RST_PIN);
SoftwareSerial bluetooth(RX, TX);
LiquidCrystal_I2C screen(LCD_ADDRESS, LCD_CHARS, LCD_LINES);

struct card masterCard;

int readAddress = 0;
int writeAddress = 0;
struct card* cards = NULL;
struct memoryBlock* freeMemory = NULL;
byte numberOfFreeBlocks = 0;
byte numberOfCards = 0;

bool masterDetected = false;
bool bluetoothConnected = false;
bool powerStatus = false;


void setup() {
  pinMode(RX, INPUT);
  pinMode(TX, OUTPUT);
  pinMode(BLUETOOTH_STATE, INPUT);
  bluetooth.begin(38400);
  Serial.begin(9600);

  screen.init();
  screen.backlight();

  SPI.begin();
  cardReader.PCD_Init();

  byte address = 0;

  masterCard.memoryAddress = address;
  masterCard.uidSize = EEPROM.read(address);
  address++;
  masterCard.valid = EEPROM.read(address);
  address++;
  masterCard.cardUid = (byte*)malloc(masterCard.uidSize * sizeof(byte));

  for (byte i = 0; i < masterCard.uidSize; i++) {
    *(masterCard.cardUid + i) = EEPROM.read(address);
    address++;
    delay(100);
  }

  readAddress = address;

  printOnScreen("Reading cards", "from memory...");
  readCardsFromMemory();

  printOnScreen("Waiting for", "connection...");

  while (!bluetoothConnected) {
    if (digitalRead(BLUETOOTH_STATE) == HIGH) {
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

  struct card newCard;

  newCard.uidSize = cardReader.uid.size;
  newCard.valid = 1;
  newCard.cardUid = (byte*)malloc(newCard.uidSize * sizeof(byte));
  newCard.memoryAddress = EMPTY;

  for (byte i = 0; i < newCard.uidSize; i++) {
    *(newCard.cardUid + i) = cardReader.uid.uidByte[i];
  }

  if (masterDetected) {
    if (compare(&newCard, &masterCard)) {
      printOnScreen("Abort");
      masterDetected = false;
      delay(1000);
      printOnScreen("Waiting for a ", "card...");
      return;
    }
    if (isExist(newCard)) {
      deleteCard(newCard);
      printOnScreen("Card is deleted");
    } else {
      addNewCard(newCard);
      printOnScreen("Card is added");
    }
    masterDetected = false;
    delay(1000);
    printOnScreen("Waiting for a ", "card...");
    return;
  }

  if (compare(&newCard, &masterCard)) {
    printOnScreen("Attach card");
    masterDetected = true;
  } else {
    if (isAuthorised(newCard)) {
      printOnScreen("Access granted");
      if (!powerStatus) {
        bluetooth.println(ON);
        powerStatus = true;
      } else {
        if (powerStatus) {
          bluetooth.println(OFF);
          powerStatus = false;
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

bool isMaster(struct card card) {
  if (masterCard.uidSize != card.uidSize) {
    return false;
  }

  byte count = 0;

  for (byte i = 0; i < card.uidSize; i++) {
    if (compare(*(masterCard.cardUid + i), *(masterCard.cardUid + i))) {
      count++;
    }
  }

  if (count == card.uidSize) {
    return true;
  } else {
    return false;
  }
}

bool isAuthorised(struct card card) {
  for (byte i = 0; i < numberOfCards; i++) {
    if (compare((cards + i), &card)) {
      return true;
    }
  }
  return false;
}

bool isExist(struct card card) {
  for (byte i = 0; i < numberOfCards; i++) {
    if (compare((cards + i), &card)) {
      return true;
    }
  }
  return false;
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

void addNewCard(struct card card) {
  numberOfCards = numberOfCards + 1;
  cards = (struct card*)realloc(cards, (numberOfCards * sizeof(struct card)));

  (cards + numberOfCards - 1)->uidSize = card.uidSize;
  (cards + numberOfCards - 1)->valid = card.valid;
  (cards + numberOfCards - 1)->cardUid = (byte*)malloc(card.uidSize * sizeof(byte));

  for (byte i = 0; i < card.uidSize; i++) {
    *((cards + numberOfCards - 1)->cardUid + i) = *(card.cardUid + i);
  }

  writeCardToMemory((cards + numberOfCards - 1));
}

void deleteCard(struct card card) {
  struct card* temp = (struct card*)malloc(numberOfCards * sizeof(struct card));
  for (byte i = 0; i < numberOfCards; i++) {
    copyCard((temp + i), (cards + i));
  }

  free(cards);

  cards = (struct card*)malloc((numberOfCards - 1) * sizeof(struct card));

  for (byte i = 0, j = 0; i < numberOfCards; i++) {
    if (compare((temp + i), &card)) {
      Serial.print("Deleting card address = ");
      Serial.println((temp + i)->memoryAddress, DEC);
      card.memoryAddress = (temp + i)->memoryAddress;
      continue;
    } else {
      copyCard((cards + j), (temp + i));
      j++;
    }
  }
  free(temp);
  temp = NULL;

  if(card.memoryAddress != EMPTY){
    deleteCardFromMemory(card);
  }
}

bool compare(const struct card* first, const struct card* second) {
  if (first->uidSize != second->uidSize) {
    return false;
  }

  byte count = 0;

  for (byte i = 0; i < first->uidSize; i++) {
    if (*(first->cardUid + i) == *(second->cardUid + i)) {
      count++;
    }
  }

  if (count == first->uidSize) {
    return true;
  } else {
    return false;
  }
}

void copyCard(struct card* destination, const struct card* source) {
  destination->uidSize = source->uidSize;
  destination->valid = source->valid;
  destination->memoryAddress = source->memoryAddress;
  for (byte i = 0; i < destination->uidSize; i++) {
    *(destination->cardUid + i) = *(source->cardUid + i);
  }
}

void readCardsFromMemory() {
  byte cardSize = 0;
  while ((cardSize = EEPROM.read(readAddress)) != EMPTY) {
   /* byte cardSize = EEPROM.read(readAddress);
    if (cardSize == 0xFF) {
      break;
    }*/
    readAddress++;
    delay(100);
    byte valid = EEPROM.read(readAddress);
    readAddress++;

    if (valid == 0) {
      numberOfFreeBlocks++;
      if (!freeMemory) {
        freeMemory = (struct memoryBlock*)malloc(numberOfFreeBlocks * sizeof(struct memoryBlock));
      } else {
        freeMemory = (struct memoryBlock*)realloc(freeMemory, numberOfFreeBlocks * sizeof(struct memoryBlock));
      }

      (freeMemory + numberOfFreeBlocks - 1)->beginning = readAddress - 2;
      (freeMemory + numberOfFreeBlocks - 1)->blockSize = cardSize;

      readAddress += cardSize;
    } else {
      numberOfCards++;
      if (!cards) {
        cards = (struct card*)malloc(numberOfCards * sizeof(struct card));
      } else {
        cards = (struct card*)realloc(cards, numberOfCards * sizeof(struct card));
      }

      (cards + numberOfCards - 1)->uidSize = cardSize;
      (cards + numberOfCards - 1)->valid = valid;
      (cards + numberOfCards - 1)->cardUid = (byte*)malloc(cardSize * sizeof(byte));
      (cards + numberOfCards - 1)->memoryAddress = readAddress - 2;

      Serial.print("Card size = ");
      Serial.println((cards + numberOfCards - 1)->uidSize, DEC);
      Serial.print("Valid = ");
      Serial.println((cards + numberOfCards - 1)->valid, DEC);
      Serial.print("Memory address = ");
      Serial.println((cards + numberOfCards - 1)->memoryAddress, DEC);

      for (byte i = 0; i < (cards + numberOfCards - 1)->uidSize; i++) {
        *((cards + numberOfCards - 1)->cardUid + i) = EEPROM.read(readAddress);
        readAddress++;
        Serial.println(*((cards + numberOfCards - 1)->cardUid + i), HEX);
        delay(100);
      }
    }
    delay(100);
  }
  writeAddress = readAddress;
}

void writeCardToMemory(struct card* card) {
  if (freeMemory) {
    for (byte i = 0; i < numberOfFreeBlocks; i++) {
      if ((freeMemory + i)->blockSize == card->uidSize) {
        writeAddress = (freeMemory + i)->beginning;
        break;
      }
    }
  }

  card->memoryAddress = writeAddress;

  EEPROM.write(writeAddress, card->uidSize);
  writeAddress++;
  delay(100);
  EEPROM.write(writeAddress, card->valid);
  writeAddress++;
  delay(100);
  for (byte i = 0; i < card->uidSize; i++) {
    EEPROM.write(writeAddress, *(card->cardUid + i));
    writeAddress++;
    delay(100);
  }
}

void deleteCardFromMemory(struct card card){
  numberOfFreeBlocks++;
  if(!freeMemory){
    freeMemory = (struct memoryBlock*)malloc(numberOfFreeBlocks * sizeof(struct memoryBlock));
  }else{
    freeMemory = (struct memoryBlock*)realloc(freeMemory, numberOfFreeBlocks * sizeof(struct memoryBlock));
  }

  (freeMemory + numberOfFreeBlocks - 1)->beginning = card.memoryAddress;
  (freeMemory + numberOfFreeBlocks - 1)->blockSize = card.uidSize;

  Serial.print("Write 0 to ");
  Serial.println(card.memoryAddress + 1,DEC);
  EEPROM.write(card.memoryAddress + 1, 0);
}

