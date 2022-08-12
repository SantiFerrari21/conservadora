#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal.h>

const int dhtPin=2;
const int rs=12, en=11, d4=4, d5=5, d6=6, d7=7;
const int relay=3;
const int tempUp=10, tempDown=9;



DHT dht(dhtPin, DHT11);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  
}

void loop() {
  
}