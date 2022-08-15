#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal.h>

// CONEXIONES NECESARIAS
const int dhtPin=8;                               //Pin de dato del sensor
const int rs=12, en=11, d4=4, d5=5, d6=6, d7=7;   //Pines del display
const int relayCool=9, relayHeat=10;              //Pines de los reles (realyCool = Celda peltier | relayHeat = Resistencia)
const int buttonUp=2, buttonDown=3;               //Botones de config.

// VARIABLES
int tempInt;
int tempSet = 24;
int tempMod;
bool heatState = 1;
int lastButtonState = LOW;

// INICIALIZACION
DHT dht(dhtPin, DHT11);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Funciones
/*Funcion debounce adaptada de https://www.arduino.cc/en/Tutorial/BuiltInExamples/Debounce
  TODO: Pensar forma de que la variable lastButtonState este dentro de la función.*/
int debounce (int reading) {
  const int debounceDelay = 50;
  int buttonState = LOW;
  unsigned long lastDebounceTime;
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        lastButtonState = buttonState;
        return buttonState;
      }
      lastButtonState = buttonState;
      return buttonState;
    }
  }
  return buttonState;
}

int readAndPrint(DHT sensor, LiquidCrystal display){
  int read = sensor.readTemperature();
  display.setCursor(10, 0);
  display.print(read);
  return read;
}

bool tempUpdate (LiquidCrystal display, int temp1, int temp2) {
  display.setCursor(10, 1);
  display.print(temp1);
  if (temp1 >= temp2) {return 1;}
  else {return 0;}
}



void setup() {                  //TODO: comunicación puerto serie.
  dht.begin();
  lcd.begin(16, 2);
  pinMode(buttonUp, INPUT);
  pinMode(buttonDown, INPUT);
  pinMode(relayCool, OUTPUT);
  pinMode(relayHeat, OUTPUT);
  lcd.print("TempInt: ");
  lcd.setCursor(0,1);
  lcd.print("TempSet: ");
  digitalWrite(relayCool, LOW);
  digitalWrite(relayHeat, LOW);
}

// TODO: Comentar codigo

void loop() {

  bool tempChange = 0;

  tempInt = readAndPrint(dht, lcd);

  int tempUp = debounce(digitalRead(buttonUp));
  int tempDown = debounce(digitalRead(buttonDown));

  if (tempUp != tempDown) {
    reset:
    if (tempUp == HIGH) {tempSet++;}
    else {tempSet--;}
    tempChange = 1;
  }

  if (tempChange) {
    heatState = tempUpdate(lcd, tempSet, tempInt);
    if (heatState) {tempMod = 2;}
    else {tempMod = -2;}
  }
  //control de los reles
  if ((tempSet + tempMod) > tempInt) {
    digitalWrite(relayCool, LOW);    
    digitalWrite(relayHeat, HIGH);
  }
  else if ((tempSet + tempMod) < tempInt) {
    digitalWrite(relayCool, HIGH);
    digitalWrite(relayHeat, LOW);
  }
  else {
    digitalWrite(relayCool, LOW);
    digitalWrite(relayHeat, LOW);
    while (tempSet > tempInt || tempSet < tempInt) {    
      tempInt = readAndPrint(dht, lcd);
      tempUp = debounce(digitalRead(buttonUp));
      tempDown = debounce(digitalRead(buttonDown));
      if (tempUp != tempDown) {
        goto reset;
      }
    }
  }
}