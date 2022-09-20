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
// INICIALIZACION

DHT dht(dhtPin, DHT11);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// VARIABLES
/*-------------------Flags-----------------------*/

bool clockFlag = 0;
bool heatState = 1;
bool stopState = 1;
byte operationState = 0;

/*-------------------Botonera---------------------*/

bool lastBStateUp = LOW;
bool lastBStateDown = LOW;

/*-------------------Temperatura-----------------------*/

int tempRead;
byte userTemp = 24;

unsigned int seconds = 0;



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

//Función que establece el estado del equipo
bool stopStateUpdate(int readTemp, byte userValue, byte modifier, bool heatFlag, bool lastStopState) {
  bool stop = 0;

  if (!lastStopState) {
    if (heatFlag) {stop = (readTemp > userValue && readTemp >= userValue + modifier);}
    else {stop = (readTemp < userValue && readTemp <= userValue - modifier);}
  }
  else {
    if (heatFlag) {stop = (readTemp >= userValue);}
    else {stop = (readTemp <= userValue);}
    }

  return stop;
}

byte operationStateUpdate(bool stopFlag, bool heatFlag) {
  return stopFlag ? 0 : heatFlag ? 1 : 2; 
}

void operation(int actuator0, int actuator1, int state) { 
  switch (state) {
    case 0:
      digitalWrite(actuator0, HIGH);
      digitalWrite(actuator1, HIGH);
      break;
    case 1:
      digitalWrite(actuator0, LOW);
      digitalWrite(actuator1, HIGH);
      break;
    case 2:
      digitalWrite(actuator0, HIGH);
      digitalWrite(actuator1, LOW);
    default:
      break;
  }
}

void display(int readTemp, byte userValue) {
  lcd.clear();
  lcd.print("TempInt: ");
  lcd.print(readTemp);
  lcd.setCursor(0,1);
  lcd.print("TempSet: ");
  lcd.print(userValue);
}

void serialInfo() {
  Serial.print("operationState: ");
  Serial.println(operationState);
  Serial.print("stopState: ");
  Serial.println(stopState);
  Serial.print("heatState: ");
  Serial.println(heatState);
  
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.begin(16,2);
  pinMode(buttonUp, INPUT);
  pinMode(buttonDown, INPUT);
  pinMode(relayHeat, OUTPUT);
  pinMode(relayCool, OUTPUT);
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= B00000100;
  TIMSK1 |= B00000010; 
  OCR1A = 62500;
  sei();
  Serial.println("READY");
}

void loop() {
  if (clockFlag) {
    seconds++;
    if (seconds % 60 == 0) {serialInfo();}
    if (seconds % 300 == 0) {seconds = 0;}
    clockFlag = 0;
  }
  tempRead = dht.readTemperature();
  lastBStateUp = debounce(buttonUp, lastBStateUp);
  lastBStateDown = debounce(buttonDown, lastBStateDown);
  if (lastBStateUp != lastBStateDown) {
    userTemp = userTempUpdate(lastBStateUp, lastBStateDown, userTemp);
    heatState = (tempRead <= userTemp);
  }
  stopState = stopStateUpdate(tempRead, userTemp, 1, heatState, stopState);
  operationState = operationStateUpdate(stopState, heatState);
  operation(relayHeat, relayCool, operationState);
  display(tempRead, userTemp);
}

ISR(TIMER1_COMPA_vect) {
  TCNT1 = 0;
  clockFlag = 1;
}