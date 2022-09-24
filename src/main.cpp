#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal.h>

#define noOfButtons 2
#define bounceDelay 20
#define minButtonPress 3

// CONEXIONES NECESARIAS
const int dhtPin=4;                                //Pin de dato del sensor
const int rs=7, en=8, d4=9, d5=10, d6=11, d7=12;   //Pines del display
const int relayCool=5, relayHeat=6;                //Pines de los reles (realyCool = Celda peltier | relayHeat = Resistencia)
const int buttonPins[] = {2,3};                //Botones de config.
// INICIALIZACION

DHT dht(dhtPin, DHT11);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// VARIABLES
/*-------------------Flags-----------------------*/

bool clockFlag = 0;       //Flag de la interrupción.
bool heatState = 1;       //Flag de calefacción.
bool stopState = 1;       //Flag de funcionamiento.
byte operationState = 0;  //Estado de funcionamiento del equipo

/*-------------------Botonera---------------------*/

unsigned long previousMillis[noOfButtons];
byte pressCount[noOfButtons];
byte userTemp = 24; //seteamos un valor inicial de temperatura

unsigned int seconds = 0; 


byte userTempUpdate(byte buttonNumber, byte userValue) {
  //Se asegura que el usuario no pueda ingresar valores superiores a ciertos limites.
  if (buttonNumber == 1 && userValue < 30) {userValue += 1;}
  if (buttonNumber == 0 && userValue > 5) {userValue -= 1;}
  return userValue;
}

void debounce() {
  byte i;
  unsigned long currentMillis = millis();
  for (i = 0; i < noOfButtons; ++i) {
    if (digitalRead(buttonPins[i])) {
      previousMillis[i] = currentMillis;
      pressCount[i] = 0;
    } else {
      if (currentMillis - previousMillis[i] > bounceDelay) {
        previousMillis[i] = currentMillis;
        ++pressCount[i];
        if (pressCount[i] == minButtonPress) {
          userTemp = userTempUpdate(i, userTemp);
        }
      }
    }
  }
}


/// @brief Verifica si el equipo se encuentra en estado detenido.
/// @param readTemp Temperatura leida por el sensor.
/// @param userValue Temperatura seteada por el usuario.
/// @param modifier Modificador del umbral de temperatura.
/// @param heatFlag Variable que define si el equipo enfría o calienta.
/// @param lastStopState Ultimo estado del equipo.
/// @return Estado del funcionamiento.
bool stopStateUpdate(int readTemp, byte userValue, byte modifier, bool heatFlag, bool lastStopState) {
  bool stop = 0;
  //Primero se compara si el equipo estaba detenido.
  if (!lastStopState) {
    //Si no lo esta, revisa si debe detenerse de acuerdo a si esta enfriando o calentando.
    if (heatFlag) {stop = (readTemp > userValue && readTemp >= userValue + modifier);}
    else {stop = (readTemp < userValue && readTemp <= userValue - modifier);}
  }
  //Si lo esta, se compara si el equipo debe seguir detenido o si debe volver a actuar.
  else {
    if (heatFlag) {stop = (readTemp >= userValue);}
    else {stop = (readTemp <= userValue);}
    }

  return stop;
}

/// @brief Actualiza el estado de funcionamiento del equipo.
/// @param stopFlag Indica si el equipo debe detenerse.
/// @param heatFlag Variable que define si el equipo enfría o calienta.
/// @return Estado de funcionamiento.
byte operationStateUpdate(bool stopFlag, bool heatFlag) {
  return stopFlag ? 0 : heatFlag ? 1 : 2; 
}

/// @brief Controla los actuadores del equipo. 
/// @param actuator0 Actuador que se encarga de calentar el dispositivo.
/// @param actuator1 Actuador que se encarga de enfriar el equipo.
/// @param state Estado de funcionamiento del equipo.
void operation(int actuator0, int actuator1, int state) { 
  //Dependiendo el estado del equipo, se activan o desactivan los relés correspondientes. 
  //NOTA: en este caso los reles actuan en estado bajo(LOW)
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

/// @brief Actualiza el diplay del equipo.
/// @param readTemp Temperatura leida por el sensor.
/// @param userValue Temperatura seteada por el usuario.
void displayUpdate(int readTemp, byte userValue) {
  // Primero se borra todo el contenido en el display y luego se escriben los datos correspondientes.
  // Más información de la libreria LiquidCrystal en https://www.arduino.cc/reference/en/libraries/liquidcrystal/
  lcd.clear();
  lcd.print("TempInt: ");
  lcd.print(readTemp);
  lcd.setCursor(0,1);
  lcd.print("TempSet: ");
  lcd.print(userValue);
}

/// @brief Información que se envía al puerto serie.
void serialInfo() {
  Serial.print("operationState: ");
  Serial.println(operationState);
  Serial.print("stopState: ");
  Serial.println(stopState);
  Serial.print("heatState: ");
  Serial.println(heatState);
  Serial.print("userTemp:");
  Serial.println(userTemp);
}

void setup() {
  byte i;
  Serial.begin(9600);
  dht.begin();
  lcd.begin(16,2);
  for (i = 0; i < noOfButtons; ++i) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(relayHeat, OUTPUT);
  pinMode(relayCool, OUTPUT);
  //Activación de interrupción por timer
  //Para esto modificamos registros del micro
  //Más información en https://electronoobs.com/eng_arduino_tut140.php
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
  //Se lee la temperatura del sensor.
  int tempRead = dht.readTemperature();
  debounce();
  heatState = (tempRead <= userTemp);

  //Se ejecutan las funciones pertinentes
  stopState = stopStateUpdate(tempRead, userTemp, 1, heatState, stopState);
  operationState = operationStateUpdate(stopState, heatState);
  operation(relayHeat, relayCool, operationState);
  displayUpdate(tempRead, userTemp);

  // Se verifica que la interrupción halla ocurrido
  if (clockFlag) {
    // Contamos los segundos, transcurrido 60segundos se envia información al puerto serie
    seconds++;
    if (seconds % 10 == 0) {serialInfo();}
    // TODO: Se puede implementar más funciones para calcular el promedio de mediciones y otras cosas.
    if (seconds % 300 == 0) {seconds = 0;}
    clockFlag = 0;
  }
}

//Función de interrupción
ISR(TIMER1_COMPA_vect) {
  TCNT1 = 0;
  clockFlag = 1;
}