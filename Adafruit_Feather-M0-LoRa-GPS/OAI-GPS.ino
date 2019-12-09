#include <Adafruit_GPS.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "LoRa-otaa.h"

#define GPSSERIAL Serial1
Adafruit_GPS GPS(&GPSSERIAL);
#define GPSECHO false

uint32_t timer = millis();   

uint8_t mydata[12];
bool dataSent = false;

union {
  float a;
  unsigned char bytes[4];
} coord;

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
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
    LMIC_setAdrMode(1);
    LMIC_setLinkCheckMode(1);
    Serial.println("lora setup");

//    for (int i = 0; i < 8; i++) {
//      LMIC_disableChannel(i);
//    }
//    for (int i = 16; i < 64; i++) {
//      LMIC_disableChannel(i);
//    }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("starting ...");
  loraInit();

  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  
}

  
void loop() {

  char c = GPS.read();

  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA()))
      return;
  }
  
  if (millis() - timer > 120000)
  {
    timer = millis();
    if (GPS.fix)
    {
      coord.a = GPS.latitude;
      mydata[0] = coord.bytes[0];
      mydata[1] = coord.bytes[1];
      mydata[2] = coord.bytes[2];
      mydata[3] = coord.bytes[3];
  
      coord.a = GPS.longitude;
      mydata[4] = coord.bytes[0];
      mydata[5] = coord.bytes[1];
      mydata[6] = coord.bytes[2];
      mydata[7] = coord.bytes[3];
      
      coord.a = GPS.altitude;
      mydata[8] = coord.bytes[0];
      mydata[9] = coord.bytes[1];
      mydata[10] = coord.bytes[2];
      mydata[11] = coord.bytes[3];
      
      dataSent = false;
      LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
      Serial.println("sending ...");
      while(!dataSent)
      {
        os_runloop_once();
        delay(1);
      }
    }
    
  }
  
}
