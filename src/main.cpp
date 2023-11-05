#include <Arduino.h>
#include <painlessMesh.h>
#include "Wire.h"
#define address 0x40

#define LEDPIN 2

#define BLINK_PERIOD 3000
#define BLINK_DURATION 100

#define MESH_SSID "WhateverYouLike"
#define MESH_PASSWORD "SomethingSneaky"
#define MESH_PORT 5555


#define MSG_DELAY_SEC 1

uint32_t baseStationID =0;

void receivedCallback(uint32_t from, String msg);
void newConnectionCallback(uint32_t nodeID);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);
uint32_t parseSimpleJson(const char* jsonstring);

void sendMessage();
Task taskSendMessage(TASK_SECOND *MSG_DELAY_SEC, TASK_FOREVER, &sendMessage);

Scheduler userSched;
painlessMesh mesh;
bool calc_delay = false;
SimpleList<uint32_t> nodes;

const size_t bufferSize = 1024;
Task blinkNoNodes;
bool onFlag = false;

//Temperature & Humidity Sensor setup
uint8_t readReg(uint8_t reg, const void* pBuf, size_t size);
uint8_t buf[4] = {0};
float temp;
float hum;

//Soil Moisture Sensor setup
#define SOIL_MOIST_PIN A0
const int DRY_VAL = 3650;
const int WET_VAL = 0;
int intervals = (DRY_VAL - WET_VAL) / 3;
int soilMoistureValue;

//Light Sensor setup
#define LIGHT_PIN A1
int val;



void setup()
{
  Serial.begin(9600);
  pinMode(LEDPIN, OUTPUT);

  mesh.setDebugMsgTypes(ERROR | DEBUG);
  mesh.init(MESH_SSID, MESH_PASSWORD, &userSched, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);


  userSched.addTask(taskSendMessage);
  taskSendMessage.enable();

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []()
                   {
    onFlag = !onFlag;
    blinkNoNodes.delay(BLINK_DURATION);

    if(blinkNoNodes.isLastIteration()){
      blinkNoNodes.setIterations((mesh.getNodeList().size()+1)*2);
      blinkNoNodes.enableDelayed(BLINK_PERIOD -
      (mesh.getNodeTime() % (BLINK_PERIOD*1000)) / 1000);
    } });
    userSched.addTask(blinkNoNodes);
    blinkNoNodes.enable();

    randomSeed(analogRead(A0));

     Wire.begin();
}

void loop()
{
  mesh.update();
  digitalWrite(LEDPIN, !onFlag);

  //Temperature & Humidity Sensor
  readReg(0x00, buf, 4);
  uint16_t data = buf[0] << 8 | buf[1];
  uint16_t data1 = buf[2] << 8 | buf[3];
  temp = (((float)data * 165 / 65535.0) - 40.0);
  hum = ((float)data1 / 65535.0) * 100;

//Soil Moisture Sensor
  //int soilMoistureValue = analogRead(SOIL_MOIST_PIN);

  //Light Sensor
  val = analogRead(LIGHT_PIN);
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

void sendMessage(){
  String msg = "{\"id\": 6,\"temp\":" + String(temp) + ",\"humidity\":" + String(hum) + ",\"lightS\":" + String(val) +"}";
  Serial.println(msg);
  if(baseStationID==0){
    Serial.println("Base Station no ID");
  }
  else{
    mesh.sendSingle(baseStationID, msg);
    Serial.println("message sent");
  }
  if(calc_delay){
    auto node = nodes.begin();
    while (node != nodes.begin()){
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay=false;
  }
  //Serial.println("Sending msg: " + msg);
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

void receivedCallback(uint32_t from, String msg){
  Serial.printf("Start here: Rec'd from %u, %s\n", from, msg.c_str());
  try
  {
    baseStationID = parseSimpleJson(msg.c_str());
  }
  catch(const std::exception& e)
  {
    Serial.printf("ERROR in json parse");
  }

}

void newConnectionCallback(uint32_t nodeID){
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() +1) *2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000)) / 1000);

  Serial.printf("--> start Here: New Connection, nodeId = %u\n", nodeID);
  Serial.printf("--> start Here: New Connection, %s\n", mesh.subConnectionJson(true));
}

void changedConnectionCallback(){
  Serial.println("Connection Changed");
  calc_delay=true;
}

void nodeTimeAdjustedCallback(int32_t offset){
  Serial.printf("adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay){
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}

uint32_t parseSimpleJson(const char* jsonString)
{
//create the parsable json object
StaticJsonDocument<bufferSize> jsonDoc;
DeserializationError error = deserializeJson(jsonDoc, jsonString);


// catch the errors if there are any
if (error) {
Serial.print("Failed to parse JSON: ");
Serial.println(error.c_str());
return 0;
}


uint32_t baseID = jsonDoc["basestation"];
return baseID;

// Output the json data
Serial.print("basestation: ");
Serial.println(baseID);
}
