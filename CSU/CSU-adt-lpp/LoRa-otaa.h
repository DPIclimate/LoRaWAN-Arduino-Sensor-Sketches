
// ensure DEVEUI and APPEUI are both LSB with APPKEY being MSB
static const u1_t PROGMEM DEVEUI[8]={ FILL_ME_IN };
static const u1_t PROGMEM APPEUI[8]={ FILL_ME_IN };
static const u1_t PROGMEM APPKEY[16] = { FILL_ME_IN };

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
