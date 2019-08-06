/*  
 *  File: Meter-Teros21
 *  Meter Teros 21 Soil Water Potential Sensor
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
uint8_t minutes = 12;
uint8_t hours = 11;

uint8_t day = 16;
uint8_t month = 01;
uint8_t year = 19;

uint8_t currAlarm;
uint8_t newAlarm;
bool timeEvent = false;

SDI12 mySDI12(SMP);
RTCZero rtc;

union {
  float value;
  unsigned char bytes[4];
} flData;

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_JOINED:
            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
            break;
        case EV_TXCOMPLETE:
            dataSent = true;
            break;
    }
}

void loraInit(){
    os_init();
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
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
  rtc.setAlarmHours(newAlarm);
}

void doMeasure(uint8_t addr, String &dest){
  String sdiInfo = "";
  Serial.println("do measure");
  String command = "";
  command += String(addr);
  command += "R0!";
  sdiInfo="";
  mySDI12.sendCommand(command);
  delay(30);
  while(mySDI12.available()){
    char c = mySDI12.read();
    sdiInfo += c;
    delay(5);
  }
  //Serial.println(sdiInfo);
  dest = sdiInfo;
  mySDI12.flush();
  Serial.println(dest);
}

void getValues(String &response, float values[]){
  boolean sign = true;
  uint8_t start = 0;
  uint8_t finish = 0;
  uint8_t plus, minus;
  String temp;
  int num = 0;
  
  for(num = 0; num < 2; num++){
    if(start < 1){
      plus = response.indexOf('+');
      minus = response.indexOf('-');

      if(plus < minus){
        start = plus;
      }else{
        start = minus;
      }
    }

    //get next sign
    plus = response.indexOf('+', start + 1);
    minus = response.indexOf('-', start + 1);

    if(plus < minus){
      finish = plus;
    }else{
      finish = minus;
    }
    // read value
    values[num] = response.substring(start + 1, finish).toFloat();
    if(response.substring(start, start + 1).equals("-"))
      values[num] *= -1;
    //Serial.println(values[num]);
    start = finish;
    
  }
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
void loop() {
  // put your main code here, to run repeatedly:
  uint8_t mydata[9];
  float values[2];
  String sdiData;
  
  if(timeEvent){
    doMeasure(1, sdiData);
    getValues(sdiData, values);
    flData.value = values[0];
    mydata[0] = flData.bytes[0];
    mydata[1] = flData.bytes[1];
    mydata[2] = flData.bytes[2];
    mydata[3] = flData.bytes[3];

    flData.value = values[1];
    mydata[4] = flData.bytes[0];
    mydata[5] = flData.bytes[1];
    mydata[6] = flData.bytes[2];
    mydata[7] = flData.bytes[3];

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
