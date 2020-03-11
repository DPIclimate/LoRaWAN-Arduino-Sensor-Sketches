/* File: OAI-testNode-ABP
 *  
 * Hardware: 
 *   Adafruit Feather M0 LoRa915MHz
 *  
 * Description:
 *   This code transmits tests data transmission on LoRaWAN using AS923.
 *   Node configuration is in accompanying file LoRa-xxxx.h.  That file must 
 *   be updated witht the relevant ABP/OTAA node DevEUI etc.
 * 
 *   The code also monitors the battery voltage.
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

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "LoRa-otaa.h"
#include <Wire.h>
#include "Adafruit_ADT7410.h"
#include <CayenneLPP.h>

// Pin definitions
#define VBATT A7
// LoRaWAN IO1 6

// define variables
bool dataSent = false;
bool ADTavail = false;
CayenneLPP lpp(51);

// Create the ADT7410 temperature sensor object
Adafruit_ADT7410 tempsensor = Adafruit_ADT7410();

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println("EV_SCAN_TIMEOUT");
            break;
        case EV_BEACON_FOUND:
            Serial.println("EV_BEACON_FOUND");
            break;
        case EV_BEACON_MISSED:
            Serial.println("EV_BEACON_MISSED");
            break;
        case EV_BEACON_TRACKED:
            Serial.println("EV_BEACON_TRACKED");
            break;
        case EV_JOINING:
            Serial.println("EV_JOINING");
            break;
        case EV_JOINED:
            Serial.println("EV_JOINED");

            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
            break;
        case EV_RFU1:
            Serial.println("EV_RFU1");
            break;
        case EV_JOIN_FAILED:
            Serial.println("EV_JOIN_FAILED");
            break;
        case EV_REJOIN_FAILED:
            Serial.println("EV_REJOIN_FAILED");
            break;
            break;
        case EV_TXCOMPLETE:
            Serial.println("EV_TXCOMPLETE (includes waiting for RX windows)");
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println("Received ack");
            if (LMIC.dataLen) {
              Serial.println("Received ");
              Serial.println(LMIC.dataLen);
              Serial.println(" bytes of payload");
              for (int i = 0; i < LMIC.dataLen; i++) {
                if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
                  Serial.print("0");
                }
                Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
              }
              Serial.println();
            }
            // Schedule next transmission
            //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            dataSent = true;
            break;
        case EV_LOST_TSYNC:
            Serial.println("EV_LOST_TSYNC");
            break;
        case EV_RESET:
            Serial.println("EV_RESET");
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println("EV_RXCOMPLETE");
            break;
        case EV_LINK_DEAD:
            Serial.println("EV_LINK_DEAD");
            break;
        case EV_LINK_ALIVE:
            Serial.println("EV_LINK_ALIVE");
            break;
         default:
            Serial.println("Unknown event");
            break;
    }
}

void loraInit()
{
    os_init();
    LMIC_reset();
    //LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
    //LMIC_setAdrMode(1);
    //LMIC_setLinkCheckMode(1);
}

uint8_t getBattVolts()
{
  float measuredV = analogRead(VBATT);
  measuredV *= 0.6445;
  measuredV -= 127;
  return (int) measuredV;
}

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(9600);
  delay(5000);
  Serial.println("starting ...");

  loraInit();

  if (tempsensor.begin())
  {
    ADTavail = true;
    delay(250);
  }

}

uint32_t timer = millis();

void loop() {
  // put your main code here, to run repeatedly
  uint8_t mydata[3];
  float c;
  lpp.reset();

  if(ADTavail)
  {
    c = tempsensor.readTempC();
  }
  else
  {
    c = -999;
  }
  lpp.addTemperature(1,c);
  lpp.addAnalogInput(2,analogRead(VBATT));
  
  dataSent = false;
  LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);
  
  while(!dataSent)
  {
    os_runloop_once();
    delay(1);
    Serial.println(LMIC.freq);
  }

  Serial.println("data sent");
  Serial.println();
  while (millis() - timer < 120000 );
  timer = millis();

}
