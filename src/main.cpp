#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal.h>

// CONEXIONES NECESARIAS
const int dhtPin=4;                                //Pin de dato del sensor
const int rs=7, en=8, d4=9, d5=10, d6=11, d7=12;   //Pines del display
const int relayCool=5, relayHeat=6;                //Pines de los reles (realyCool = Celda peltier | relayHeat = Resistencia)
const int buttonUp=2, buttonDown=3;                //Botones de config.

// VARIABLES
/*-------------------Flags-----------------------*/

bool heatState = 1;
byte operationState = 0;

/*-------------------Botonera---------------------*/

bool lastBStateUp = LOW;
bool lastBStateDown = LOW;

/*-------------------Temperatura-----------------------*/

int tempRead;
byte userTemp = 24;

// INICIALIZACION

DHT dht(dhtPin, DHT11);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Funcion debounce adaptada de https://www.arduino.cc/en/Tutorial/BuiltInExamples/Debounce
bool debounce(int buttonPin, bool lastButtonState) {
  unsigned long lastDebounceTime;
  unsigned long debounceDelay = 50;
  bool buttonState;

  bool reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {buttonState = reading;}
  }

  return buttonState;
}

//Función que actualiza la temperatura seteada
byte userTempUpdate(bool bStateUp, bool bStateDown, byte userValue) {
  if (bStateUp && userValue < 30) {userValue += 1;}
  if (bStateDown && userValue > 5) {userValue -= 1;}
  return userValue;
}

bool stopState(bool heatMode, int readTemp, byte userValue, byte modifier, bool lastStopState) {
  bool stop = 0;

  if (!lastStopState) {
    
    if (heatMode) {
      if (readTemp > userValue) {
        if (readTemp >= userValue + modifier) {
          stop = 1;
        }
      }
    }
    else {
      if (readTemp < userValue) {
        if (readTemp <= userValue - modifier) {
          stop = 1;
        }
      }
    }

  }

  else {
    if (heatMode) {
      if (readTemp < userValue) {
        stop = 0;
      }
    }
    else {
      if (readTemp > userValue) {
        stop = 0;
      }
    }

  }

  return stop;
}

void operation(int actuator0, int actuator1, int state) { 
  switch (state) {
    case 0:
      digitalWrite(actuator0, LOW);
      digitalWrite(actuator1, LOW);
      break;
    case 1:
      digitalWrite(actuator0, HIGH);
      digitalWrite(actuator1, LOW);
      break;
    case 2:
      digitalWrite(actuator0, LOW);
      digitalWrite(actuator1, HIGH);
    default:
      break;
  }
}

void setup() {
  dht.begin();
}

void loop() {
  tempRead = dht.readTemperature();
  lastBStateUp = debounce(buttonUp, lastBStateUp);
  lastBStateDown = debounce(buttonDown, lastBStateDown);
  if (lastBStateUp != lastBStateDown) {
    userTemp = userTempUpdate(lastBStateUp, lastBStateDown, userTemp);
    heatState = (tempRead <= userTemp);
  }
  operation(relayHeat, relayCool, operationState);
}
