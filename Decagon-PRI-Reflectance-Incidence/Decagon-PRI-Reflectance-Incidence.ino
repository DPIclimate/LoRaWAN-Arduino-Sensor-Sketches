/* File: Decagon-PRI-Reflectance-Incidence
 *  
 * Hardware: 
 *   Adafruit Feather M0 LoRa915MHz
 *   Decagon PRI Reflectance & Incidence Sensor
 *  
 * Description:
 *   This code transmits PRI sensor data over LoRaWAN.
 *   Node configuration is in accompanying file LoRa-otaa.h.  That file must 
 *   be updated witht the relevant otaa node DevEUI etc. See README.md for more details.
 * 
 *   PRI-Hemi+Stop -> normal data sent comprises 17 bytes:
 *   [0....3] float: incidence@531nm 
 *   [4....7] float: incidence@570nm
 *   [8] direction of sensor
 *   [9...12] float: reflected@531nm
 *   [13..16] float: reflected@570nm
 *   [17] direction of sensor
 *   [18] battery voltage
 *   
 *    PRI-Stop -> normal data sent comprises 9 bytes:
 *   [0....3] float: reflected@531nm 
 *   [4....7] float: reflected@570nm
 *   [8] direction of sensor
 *   [9] battery voltage
 * 
 *   The code also monitors the battery voltage.
 *  
 *   The interval for reading the data from the sensor is set at 1 hour
 *   
 *   calculating the PRI index:
 *   Ra = reflected 531, Ia = incidence 531 (from Hemi sensor)
 *   Rb = reflected 570, Ib = incidence 570
 *   
 *   PRI = (Ra/Ia - Rb/Ib) / (Ra/Ia + Rb/Ib)
 *   
 * 
 * Author:  Allen Benter allen.benter@dpi.nsw.gov.au
 * Date:    11 September, 2017
 * 
 * Version:
 *   0.6.x inherited from SMP sensor code and initial release
 *   0.7.0 single STOP sensor only.  Array is bytes [0..9]+voltage
 *   
 * License:
 *   GPLv3 - see attached file gpl-3.0.txt
 *  
 */

#include <RTCZero.h>
#include <SDI12.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <math.h>
#include <CayenneLPP.h>
#include "LoRa-otaa.h"

// Pin definitions
#define VBATT A7
#define SDI 12
// LoRaWAN IO1 6

// define time variables
uint8_t seconds = 00;
uint8_t minutes = 34;
uint8_t hours = 12;
uint8_t day = 14;
uint8_t month = 02;
uint8_t year = 19;
uint8_t currAlarm = hours;
uint8_t newAlarm;
bool timeEvent = false;

SDI12 mySDI12(SDI);
RTCZero rtc;

// define variables
bool dataSent = false;

union {
  float value;
  unsigned char bytes[4];
} pri;

struct PRI_DATA {
  String sdi12data;
  float firstValue;
  float secondValue;
  int direction;
};

void onEvent (ev_t ev) {
    switch(ev) {
        case EV_TXCOMPLETE:
            // Schedule next transmission
            dataSent = true;
            break;
    }
}

void loraInit(){
    os_init();
    LMIC_reset();

    // Disable link check validation
    LMIC_setAdrMode(1);
    LMIC_setLinkCheckMode(1);
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

}

void setNextAlarm(){
  newAlarm = rtc.getHours() + 1;
  if(newAlarm > 18)
    newAlarm = 6;

  rtc.setAlarmTime(newAlarm, 0, 0);
}

uint8_t getBattVolts(){
  float measuredV = analogRead(VBATT);
  measuredV *= 0.6445;
  measuredV -= 127;
  return (int) measuredV;
}

// initiate measure
void doMeasure(uint8_t device){
  //Serial.println("do measure");
  String command = "";
  command += String(device);
  command += "M!";
  mySDI12.sendCommand(command);
  delay(30);
  while(mySDI12.available()){
    char c = mySDI12.read();
    if ((c != '\n') || (c != '\r')){
      delay(5);
    }
  }
  mySDI12.flush();
  delay(1000);
}

// read data returned by sensor
void getData(uint8_t device, String &dest){
  doMeasure(device);
  String command = "";
  String sdiData = "";
  command += String(device);
  command += "D0!";
  mySDI12.sendCommand(command);
  delay(500);
  while(mySDI12.available()){
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')){
      sdiData += c;
      delay(5);
    }
  }
  dest = sdiData;
  mySDI12.flush();
  delay(2000);
}

// extract values from data string
void readValues(struct PRI_DATA &pridata){
  String temp = pridata.sdi12data;
  float Ia, Ib;
  int dirA;
  
  //find sign positions
  int lastPlus = temp.lastIndexOf('+');
  int firstPlus = temp.indexOf('+');
  int secondPlus = temp.indexOf('+', firstPlus+1);

  //get floats
  Ia = temp.substring(firstPlus + 1, secondPlus).toFloat();
  Ib = temp.substring(secondPlus + 1, lastPlus).toFloat();
  dirA = temp.substring(lastPlus + 1).toInt();

  pridata.firstValue = Ia;
  pridata.secondValue = Ib;
  pridata.direction = dirA;
}

void setup() {
  // put your setup code here, to run once:
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  rtc.begin();
  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);

  setNextAlarm();
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  rtc.attachInterrupt(rtcTimeAlarm);

  mySDI12.begin();

  loraInit();

  timeEvent = true;

}

void loop() {
  // put your main code here, to run repeatedly
  uint8_t mydata[10];
  PRI_DATA incidence;
  PRI_DATA reflected;

  if (timeEvent){
    //get STOP data
    getData(2, incidence.sdi12data);
    readValues(incidence);
    pri.value = incidence.firstValue;
    mydata[0] = pri.bytes[0];
    mydata[1] = pri.bytes[1];
    mydata[2] = pri.bytes[2];
    mydata[3] = pri.bytes[3];
    pri.value = incidence.secondValue;
    mydata[4] = pri.bytes[0];
    mydata[5] = pri.bytes[1];
    mydata[6] = pri.bytes[2];
    mydata[7] = pri.bytes[3];
    mydata[8] = incidence.direction;
    
    mydata[9] = getBattVolts();

    dataSent = false;
    LMIC_setTxData2(1, mydata, sizeof(mydata), 1);
    while(!dataSent){
      os_runloop_once();
      delay(1);
    }

    setNextAlarm();

    timeEvent = false;
  }

  rtc.standbyMode();
}

void rtcTimeAlarm(){
  timeEvent = true;
}
