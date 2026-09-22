/***************************************************************************
  This is a library for the BME680 gas, humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BME680 Breakout
  ----> http://www.adafruit.com/products/3660

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#include <time.h>
#include <Adafruit_ST7789.h>
Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(240, 135);

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

enum hvacState{
  Heating, // 0
  Cooling, // 1
  hCount // 2
};

enum menuState{
  TemperatureMenu, //0
  OperationMenu, //1
  UnitMenu, //2
  mCount //3
};

enum tempState {
  C,
  F,
  fCount
};

hvacState opMode = Heating;
menuState menuMode = TemperatureMenu;
tempState tempMode = C;
float targetTemp = 24.0;
volatile long prevChangeTime = 0;
volatile long prevChangeTimetwo = 0;
long debounceTime = 50;
volatile bool changeButtonFlag = false;
volatile bool menuButtonFlag = false;

void IRAM_ATTR buttonToChangeThings() {
  long now = millis();
  if (now> prevChangeTime + debounceTime){
    changeButtonFlag = true;
    prevChangeTime = now;
  }
}

void IRAM_ATTR buttonToChangesMenu() {
  long now = millis();
  if (now > prevChangeTime + debounceTime){
    menuButtonFlag = true;
    prevChangeTimetwo = now;
  }
}

Adafruit_BME680 bme(&Wire); // I2C
//Adafruit_BME680 bme(&Wire1); // example of I2C on another bus
//Adafruit_BME680 bme(BME_CS); // hardware SPI
//Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);

float getCurrentTemp(){
  if(tempMode == tempState::C){
    return bme.temperature;
  }
  if (tempMode == tempState::F){
    return bme.temperature * 9./5.+32;
  }
  return -1100.;
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("BME680 test"));

  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  pinMode(1, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(1), buttonToChangeThings, RISING);

  pinMode(2, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(2), buttonToChangesMenu, RISING);

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_2X);
  /*
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
  */

  display.init(135, 240);
  display.setRotation(3);
  canvas.setTextColor(ST77XX_BLACK);
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, 1);
}

void loop() {
  if (!bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }

  float currentTemp = getCurrentTemp();
  Serial.print("Temperature = ");
  Serial.print(currentTemp);
  if ( tempMode == tempState::F){
    Serial.print(" *F ");
  }else {
    Serial.println(" *C ");
  }
  Serial.print(" with target = ");
  Serial.print(targetTemp);
  Serial.print(" operating in mode = ");
  Serial.print((int)opMode);
  Serial.print(" in menu = ");
  Serial.print(menuMode );

  if(menuButtonFlag) {
    menuButtonFlag = false;
    menuMode = (menuState)(((int)menuMode + 1) % (int)menuState::mCount);
  }

  if (changeButtonFlag){
    if (menuMode == TemperatureMenu && (tempMode == C)){
      targetTemp += 1.0;
      if (targetTemp >30)
       targetTemp = targetTemp -10;
    }else if ((menuMode == TemperatureMenu) && (tempMode == F)){
      targetTemp += 5.0;
      if (targetTemp >100)
       targetTemp = targetTemp -80;
    }

    if (menuMode == OperationMenu){
     changeButtonFlag = false;
     opMode = (hvacState)(((int)opMode +1) % (int)hvacState::hCount);

     canvas.fillScreen(ST77XX_GREEN);
     canvas.setCursor(0, 20);
     if (opMode == Heating){
       canvas.print(" Heating Mode ON ");
     }else if (opMode == Cooling){
       canvas.print(" AC Mode ON ");
     }
     display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);
     delay(50);
    }
    if (menuMode == UnitMenu){
      //change C to F and f to C
      menuButtonFlag = false;
      tempMode = (tempState)(((int)tempMode + 1) % (int)tempState::fCount);

      canvas.fillScreen(ST77XX_GREEN);
      canvas.setCursor(0, 20);
      if (tempMode == F){
        canvas.print(" Temperature is in Faranheit ");
      }else if (tempMode == C){
        canvas.print(" Temperature is in Celsius ");
      }
      display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);
      delay(50);
    }
    changeButtonFlag = false;
  

   Serial.println();
   delay(100);
  
    if ((opMode == Heating) && (menuMode == TemperatureMenu)) {
      if(currentTemp < targetTemp){
        Serial.println("heating is now on!");
        canvas.fillScreen(ST77XX_ORANGE);
        canvas.setCursor(0, 20);
        canvas.print(" Heating is now on! ");
        canvas.print(" target temp = ");
        canvas.print(targetTemp);
        canvas.print(" press D1 to increase target temp ");
        canvas.print(" press D2 to change menu ");
        display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);
        delay(50);
      }
    }else if ((menuMode == TemperatureMenu) && (opMode == Cooling)) {
      if(currentTemp > targetTemp){
        Serial.println("cooling is now on!");
        canvas.fillScreen(ST77XX_BLUE);
        canvas.setCursor(0, 20);
        canvas.print(" Cooling is now on! ");
        canvas.print(" target temp = ");
        canvas.print(targetTemp);
        canvas.print(" press D1 to increase target temp ");
        canvas.print(" press D2 to change menu ");
        display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);
        delay(50);
      }
    }else if (menuMode == TemperatureMenu) {
      Serial.println("Temp Met");
      canvas.fillScreen(ST77XX_GREEN);
      canvas.setCursor(0, 20);
      canvas.print(" Tempurature met ");
      canvas.print(" target temp = ");
      canvas.print(targetTemp);
      canvas.print(" press D1 to increase target temp ");
      canvas.print(" press D2 to change menu ");
      display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);
      delay(50);
    }
  }
}
