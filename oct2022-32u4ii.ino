// MIT License
// https://github.com/gonzalocasas/arduino-uno-dragino-lorawan/blob/master/LICENSE
// Based on examples from https://github.com/matthijskooijman/arduino-lmic
// Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
// Adaptions: Andreas Spiess
#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
//#include <credentials.h>

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8]= { 0x60, 0x81, 0xF9, 0x3E, 0xD4, 0x5F, 0xEF, 0x90 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]= { 0x60, 0x81, 0xF9, 0x92, 0xC7, 0xE8, 0x65, 0x3F };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from the TTN console can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = { 0x92, 0x59, 0x83, 0x2F, 0x78, 0x58, 0x0A, 0xBD, 0xF3, 0x9D, 0x95, 0xA1, 0xDF, 0x0B, 0xB1, 0xF4 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}


#include <CayenneLPP.h>
#include <LowPower.h>

CayenneLPP lpp(uint8_t size);

//Sleep time/min
int sleepTime=1;



static osjob_t sendjob;

unsigned long readVcc() { 
  long result; // Read 1.1V reference against AVcc 
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1); 
  delay(2); // Wait for Vref to settle 
  ADCSRA |= _BV(ADSC); // Convert 
  while (bit_is_set(ADCSRA,ADSC)); 
  result = ADCL; 
  result |= ADCH<<8; 
  result = 1126400L / result; // Back-calculate AVcc in mV 
  return result; 
}

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 0;

// Pin mapping Dragino Shield
const lmic_pinmap lmic_pins = {
	.nss = 8,
	.rxtx = LMIC_UNUSED_PIN,
	.rst = 4,
	.dio = {7, 5, LMIC_UNUSED_PIN},
};
void onEvent (ev_t ev) {
    if (ev == EV_TXCOMPLETE) {
        Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
        // Schedule next transmission
        //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);

        //Sleep for requested time
        hal_sleep();  //Module sleep
        for (int i=0; i<sleepTime*6; i++) {
         //Use library from https://github.com/rocketscream/Low-Power
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        }
        
        //Send next payload
        do_send(&sendjob);        
    }
}

void do_send(osjob_t* j){
  
//---------------------------------------------
//Put your code here
//---------------------------------------------

  //https://www.thethingsnetwork.org/docs/devices/arduino/api/cayennelpp.html 
  CayenneLPP lpp(51);

  lpp.reset();
    
  while (readVcc()<3000) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  
  float u = readVcc();
  lpp.addAnalogInput(1, u/1000);
  
  Serial.print("Napeti baterie: ");
  Serial.print(u);
  Serial.println(" V");

  //lpp.addTemperature(1, 22.3);
  //lpp.addBarometricPressure(2, 1073.21);
  //lpp.addGPS(3, 52.37365, 4.88650, 2);

  // Payload to send (uplink)
  //static uint8_t message[] = "0";

//---------------------------------------------
// End of user code
//---------------------------------------------

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        //LMIC_setTxData2(1, lpp.getBuffer(),  lpp.getSize()-1, 0);
        LMIC_setTxData2(1, lpp.getBuffer(),  lpp.getSize(), 0);
        Serial.println(F("Sending uplink packet..."));
    }
    // Next TX is scheduled after TX_COMPLETE event.
    
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("Starting..."));
      
    // LMIC init
    os_init();
    
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();    
    
   

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;
    
    for (int i = 1; i <= 8; i++) {
      LMIC_disableChannel(i);
      }
      
    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF7,14);

    // Start job
    do_send(&sendjob);
    //delay(5000);
    
}

void loop() {
    os_runloop_once();
    //delay(5000);
}
