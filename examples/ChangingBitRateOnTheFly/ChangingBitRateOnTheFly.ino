//------------------------------- Board Check ----------------------------------

#ifndef ARDUINO_ARCH_ESP32
  #error "Select an ESP32 board"
#endif

//------------------------------- Include files --------------------------------

#include <ACAN_ESP32.h>
#include <core_version.h> // For ARDUINO_ESP32_RELEASE

//——————————————————————————————————————————————————————————————————————————————
//   SETUP
//——————————————————————————————————————————————————————————————————————————————

void setup() {
 //--- Switch on builtin led
  pinMode (LED_BUILTIN, OUTPUT) ;
  digitalWrite (LED_BUILTIN, HIGH) ;
//--- Start serial
  Serial.begin (115200) ;
  delay (100) ;
  while (!Serial) {}
//--- Display ESP32 Chip Info
  esp_chip_info_t chip_info ;
  esp_chip_info (&chip_info) ;
  Serial.print ("ESP32 Arduino Release: ") ;
  Serial.println (ARDUINO_ESP32_RELEASE) ;
  Serial.print ("ESP32 Chip Revision: ") ;
  Serial.println (chip_info.revision) ;
  Serial.print ("ESP32 SDK: ") ;
  Serial.println (ESP.getSdkVersion ()) ;
  Serial.print ("ESP32 Flash: ") ;
  Serial.print (spi_flash_get_chip_size () / (1024 * 1024)) ;
  Serial.print (" MB ") ;
  Serial.println (((chip_info.features & CHIP_FEATURE_EMB_FLASH) != 0) ? "(embeded)" : "(external)") ;
  Serial.print ("APB CLOCK: ") ;
  Serial.print (APB_CLK_FREQ) ;
  Serial.println (" Hz") ;
}

//——————————————————————————————————————————————————————————————————————————————

static uint32_t gBlinkLedDate = 0 ;
static const uint32_t bitRateArray [4] = { 125000, 250000, 500000, 1000000 } ;
static uint32_t gBitRateIndex = 0 ;
static const uint32_t MESSAGE_COUNT = 10 ;

//——————————————————————————————————————————————————————————————————————————————
//   LOOP
//——————————————————————————————————————————————————————————————————————————————

void loop () {
  if (gBlinkLedDate < millis ()) {
    gBlinkLedDate += 5000 ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  //--- Configure ESP32 CAN
    Serial.print ("Configure ESP32 CAN at ") ;
    Serial.print (bitRateArray [gBitRateIndex]) ;
    Serial.println (" bit/s") ;
    ACAN_ESP32_Settings settings (bitRateArray [gBitRateIndex]) ;
    gBitRateIndex = (gBitRateIndex + 1) % 4 ;
 //   settings.mRequestedCANMode = ACAN_ESP32_Settings::LoopBackMode ;
  //  settings.mRxPin = GPIO_NUM_4 ; // Optional, default Tx pin is GPIO_NUM_4
  //  settings.mTxPin = GPIO_NUM_5 ; // Optional, default Rx pin is GPIO_NUM_5
    const uint32_t errorCode = ACAN_ESP32::can.begin (settings) ;
    if (errorCode == 0) {
      Serial.print ("Bit Rate prescaler: ") ;
      Serial.println (settings.mBitRatePrescaler) ;
      Serial.print ("Time Segment 1:     ") ;
      Serial.println (settings.mTimeSegment1) ;
      Serial.print ("Time Segment 2:     ") ;
      Serial.println (settings.mTimeSegment2) ;
      Serial.print ("RJW:                ") ;
      Serial.println (settings.mRJW) ;
      Serial.print ("Triple Sampling:    ") ;
      Serial.println (settings.mTripleSampling ? "yes" : "no") ;
      Serial.print ("Actual bit rate:    ") ;
      Serial.print (settings.actualBitRate ()) ;
      Serial.println (" bit/s") ;
      Serial.print ("Exact bit rate ?    ") ;
      Serial.println (settings.exactBitRate () ? "yes" : "no") ;
      Serial.print ("Distance            ") ;
      Serial.print (settings.ppmFromDesiredBitRate ()) ;
      Serial.println (" ppm") ;
      Serial.print ("Sample point:       ") ;
      Serial.print (settings.samplePointFromBitStart ()) ;
      Serial.println ("%") ;
      Serial.println ("Configuration OK!") ;
    }else {
      Serial.print ("Configuration error 0x") ;
      Serial.println (errorCode, HEX) ;
    }
  //--- Loop for sending and receiving MESSAGE_COUNT messages
    uint32_t sentCount = 0 ;
    uint32_t receiveCount = 0 ;
    uint32_t statusPrintDate = millis () ;
    while (((sentCount < MESSAGE_COUNT) || (receiveCount < MESSAGE_COUNT)) && (gBlinkLedDate > millis ())) {
      if (statusPrintDate <= millis ()) {
        Serial.print ("  STATUS 0x") ;
        Serial.print (CAN_STATUS, HEX) ;
        Serial.print (", RXERR ") ;
        Serial.print (CAN_RX_ECR) ;
        Serial.print (", TXERR ") ;
        Serial.println (CAN_TX_ECR) ;
        statusPrintDate += 250 ;
      }
      if (sentCount < MESSAGE_COUNT) {
        CANMessage frame ;
        const bool ok = ACAN_ESP32::can.tryToSend (frame) ;
        if (ok) {
          sentCount += 1 ;
          Serial.print ("  Sent ") ;
          Serial.print (sentCount) ;
          Serial.print (", STATUS 0x") ;
          Serial.print (CAN_STATUS, HEX) ;
          Serial.print (", RXERR ") ;
          Serial.print (CAN_RX_ECR) ;
          Serial.print (", TXERR ") ;
          Serial.println (CAN_TX_ECR) ;
        }
      }
      CANMessage frame ;
      while (ACAN_ESP32::can.receive (frame)) {
        receiveCount += 1 ;
        Serial.print ("  Received ") ;
        Serial.println (receiveCount) ;
      }
    }
  }
}
