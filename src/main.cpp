#include <Arduino.h>

const int dataPin = 2;
const int latchPin = 3;
const int clockPin = 4;
const int D2 = 11;
const int D3 = 12;
const int D4 = 13;
const int keyboardInputDelay = 300;
unsigned long lastKeyboardInput = 0;

const byte numRows = 4;
const byte numCols = 4;
const byte colsPins = 2;

const byte rowPins[numRows] = {5,6,7,8};
const byte colPins[numCols] = {9,10,A2,A3};
const byte analogSuccessPin = A0;
const byte analogUnsuccessPin = A1;
const String rightAnswer = "1234";
String input = "";
int display[4] = {-1,-1,-1,-1};

char keymap[numRows][numCols] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*', '0','#','D'}
};

bool characterIsDigit(char character) {
  return character >= '0' && character <= '9';
}

void writeAnalogBool(int pin, bool high) {
  if (high) analogWrite(pin, 1024);
  else analogWrite(pin, 0);
}

bool notNumber(int to_check, int lastArg) {
  return to_check != lastArg;
}

template<typename... Args>
bool notNumber(int to_check, int firstArg, Args... args) {
  if (to_check == firstArg) return false;
  return notNumber(to_check, args...);
}

bool containsNumber(int to_check, int lastArg) {
  return to_check == lastArg;
}

template<typename... Args>
bool containsNumber(int to_check, int firstArg, Args... args) {
  if (to_check == firstArg) return true;
  return containsNumber(to_check, args...);
}

byte shiftedBoolBit(int shift, bool enabled) {
  if (!enabled) return 0x00;
  return (1 << shift);
}

byte displayBitMask(int digit, bool setFirstDigit) {
  if (digit > 9) {
    Serial.println("not valid number " + digit);
  }
  byte bitMask = 0x00;
  if (digit >= 0) {
    bitMask |= shiftedBoolBit(1, containsNumber(digit, 5, 6));
    bitMask |= shiftedBoolBit(0, containsNumber(digit, 1, 4));
    bitMask |= shiftedBoolBit(2, containsNumber(digit, 2));
    bitMask |= shiftedBoolBit(3, containsNumber(digit, 1, 4, 7));
    bitMask |= shiftedBoolBit(4, notNumber(digit, 0, 2, 6, 8));
    bitMask |= shiftedBoolBit(5, containsNumber(digit, 1, 2, 3, 7));
    bitMask |= shiftedBoolBit(6, containsNumber(digit, 0, 1, 7));
  } else bitMask = 0b011111111;
  if (setFirstDigit) bitMask |= (1 << 7);
  return bitMask;
}

void writeShift(byte toWrite) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, toWrite);
  digitalWrite(latchPin, HIGH);
}

void writeDisplay(int digit, int display) {
  bool displayIsFirst = display == 0;
  byte bitMask = displayBitMask(digit, displayIsFirst);
  if (!displayIsFirst) {
    int displayPin = D2 - 1 + display;
    //for (int i = 1; i < 4; i++) {
    //  if (display != i) digitalWrite(D2 - 1 + i, LOW);
    //}
    if ((bitMask & 0b01111111) == 0b01111111) {
      digitalWrite(displayPin, LOW);
      return;
    }
    writeShift(bitMask);
    digitalWrite(displayPin, HIGH);
    delay(1);
    digitalWrite(displayPin, LOW);
  } else {
    writeShift(bitMask);
    delay(1);
    writeShift(bitMask & 0b01111111);
  }
}

char readCharacter() {
  if (millis() - lastKeyboardInput < keyboardInputDelay) return '\0';
  for (int col = 0; col < numCols; col++) {
    digitalWrite(colPins[col], LOW);
    for (int row = 0; row < numRows; row++) {
      if (digitalRead(rowPins[row]) == LOW) {
        lastKeyboardInput = millis();
        digitalWrite(colPins[col], HIGH);
        return keymap[row][col];
      }
    }
    digitalWrite(colPins[col], HIGH);
  }
  return '\0';
}

void setup() {
  Serial.begin(9600);
  Serial.println("start");
  for (int row = 0; row < numRows; row++) {
    pinMode(rowPins[row], INPUT_PULLUP);
  }

  
  pinMode(colPins[0], OUTPUT);
  pinMode(colPins[1], OUTPUT);
  pinMode(colPins[2], OUTPUT);
  pinMode(colPins[3], OUTPUT);
  for (int col = 0; col < 4; col++) {
    digitalWrite(colPins[col], HIGH);
  }

  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  writeShift(0x00);

  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
}

void updateDisplay() {
  for (int i = 0; i < 4; i++) {
    writeDisplay(display[i], i);
  }
}


void loop() {
  updateDisplay();
  char inputChar = readCharacter();
  if (inputChar == '\0') return;
  if (input.length() >= rightAnswer.length()) return;
  if (!characterIsDigit(inputChar)) return;
  int inputDigit = inputChar - '0';
  input += inputChar;
  int index = input.length() - 1;
  if (index >= 4) {
    return;
  }
  display[index] = inputDigit;
  if (input.length() < rightAnswer.length()) return;
  if (rightAnswer.compareTo(input) == 0) {
    writeAnalogBool(analogSuccessPin, true);
  } else writeAnalogBool(analogUnsuccessPin, true);
}