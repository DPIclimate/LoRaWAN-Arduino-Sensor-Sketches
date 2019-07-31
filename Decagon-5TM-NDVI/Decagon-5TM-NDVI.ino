/* File: Decagon-5TM-NDVI
 *  
 * Hardware: 
 *   Adafruit Feather M0 LoRa915MHz
 *   Decagon 5TM Soil Mositure Sensor
 *   Decagon NDVI Sensor
 *  
 * Description:
 *   This code transmits PRI sensor data over LoRaWAN.
 *   Node configuration is in accompanying file LoRa-otaa.h.  That file must 
 *   be updated witht the relevant otaa node DevEUI etc. See README.md for more details.
 * 
 *   5TM-Soil Moisture + PRI-Reflectance -> normal data sent comprises 18 bytes:
 *   
 *   [0....3] floatLE: Soil Temp
 *   [4....7] floatLE: Soil Moisture
 *   [8....11] floatLE: reflected@531nm
 *   [12...15] float: reflected@570nm
 *   [16] direction of NDVI sensor
 *   [17] battery voltage
 * 
 *   The code also monitors the battery voltage.
 *  
 *   The interval for reading the data from the sensor is set at 1 hour
 * 
 * Author:  Allen Benter allen.benter@dpi.nsw.gov.au
 * Date:    11 September, 2017
 * 
 * Version:
 *   0.6.x inherited from SMP sensor code and initial release
 *   0.7.0 Single NDVI reflectance and 5TM Soil Sensor
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
#include "LoRa-otaa.h"

// Pin definitions
#define VBATT A7
#define SMP 12
// LoRaWAN IO1 6 - Pins soldered together with wire for continued transmission beyond the first uplink

// define time variables
uint8_t seconds = 0;
uint8_t minutes = 47  ;
uint8_t hours = 14;

uint8_t day = 25;
uint8_t month = 01;
uint8_t year = 19;
uint8_t currAlarm = hours;
uint8_t newAlarm;
bool timeEvent = false;

// define variables
bool dataSent = false;

SDI12 mySDI12(SMP);
RTCZero rtc;

union {
  float value;
  unsigned char bytes[4];
} smp;

struct SMP_DATA {
  String sdi12data;
  float temperature;
  float moisture;
};

union {
  float value;
  unsigned char bytes[4];
} ndvi;

struct NDVI_DATA {
  String sdi12data;
  float firstValue;
  float secondValue;
  int direction;
};

void onEvent (ev_t ev) {
//    Serial.print(os_getTime());
//    Serial.print(": ");
    switch(ev){
        case EV_SCAN_TIMEOUT:
//            Serial.println("EV_SCAN_TIMEOUT");
            break;
        case EV_BEACON_FOUND:
//            Serial.println("EV_BEACON_FOUND");
            break;
        case EV_BEACON_MISSED:
//            Serial.println("EV_BEACON_MISSED");
            break;
        case EV_BEACON_TRACKED:
//            Serial.println("EV_BEACON_TRACKED");
            break;
        case EV_JOINING:
        
//            Serial.println("EV_JOINING");
//            Serial.println();
//            Serial.print("Time event value is: ");
//            Serial.print(timeEvent);
//            Serial.println();

            break;
        case EV_JOINED:
//            Serial.println("EV_JOINED");

            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
            break;
        case EV_RFU1:
//            Serial.println("EV_RFU1");
            break;
        case EV_JOIN_FAILED:
//            Serial.println("EV_JOIN_FAILED");
            break;
        case EV_REJOIN_FAILED:
//            Serial.println("EV_REJOIN_FAILED");
            break;
            break;
        case EV_TXCOMPLETE:
            dataSent = true;
            break;
        case EV_LOST_TSYNC:
//            Serial.println("EV_LOST_TSYNC");
            break;
        case EV_RESET:
//            Serial.println("EV_RESET");
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
//            Serial.println("EV_RXCOMPLETE");
            break;
        case EV_LINK_DEAD:
//            Serial.println("EV_LINK_DEAD");
            break;
        case EV_LINK_ALIVE:
//            Serial.println("EV_LINK_ALIVE");
            break;
         default:
//            Serial.println("Unknown event");
            break;
    }
}

void loraInit(){
    os_init();
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

    // Disable link check validation
    LMIC_setAdrMode(1);
}

void setNextAlarm(){
  newAlarm = rtc.getHours() + 1;
    if(newAlarm > 23){
      newAlarm = 0;
    }
  rtc.setAlarmTime(newAlarm, 0, 0);
//  Serial.println("New alarm set to: ");
//  Serial.print(newAlarm);
//  Serial.println();
}

uint8_t getBattVolts(){
  float measuredV = analogRead(VBATT);
  measuredV *= 0.6445;
//  Serial.println();
//  Serial.print("Battery Voltage: ");
//  Serial.print(measuredV/100);
//  Serial.println();
  measuredV -= 127;
  return (int) measuredV;
}

// initiate measure
void doMeasure(uint8_t device){
//  Serial.println("do measure");
  String command = "";
  command += String(device);
  command += "M!";
  mySDI12.sendCommand(command);
  delay(30);
  
  while(mySDI12.available()){
    char c = mySDI12.read();
//    Serial.println(c);  
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
//  Serial.println();
//  Serial.print("SDI12 Command: ");
//  Serial.print(command);
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

// Soil Moisture Temp Probe
void readValuesSMP(struct SMP_DATA &smpdata){
  String temp = smpdata.sdi12data;
  float Ia, Ib;
  int dirA;
  
  //find sign positions
  int lastPlus = temp.lastIndexOf('+');
  int firstPlus = temp.indexOf('+');
  if(firstPlus == lastPlus){
    // the first value is negative
    firstPlus = temp.indexOf('-');
  }

  //get floats
  Ia = temp.substring(firstPlus + 1, lastPlus).toFloat();
  Ib = temp.substring(lastPlus + 1).toFloat();

  smpdata.moisture = Ia;
  smpdata.temperature = Ib;
}

// NDVI reflectance sensor
void readValuesNDVI(struct NDVI_DATA &ndvidata){
  String temp = ndvidata.sdi12data;
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

  ndvidata.firstValue = Ia;
  ndvidata.secondValue = Ib;
  ndvidata.direction = dirA;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  rtc.begin();
  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);
  delay(200);
  setNextAlarm();
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  rtc.attachInterrupt(rtcTimeAlarm);
  
  mySDI12.begin();
  delay(7000);
  loraInit();
  
  timeEvent = true;
}

void loop() {
  uint8_t mydata[18];
  SMP_DATA smpdata;
  NDVI_DATA reflected;
  
    if (timeEvent){
    //get SMP data
    getData(0, smpdata.sdi12data);
    readValuesSMP(smpdata);
    smp.value = smpdata.temperature;
    mydata[0] = smp.bytes[0];
    mydata[1] = smp.bytes[1];
    mydata[2] = smp.bytes[2];
    mydata[3] = smp.bytes[3];
    smp.value = smpdata.moisture;
    mydata[4] = smp.bytes[0];
    mydata[5] = smp.bytes[1];
    mydata[6] = smp.bytes[2];
    mydata[7] = smp.bytes[3];

    getData(1, reflected.sdi12data);
    readValuesNDVI(reflected);
    ndvi.value = reflected.firstValue;

    mydata[8] = ndvi.bytes[0];
    mydata[9] = ndvi.bytes[1];
    mydata[10] = ndvi.bytes[2];
    mydata[11] = ndvi.bytes[3];
    ndvi.value = reflected.secondValue;
    mydata[12] = ndvi.bytes[0];
    mydata[13] = ndvi.bytes[1];
    mydata[14] = ndvi.bytes[2];
    mydata[15] = ndvi.bytes[3];
    mydata[16] = reflected.direction;
    
    mydata[17] = getBattVolts();
    
    dataSent = false;

//    Serial.println("SENDING DATA!");
    LMIC_setTxData2(1, mydata, sizeof(mydata), 0);

    while(!dataSent){
      os_runloop_once();
//      Serial.println(LMIC.freq);
      delay(1);
    }

//    Serial.println("Data sent.");
    
    setNextAlarm();
    timeEvent = false;
    
  }
  rtc.standbyMode();
}

void rtcTimeAlarm(){
  timeEvent = true;
}
