#include <Arduino.h>
#include "Wire.h"
#define address 0x40

//Temperature & Humidity Sensor setup
uint8_t readReg(uint8_t reg, const void* pBuf, size_t size);
uint8_t buf[4] = {0};

//Soil Moisture Sensor setup
#define SOIL_MOIST_PIN A0
const int DRY_VAL = 3650;
const int WET_VAL = 0;
int intervals = (DRY_VAL - WET_VAL) / 3;

//Light Sensor setup
#define LIGHT_PIN A4

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Wire.begin();
}

void loop()
{
  //Temperature & Humidity Sensor
  readReg(0x00, buf, 4);
  uint16_t data = buf[0] << 8 | buf[1];
  uint16_t data1 = buf[2] << 8 | buf[3];
  float temp = (((float)data * 165 / 65535.0) - 40.0);
  float hum = ((float)data1 / 65535.0) * 100;
  Serial.print("temp(C):");
  Serial.print(temp);
  Serial.print("\t");
  Serial.print("hum(%RH):");
  Serial.println(hum);
  
  //Soil Moisture Sensor 
  int soilMoistureValue = analogRead(SOIL_MOIST_PIN);
  if(soilMoistureValue > WET_VAL && soilMoistureValue < (WET_VAL + intervals))
  {
    Serial.println("Very Wet");
  }
  else if(soilMoistureValue > (WET_VAL + intervals) && soilMoistureValue < (DRY_VAL - intervals))
  {
    Serial.println("Wet");
  }
  else if(soilMoistureValue < DRY_VAL && soilMoistureValue > (DRY_VAL - intervals))
  {
    Serial.println("Dry");
  }

  //Light Sensor
  int val = analogRead(LIGHT_PIN);
  Serial.println(val,DEC);

  delay(500);
}

uint8_t readReg(uint8_t reg, const void* pBuf, size_t size)
{
  if (pBuf == NULL) {
    Serial.println("pBuf ERROR!! : null pointer");
  }
  uint8_t * _pBuf = (uint8_t *)pBuf;
  Wire.beginTransmission(address);
  Wire.write(&reg, 1);
  if ( Wire.endTransmission() != 0) {
    return 0;
  }
  delay(20);
  Wire.requestFrom(address, (uint8_t) size);
  for (uint16_t i = 0; i < size; i++) {
    _pBuf[i] = Wire.read();
  }
  return size;
}