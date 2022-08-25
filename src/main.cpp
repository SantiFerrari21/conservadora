#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal.h>

// TODO: Comentar codigo

// CONEXIONES NECESARIAS
const int dhtPin=8;                               //Pin de dato del sensor
const int rs=12, en=11, d4=4, d5=5, d6=6, d7=7;   //Pines del display
const int relayCool=9, relayHeat=10;              //Pines de los reles (realyCool = Celda peltier | relayHeat = Resistencia)
const int buttonUp=2, buttonDown=3;               //Botones de config.

bool clockFlag = 0;
bool* ptrClock = &clockFlag;
bool standByFlag = 0;
bool* ptrStandBy = &standByFlag;
int relayState = 0;
int* ptrRState = &relayState;

// VARIABLES

int lastBStateUp = LOW;
int* ptrLastBSUp = &lastBStateUp;
int lastBStateDown = LOW;
int* ptrLastBSDown = &lastBStateDown;
int tempInt;
int* ptrTempInt = &tempInt;
int tempSet = 24;
int* ptrTempSet = &tempSet;
int tempMod = 2;
int* ptrMod = &tempMod;


// INICIALIZACION
DHT dht(dhtPin, DHT11);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//FUNCIONES TODO: Renombrar las funciones para que sean intuitivas
/*Funcion debounce adaptada de https://www.arduino.cc/en/Tutorial/BuiltInExamples/Debounce*/
int debounce (int button, int* ptrLastBState) {
  unsigned long lastDebounceTime;
  unsigned long debounceDelay = 50;
  int buttonState;

  int reading = digitalRead(button);

  if (reading != *ptrLastBState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {buttonState = reading;}
  }

  *ptrLastBState = buttonState;
  
  return buttonState;
}

void measure(bool* cFlag, int* intTemp) {       //TODO: implementar chekeo de NaN y funcion para promediar
  if (*cFlag) {
    *intTemp = dht.readTemperature();
    *cFlag = 0;
  }
}

void keypad(int bState0, int bState1, int* setTemp, bool* sBFlag) {
  if (bState0 != bState1){
    if (bState0) {*setTemp += 1;}
    else {*setTemp -= 1;}
    *sBFlag = 0;
  }
}

void checker(int intTemp, int setTemp, int* mod, int* state, bool* sBFlag) {                  //TODO: pensar en como desacoplar mas esta funcion
  if ((*mod > 0 && intTemp <= setTemp) || (*mod < 0 && intTemp >= setTemp)) {*sBFlag = 0;}    
  
  if (!*sBFlag) {
    *mod = (setTemp >= intTemp) ? *mod : -*mod;
    int modTemp = setTemp + *mod;
    if ((*mod > 0 && intTemp >= modTemp) || (*mod < 0 && intTemp <= modTemp)) {*state = 0;}
    else if (modTemp >= intTemp) {*state = 1;}
    else {*state = 2;}
  }
}

void operation(int actuator0, int actuator1, int* state, bool* sBFlag) { 
  switch (*state) {
    case 0:
      actuator0 = LOW;
      actuator1 = LOW;
      *sBFlag = 1;
      break;
    case 1:
      actuator0 = HIGH;
      actuator1 = LOW;
      break;
    case 2:
      actuator0 = LOW;
      actuator1 = HIGH;
    default:
      break;
  }
}

void display(int intTemp, int setTemp) {
  lcd.setCursor(10, 0);
  lcd.print(intTemp);
  lcd.setCursor(10, 1);
  lcd.print(setTemp);
}

void serialInfo(bool temp = 1, bool relays = 1) {                 //TODO: implementar delay entre mensajes
  Serial.println("----------------------------------------");
  if (temp) {
    Serial.println("---------------TEMP  INFO---------------");
    Serial.print("Temp. Int: ");
    Serial.println(tempInt);
    Serial.print("Temp. Set: ");
    Serial.println(tempSet);
    Serial.print("Modifier: ");
    Serial.println(tempMod);
  }
  if (relays) {
    Serial.println("---------------RELAY INFO---------------");
    Serial.print("RelayHeat: ");
    Serial.println(bitRead(PORTD, relayHeat));
    Serial.print("RelayCool: ");
    Serial.println(bitRead(PORTD, relayCool));
    Serial.print("RelayState:");
    Serial.println(relayState);
    Serial.print("RelayStandBy:");
    Serial.println(standByFlag);
  } 
  Serial.println("----------------------------------------"); 
}

void setup() {
  Serial.begin(9600);
  Serial.println("INIT...");
  dht.begin();
  lcd.begin(16, 2);
  lcd.print("TempInt: ");
  lcd.setCursor(0, 1);
  lcd.print("TempSet: ");
  pinMode(buttonUp, INPUT);
  pinMode(buttonDown, INPUT);
  pinMode(relayCool, OUTPUT);
  pinMode(relayHeat, OUTPUT);
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= B00000010;
  OCR1A = 62500;
  sei();
  Serial.println("READY");
}

void loop() {
  measure(ptrClock, ptrTempInt);
  int buttonStateUp = debounce(buttonUp, ptrLastBSUp);
  int buttonStateDown = debounce(buttonDown, ptrLastBSDown);
  keypad(buttonStateUp, buttonStateDown, ptrTempSet, ptrStandBy);
  checker(tempInt, tempSet, ptrMod, ptrRState, ptrStandBy);
  operation(relayHeat, relayCool, ptrRState, ptrStandBy);
  display(tempInt, tempSet);
  serialInfo();
}

ISR(TIMER1_COMPA_vect) {
  TCNT1 = 0;
  *ptrClock = 1;
}