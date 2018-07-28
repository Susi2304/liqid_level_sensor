#include <LiquidCrystal.h>
#include<EEPROM.h>
#define TRIG  A1
#define ECHO  A2
#define COIL A4
#define ALARM A3
#define repeat 20
#define BOTTOM_BYTE_1 1
#define BOTTOM_BYTE_2 2
#define TOP_BYTE_1 3
#define TOP_BYTE_2 4
#define TRIP_LEVEL_BYTE 5

uint8_t block[8]  = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
unsigned short top, bottom, distance, height, percentage, tripLevel, blocks;
unsigned short button, menu, state;
bool trip, coil, fault;
float reading[repeat];
//0-normal
//1-auth
//2-menu
//3-parameters
//4-fault
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
void setup() {
  //factoryReset();
  eepromRead();
  state = 0;
  trip = false;
  coil = false;
  fault = false;
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(COIL, OUTPUT);
  pinMode(ALARM, OUTPUT);
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.createChar(3, block);
  lcd.home();
}
void loop() {
  measure();
  parametricActions();
  operateCoil();
  getButton();
  if ((state == 0 || state == 4) && button == 4)state = 1;
  if ((state == 0 || state == 4) && button == 1)state = 3;
  show();
  delay(100);
  //serialOut();
}
void measure() {
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(12);
  digitalWrite(TRIG, LOW);
  float d = 0.175 * pulseIn(ECHO, HIGH);
  for (int i = 0; i < (repeat - 1); i++)
  {
    reading[i] = reading[i + 1];
  }
  reading[repeat - 1] = d;
  float temp = 0;
  for (int i = 0; i < repeat; i++)
  {
    temp += reading[i];
  }
  distance = temp / repeat;
  height = bottom - distance;
  percentage = map(height, 0, (bottom - top), 0, 100);
  blocks = map(percentage, 0, 100, 0, 16);
  if (distance < 20) fault = true;
  else fault = false;
}
void parametricActions() {
  if (percentage < tripLevel)
    trip = true;
  if (percentage > tripLevel + 2)
    trip = false;
}
void operateCoil() {
  if (fault || trip) {
    digitalWrite(COIL, LOW);
    alarm();
    coil = false;
  }
  else {
    digitalWrite(COIL, HIGH);
    coil = true;
  }
}
void getButton(void) {
  unsigned short x, x1;
  button = 0;
  x1 = analogRead(A0);
  if (x1 < 1000) {
    delay(50);
    x = analogRead(A0);
    if (abs(x - x1) < 20) {
      if (x < 50) button = 1;
      else if (x < 150) button = 2;
      else if (x < 300) button = 3;
      else if (x < 450) button = 4;
      else if (x < 700) button = 5;
    }
  }
}
void show() {
  lcd.clear();
  if (state == 0)
  {
    for (int i = 0; i < blocks; i++)
      lcd.write(3);
    lcd.setCursor(0, 1);
    lcd.print(percentage);
    lcd.print("% ");
    if (fault)lcd.print("F ");
    else if (!coil) lcd.print("T ");
    else lcd.print("N ");
    lcd.print(distance);
    lcd.print("mm");
  }
  else if (state == 1)
  {
    state = 0;
    int i = 50;
    while (i > 0)
    {
      lcd.clear();
      i--;
      lcd.print(i);
      lcd.setCursor(0, 1);
      getButton();
      lcd.print(button);
      if (button == 5) {
        state = 2;
        menu = 1;
        break;
      }
      delay(100);
    }

  }
  else if (state == 2)
  {
    menuAction();
  }
  else if (state == 3) {
    lcd.print("T:");
    lcd.print(top);
    lcd.print("mm B:");
    lcd.print(bottom);
    lcd.print("mm");
    lcd.setCursor(0, 1);
    lcd.print("P:");
    lcd.print(percentage);
    lcd.print("% TL:");
    lcd.print(tripLevel);
    lcd.print("%");
    state = 0;
  }
  else if (state == 4) {
    fault = true;
    lcd.print("Fault");
  }

}
void menuAction() {
  lcd.clear();
  if (button == 1)menu++;
  if (button == 4)menu--;
  if (menu < 1) menu = 5;
  if (menu > 5) menu = 1;

  if (menu == 1)
  {
    lcd.print("1: BOTTOM");
    lcd.setCursor(0, 1);
    if (button == 2) bottom++;
    else if (button == 3) bottom--;
    lcd.print(bottom);
    lcd.print(" mm");
  }
  else if (menu == 2)
  {
    lcd.print("2: TOP");
    lcd.setCursor(0, 1);
    if (button == 2) top++;
    else if (button == 3) top--;
    lcd.print(top);
    lcd.print(" mm");
  }
  else if (menu == 3)
  {
    lcd.print("2: TRIP LEVEL");
    lcd.setCursor(0, 1);
    if (button == 2) tripLevel++;
    else if (button == 3) tripLevel--;
    lcd.print(tripLevel);
    lcd.print(" %");
  }
  else if (menu == 4)
  {
    lcd.print("3: HARD RESET");
    lcd.setCursor(0, 1);
    lcd.print("press up");
    getButton();
    if (button == 2) {
      factoryReset();
      lcd.clear();
      lcd.print("resetting...");
      delay(5000);
      setup();
    }
  }
  else if (menu == 5)
  {
    lcd.print("4: SAVE & EXIT");
    lcd.setCursor(0, 1);
    lcd.print("press up");
    getButton();
    if (button == 2) {
      eepromWrite();
      lcd.clear();
      lcd.write("saving...");
      delay(5000);
      setup();
    }
  }
}
void serialOut() {
  Serial.print(" distance:");
  Serial.print(distance);
  Serial.print(" height:");
  Serial.print(height);
  Serial.print(" state:");
  Serial.print(state);
  Serial.print(" menu:");
  Serial.print(menu);
  Serial.println();

}
void factoryReset() {
  top = 100;
  bottom = 1000;
  tripLevel = 10;
  eepromWrite();
}
void eepromWrite() {
  int tb1, tb2, bb1, bb2;
  tb1 = top / 10;
  tb2 = top % 10;
  bb1 = bottom / 10;
  bb2 = bottom % 10;
  EEPROM.write(TOP_BYTE_1, tb1);
  EEPROM.write(TOP_BYTE_2, tb2);
  EEPROM.write(BOTTOM_BYTE_1, bb1);
  EEPROM.write(BOTTOM_BYTE_2, bb2);
  EEPROM.write(TRIP_LEVEL_BYTE, tripLevel);
}
void eepromRead() {
  int tb1, tb2, bb1, bb2;
  tb1 = EEPROM.read(TOP_BYTE_1);
  tb2 = EEPROM.read(TOP_BYTE_2);
  bb1 = EEPROM.read(BOTTOM_BYTE_1);
  bb2 = EEPROM.read(BOTTOM_BYTE_2);
  tripLevel = EEPROM.read(TRIP_LEVEL_BYTE);
  top = tb1 * 10 + tb2;
  bottom = bb1 * 10 + bb2;
}

void alarm() {
  analogWrite(ALARM, 130);
  delay(10);
  analogWrite(ALARM, 0);
}



