/* File: OAI-5TM-0.6.2
 *  
 * Hardware: 
 *   Adafruit Feather M0 LoRa915MHz
 *   Decagon 5TM Soil Moisture Probe.
 *  
 * Description:
 *   This code reads soil moisture and temperature and transmits on LoRaWAN.
 *   Node configuration is in accompanying file LoRa-Cisco.h.  That file must 
 *   be updated witht the relevant ABP node DevEUI etc.
 * 
 * Author:  Allen Benter allen.benter@dpi.nsw.gov.au
 * Date:    29th June, 2017
 * 
 * Version:
 *   0.2 averaging 6 reads, sending every 6 intervals (abt 12 hours)
 *   0.4 averaging 1 read, sending every interval 
 *   0.6 removed averaging, sending per TPL5110
 *   0.6.2 decimal temperature & moisture readings
 *         single initiate/read - auto find deata elements and time
 *   
 * License:
 *   GPLv3 - see attached file gpl-3.0.txt
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
#define SMP 12 // printed PCB = 12, proto = 11
// LoRaWAN IO1 6

// define variables
bool dataSent = false;

// define time variables
uint8_t seconds = 0;
uint8_t minutes = 05;
uint8_t hours = 13;

uint8_t day = 11;
uint8_t month = 3;
uint8_t year = 19;

uint8_t currAlarm;
uint8_t newAlarm;
bool timeEvent = false;

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

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
            timeEvent = true;
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            dataSent = true;
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}

void loraInit(){
    os_init();
    LMIC_reset();
}

uint8_t getBattVolts(){
  float measuredV = analogRead(VBATT);
  measuredV *= 0.6445;
  measuredV -= 127;
  return (int) measuredV;
}

void setNextAlarm(){
  uint8_t currHour = rtc.getHours();
  
  newAlarm = currHour + 1;
  if(newAlarm > 23){
    newAlarm = 0;
  }
  rtc.setAlarmTime(newAlarm, 0, 0);
}

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

void readValues(struct SMP_DATA &smpdata){
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


void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  rtc.begin();
  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);

  minutes = 0;
  seconds = 0;

  setNextAlarm();
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  rtc.attachInterrupt(rtcTimeAlarm);
  
  mySDI12.begin();
  loraInit();
  timeEvent = true;

}

uint32_t timer = millis(); 
void loop(){
  // put your main code here, to run repeatedly:
  uint8_t mydata[9];
  SMP_DATA smpdata;
  
  if(timeEvent){
    getData(0, smpdata.sdi12data);
    readValues(smpdata);
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
    mydata[8] = getBattVolts();
    
    dataSent = false;
    LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
    
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
