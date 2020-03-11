// app = dpi-test
// ensure DEVEUI and APPEUI are both LSB
static const u1_t PROGMEM DEVEUI[8]={ 0x27, 0x9A, 0xE4, 0xF3, 0x55, 0xCE, 0xA4, 0x00 };
static const u1_t PROGMEM APPEUI[8]={ 0xC2, 0xC0, 0x02, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
static const u1_t PROGMEM APPKEY[16] = { 0xB0, 0x3E, 0x60, 0x7F, 0x13, 0xD2, 0xEB, 0x6B, 0x4B, 0x06, 0xE3, 0x3C, 0xEC, 0xF1, 0x39, 0x1C };



void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

//// Pin mapping - M0
const lmic_pinmap lmic_pins = {
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {3, 6, LMIC_UNUSED_PIN},
};
