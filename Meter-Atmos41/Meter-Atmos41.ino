/* File: Atmos41-02
 *  
 * Hardware: 
 *   Adafruit Feather M0 LoRa915MHz
 *   Meter ATMOS-41 Weather Station.
 *  
 * Description:
 *   This code reads weather data and transmits on LoRaWAN.
 *   Node configuration is in accompanying file LoRa-otaa.h.  That file must 
 *   be updated witht the relevant OTA/ABP node DevEUI, AppEUI & AppKey.
 *   
 * Data Format (bytes):
 * 
 * [  0  ] Code = 03 -> packet format
 * [ 1- 2] Solar (0...1750)
 * [ 3- 4] Precipitation (0.000 ... 125.000)
 * [ 5- 6] Strikes (0 ... 65,535)
 * [ 7- 8] Strike Dist (0 ... 40)
 * [ 9-10] Wind Speed (0.00 ... 40.00)
 * [11-12] Wind Direct (0 ... 359)
 * [13-14] Wind Gust (0.00 ... 40.00)
 * [15-16] Air Temp (-40.0 ... 50.0)
 * [17-18] Vapour Press (0.00 ... 47.00)
 * [19-20] Atmos Press (50.01 ... 110.01)
 * [21-22] Relative Humidity (0.0 ... 100.0)
 * [23-24] Hmidity Sensor Temp (-40.0 ... 50.0)
 * [25-26] x-orientation (tilt) (0.0 ... 180.0)
 * [27-28] y-orientation (tilt) (0.0 ... 180.0)
 * [29-30] compass heading (0 ... 359)
 * [31-32] north wind speed (0.00 ... 40.00)
 * [33-34] east wind speed (0.00 ... 40.00)
 * [35-36] battery (0.00 ... 4.00)
 * 
 * Author:  Allen Benter allen.benter@dpi.nsw.gov.au
 * Date:    29 Jan, 2018
 * 
 * Version:
 *   02 simple read every 10 minutes
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
#define DELAY_TIME 10000 //ms
#define SMP 12 // printed PCB = 12, proto = 11
// LoRaWAN IO1 6

// define variables
bool dataSent = false;

// define time variables
uint8_t seconds = 0;
uint8_t minutes = 36;
uint8_t hours = 8;

uint8_t day = 23;
uint8_t month = 11;
uint8_t year = 18;

uint8_t currAlarm;
uint8_t newAlarm;
bool timeEvent = false;

SDI12 mySDI12(SMP);
RTCZero rtc;

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
            //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            dataSent = true;
            break;
    }
}

void loraInit(){
    os_init();
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

// Uncomment when using AU915
//    for (int i = 0; i < 8; i++) {
//      LMIC_disableChannel(i);
//    }
//    for (int i = 16; i < 64; i++) {
//      LMIC_disableChannel(i);
//    }

   // LMIC_setAdrMode(1);
   // LMIC_setLinkCheckMode(1);
}

void setNextAlarm(){
  uint8_t currMins = rtc.getMinutes();
  
  newAlarm = currMins + 10;
  if(newAlarm > 59){
    newAlarm = 0;
  }
  rtc.setAlarmMinutes(newAlarm);
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
  
  for(num = 0; num < 17; num++){
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
    start = finish;
    
  }

}

void fillArray(uint8_t mydata[], float values[]){
  int16_t temp;
  
  //packet format
  mydata[0] = 0x03;
  //solar
  mydata[1] = (int16_t)values[0] & 0xff;
  mydata[2] = ((int16_t)values[0] >> 8) & 0xff;
  //precip
  temp = values[1] * 100;
  mydata[3] =  temp & 0xff;
  mydata[4] = (temp >> 8) & 0xff;
  //lightning count
  if (values[2] > 32767){
    temp = 32767;
  }else{
    temp = (int16_t)values[2];
  }
  mydata[5] = temp & 0xff;
  mydata[6] = (temp >> 8) & 0xff;
  //lightning distance
  mydata[7] = (int16_t)values[3] & 0xff;
  mydata[8] = ((int16_t)values[3] >> 8) & 0xff;
  //wind speed
  temp = values[4] * 100;
  mydata[9] = temp & 0xff;
  mydata[10] = (temp >> 8) & 0xff;
  //wind direction
  mydata[11] = (int16_t)values[5] & 0xff;
  mydata[12] = ((int16_t)values[5] >> 8) & 0xff;
  //wind gust
  temp = values[6] * 100;
  mydata[13] = temp & 0xff;
  mydata[14] = (temp >> 8) & 0xff;
  //air temp
  temp = values[7] * 10;
  mydata[15] = temp & 0xff;
  mydata[16] = (temp >> 8) & 0xff;
  //vap press
  temp = values[8] * 100;
  mydata[17] = temp & 0xff;
  mydata[18] = (temp >> 8) & 0xff;
  //atmos press
  temp = values[9] * 100;
  mydata[19] = temp & 0xff;
  mydata[20] = (temp >> 8) & 0xff;
  //rel humidity
  temp = values[10] * 1000;
  mydata[21] = temp & 0xff;
  mydata[22] = (temp >> 8) & 0xff;
  //humidity sensor temp
  temp = values[11] * 10;
  mydata[23] = temp & 0xff;
  mydata[24] = (temp >> 8) & 0xff;
  //x-tilt
  temp = values[12] * 10;
  mydata[25] = temp & 0xff;
  mydata[26] = (temp >> 8) & 0xff;
  //y-tilt  
  temp = values[13] * 10;
  mydata[27] = temp & 0xff;
  mydata[28] = (temp >> 8) & 0xff;
  //compass
  mydata[29] = (int16_t)values[14] & 0xff;
  mydata[30] = ((int16_t)values[14] >> 8) & 0xff;
  //nth wind speed
  temp = values[15] * 100;
  mydata[31] = temp & 0xff;
  mydata[32] = (temp >> 8) & 0xff;
  //east wind speed
  temp = values[16] * 100;
  mydata[33] = temp & 0xff;
  mydata[34] = (temp >> 8) & 0xff;
  //battery
  float measuredV = analogRead(VBATT);
  measuredV *= 0.6445;
  mydata[35] = (int16_t)measuredV & 0xff;
  mydata[36] = ((int16_t)measuredV >> 8) & 0xff;
}

void powerUpDevice(){
  Serial.println("wait for device ...");
  uint32_t lastEvent = millis();
  while ((millis() - lastEvent) < DELAY_TIME);
  Serial.println("next event");
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
  rtc.enableAlarm(rtc.MATCH_MMSS);
  rtc.attachInterrupt(rtcTimeAlarm);
    
  mySDI12.begin();

  loraInit();

  timeEvent = true;

}

uint32_t timer = millis(); 
void loop() {
  // put your main code here, to run repeatedly:
  uint8_t mydata[37];
  float values[17];
  String sdiData;
  
  if(timeEvent){
    powerUpDevice();
    doMeasure(1, sdiData);
    getValues(sdiData, values);
    fillArray(mydata, values);

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
