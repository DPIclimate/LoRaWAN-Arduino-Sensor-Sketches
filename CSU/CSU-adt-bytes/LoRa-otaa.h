// app = dpi-test
// ensure DEVEUI and APPEUI are both LSB
static const u1_t PROGMEM DEVEUI[8]={ 0x32, 0xB9, 0xF5, 0x0E, 0x09, 0x75, 0x40, 0x00 };
static const u1_t PROGMEM APPEUI[8]={ 0x7B, 0xBE, 0x02, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
static const u1_t PROGMEM APPKEY[16] = { 0xD1, 0x6D, 0x04, 0x3A, 0x5D, 0x6D, 0xED, 0x33, 0x00, 0xF5, 0x8B, 0x66, 0x8D, 0x4B, 0x3F, 0xDE };



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
