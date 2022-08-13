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
int tempSetMod;
bool heatState = 1;
int lastButtonState = LOW;
const int debounceDelay = 50;

// INICIALIZACION
DHT dht(dhtPin, DHT11);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Funciones
//Funcion debounce adaptada de https://www.arduino.cc/en/Tutorial/BuiltInExamples/Debounce
int debounce (int reading) {
  unsigned long lastDebounceTime;
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == HIGH) {
      lastButtonState = reading;
      return HIGH;
    }
    else {
      lastButtonState = reading;
      return LOW;
    }
  }
}


void setup() {                  //TODO: comunicación puerto serie.
  dht.begin();
  lcd.begin(16, 2);
  pinMode(buttonUp, INPUT);
  pinMode(buttonDown, INPUT);
  pinMode(relayCool, OUTPUT);
  pinMode(relayHeat, OUTPUT);
  lcd.print('TempInt: ');
  lcd.setCursor(0,1);
  lcd.print('TempSet: ');
  digitalWrite(relayCool, LOW);
  digitalWrite(relayHeat, LOW);
}

//TODO: Crear funciones para simplificar el codigo del loop

void loop() {

  tempInt = dht.readTemperature();
  lcd.setCursor(10, 0);
  lcd.print(tempInt);

  int tempUp = debounce(digitalRead(buttonUp));
  int tempDown = debounce(digitalRead(buttonDown));
  if (tempUp == HIGH) {
    tempSet++;
    if (tempSet >= tempInt) { heatState = 1; } 
    if (tempSet < tempInt) { heatState = 0; }
  }
  if (tempDown == HIGH) { 
    tempSet--;
    if (tempSet >= tempInt) { heatState = 1; } 
    if (tempSet < tempInt) { heatState = 0; } 
  } 
  lcd.setCursor(10, 1);
  lcd.print(tempSet);

  switch (heatState) // se elige si hay que calentar/enfriar
  {
  case 0: //enfría por tanto se pone el umbral por debajo de lo seteado
    tempSetMod = tempSet - 2;
    break;
  case 1: //calienta por tanto se pone el umbral por arriba de lo seteado
    tempSetMod = tempSet + 2;
    break;
  default:
    break;
  }

  //control de los reles
  if (tempSetMod > tempInt) {       //caso 1: la temp de umbral es mayor y se activa el rele con la resistencia
    digitalWrite(relayCool, LOW);    
    digitalWrite(relayHeat, HIGH);
  }
  else if (tempSetMod < tempInt) {  //caso 2: la temp de umbral es menor y se activa el rele con la celda
    digitalWrite(relayCool, HIGH);
    digitalWrite(relayHeat, LOW);
  }
  else {                            //caso 3: la temp de umbral se consiguio y se desactivan ambos reles
    digitalWrite(relayCool, LOW);
    digitalWrite(relayHeat, LOW);
    while (tempSet > tempInt || tempSet < tempInt) {    //se inicia un bucle hasta que la que se pierda la dif de la temp de umbral
      tempInt = dht.readTemperature();                  //se sigue mostrando la temp int
      lcd.setCursor(10, 0);                             //TODO: romper ciclo cuando se pulsa un boton
      lcd.print(tempInt);
      delay(6000);
    }
  }
  delay(2000);
}

