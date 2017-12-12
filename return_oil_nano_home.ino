#include <LiquidCrystal_I2C.h>
#include <NewPing.h>
#include<EEPROM.h>
#define TRIG 2
#define ECHO 3
#define COIL 4
#define rpt 10
float d, dis, perc, blocks, rd[rpt];
int top, bottom;
boolean coil = false, s_flt = false, debug = false;
LiquidCrystal_I2C lcd(0x27, 16, 2);
NewPing us(TRIG, ECHO, 200);
uint8_t block[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
void setup() {
  digitalWrite(COIL, HIGH);
  Serial.begin(115200);
  lcd.begin();
  lcd.createChar(0, block);
  lcd.home();
  top = EEPROM.read(0);
  bottom = EEPROM.read(1);
  if (top > 50)
  {
    top = 12; bottom = 50;
    EEPROM.write(0, top);
    EEPROM.write(1, bottom);
  }
  dis = bottom;
  for (int i = 0; i < rpt; i++) {
    rd[i] = bottom;
  }
}

void loop() {
  measure();
  sensorFault();
  operateCoil();
  checkInput();
  if (debug)
    adjust();
  else
    lcdDispaly();
  serialOut();
}
void measure() {
  d = us.ping_median() * 0.0175;
  for (int i = 0; i < rpt - 1; i++)
  {
    rd[i] = rd[i + 1];
  }
  rd[rpt - 1] = d;
  dis = dis - rd[0] / rpt + d / rpt;
  perc = map(dis, bottom, top, 0, 100);
  blocks = map(perc, 0, 100, 0, 16);
}
void sensorFault()
{
  if (d < 3 || d > 200)
    s_flt = true;
}
void operateCoil()
{
  if (perc > 95 && !coil)
  {
    coil = true;
    digitalWrite(COIL, LOW);
  }
  if (perc < 90 && coil)
  {
    coil = false;
    digitalWrite(COIL, HIGH);
  }
}
void checkInput() {
  if (!digitalRead(8))
  {
    delay(500);
    if (!digitalRead(8))
    {
      debug = !debug;
      EEPROM.write(0, top);
      EEPROM.write(1, bottom);
    }
  }
}
void lcdDispaly()
{
  lcd.clear();
  for (int i = 0; i < 16; i++)
  {
    lcd.write(0);
  }
  lcd.setCursor(0, 1);
  lcd.print(perc);
  lcd.print("% ");
  if (coil)
    lcd.print("RUN");
  else
    lcd.print("TRIP");
}
void adjust()
{
  if (!digitalRead(9))
    top--;
  if (!digitalRead(10))
    top++;
  if (!digitalRead(10))
    bottom--;
  if (!digitalRead(12))
    bottom++;
}
void serialOut() {
  Serial.println("d:");
  Serial.print(d);
  Serial.println("dis:");
  Serial.print(dis);
  Serial.println("perc:");
  Serial.print(perc);
  Serial.println("blocks:");
  Serial.print(blocks);
  Serial.println("s_flt:");
  Serial.print(s_flt);
  Serial.println("coil:");
  Serial.print(coil);
  Serial.println("debug:");
  Serial.print(debug);
  Serial.println("top:");
  Serial.print(top);
  Serial.println("bottom:");
  Serial.print(bottom);
}

