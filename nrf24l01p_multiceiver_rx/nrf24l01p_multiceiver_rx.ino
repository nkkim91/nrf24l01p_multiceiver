/*
  // March 2014 - TMRh20 - Updated along with High Speed RF24 Library fork
  // Parts derived from examples by J. Coliz <maniacbug@ymail.com>
*/
/**
 * Example for efficient call-response using ack-payloads 
 *
 * This example continues to make use of all the normal functionality of the radios including
 * the auto-ack and auto-retry features, but allows ack-payloads to be written optionally as well.
 * This allows very fast call-response communication, with the responding radio never having to 
 * switch out of Primary Receiver mode to send back a payload, but having the option to if wanting
 * to initiate communication instead of respond to a commmunication.
 */
 


#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "eeprom_common.h"

#define NUM_OF_PIPES  6

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 
RF24 radio(9,10);

// Topology
const uint64_t pipes[2] = { 0xB3B4B5B6F1LL, 0xDEADBEEFFFLL };              // Radio pipe addresses for the 2 nodes to communicate.

// A single byte to keep track of the data being sent back and forth

#define PAYLOAD_SIZE (16)

struct PayloadData {
  unsigned long ulTimeStamp;
  unsigned long ulCounter;
};

union PayloadRaw {
  struct PayloadData stPData;
  char cCh[PAYLOAD_SIZE];
};

#define PRawSize sizeof(union PayloadRaw)

#define PL_TimeStamp stPData.ulTimeStamp 
#define PL_Counter stPData.ulCounter 

union PayloadRaw uiTxBuf, uiRxBuf[NUM_OF_PIPES];
unsigned char ucRxBufFlag[NUM_OF_PIPES];
unsigned char ucWriteAckFlag;

extern union EEPROM_AddrDataRaw uiEEPROMAddrData;

void setup()
{

  Serial.begin(115200);
  printf_begin();
  printf("\n\r");
  printf("%7lu) ## NK [%s:%d] RF24 Multiceiver(Receiver Role)\n", millis(), __func__, __LINE__);

  if( PRawSize!=PAYLOAD_SIZE ) {
    printf("PayloadData size is bigger PAYLOAD_SIZE !!");
    while(1);
  }

  printf("unsigned long long : %d\n", sizeof(unsigned long long));
  printf("uint64_t           : %d\n", sizeof(uint64_t));
  printf("unsigned long      : %d\n", sizeof(unsigned long));
  printf("unsigned int       : %d\n", sizeof(unsigned int));

  pinMode(EEPROM_BUTTON_PIN, INPUT);
  
  if( digitalRead(EEPROM_BUTTON_PIN) ) {
    printf("EEPROM Button Pressed !!\n");
    printf("Initialize EEPROM and write the address !!\n");

    EEPROM_UpdateAddrData();
    
    EEPROM_WriteAddrData();
    printf("Done !! Recycle the power !!\n");
    while(1);
  } else {
    printf("EEPROM Button is NOT pressed !!\n");
    printf("Read the address from EEPROM !!\n");
    EEPROM_ReadAddrData();
  }

  // Setup and configure rf radio

  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(7,15);                 // Smallest time between retries, max no. of retries, NK - it's a key when payload size is bigger than 1
  radio.setPayloadSize(PAYLOAD_SIZE);     // Here we are sending 1-byte payloads to test the call-response speed
  radio.setDataRate(RF24_250KBPS);          // Set air data rate to 1 Mbps (Default)
  radio.setChannel(uiEEPROMAddrData.AD_RFCh);                 // Set channel to 0x4C(76) - 2476 Hz
  radio.setPALevel(3);                    // Set PA Level (0 ~ 3)

  // Below two lines are not necessary for RX but better to set with any specific address than reset default address
  radio.openWritingPipe(uiEEPROMAddrData.AD_AnyAddr);        // Both radios listen on the same pipes by default, and switch when writing, Not necessary for RX
  radio.openReadingPipe(0,uiEEPROMAddrData.AD_AnyAddr);      // Not necessary for RX
  
  radio.openReadingPipe(1,uiEEPROMAddrData.AD_RxAddr);
  radio.openReadingPipe(2,0xCDLL);  // 0xB3B4B5B6CDLL
  radio.openReadingPipe(3,0xA3LL);  // 0xB3B4B5B6A3LL
  radio.openReadingPipe(4,0x0FLL);  // 0xB3B4B5B60FLL
  radio.openReadingPipe(5,0x05LL);  // 0xB3B4B5B605LL

  radio.enableDynamicPayloads();
//  radio.disableDynamicPayloads();
  radio.enableAckPayload();

  radio.startListening();                 // Start listening
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
}

int i;

void loop(void) {
  
  byte pipeNo;
  byte gotByte;                                       // Dump the payloads until we've gotten everything
#if 0
  while( radio.available(&pipeNo) ){
    memset(uiRxBuf[0].cCh, 0, PAYLOAD_SIZE);
    radio.read( uiRxBuf[0].cCh, PAYLOAD_SIZE );
//    memcpy(uiTxBuf.cCh, uiRxBuf.cCh, sizeof(union PayloadRaw));
    radio.writeAckPayload(pipeNo, uiRxBuf[0].cCh, PAYLOAD_SIZE );
//    printf("%7lu) ## NK [%s:%d] Pipe:%d, Pkt TimeStamp : %lu, Counter : %lu\n", millis(), __func__, __LINE__, pipeNo, uiRxBuf.PL_TimeStamp, uiRxBuf[0].PL_Counter);
//    delay(10);  // very necessary ? why ?
  }
#else
  for( i = 0; i < NUM_OF_PIPES; i++) {
    ucRxBufFlag[i] = 0;
  }
  ucWriteAckFlag = 0;
  while( radio.available(&pipeNo) ){
    if( pipeNo >= 0 && pipeNo < NUM_OF_PIPES ) {
      radio.read( uiRxBuf[pipeNo].cCh, PAYLOAD_SIZE );
      ucRxBufFlag[pipeNo] = 1;
      ucWriteAckFlag = 1;
    }
  }

  if( ucWriteAckFlag ) {
    for( i = 0; i < NUM_OF_PIPES; i++) {
      if( ucRxBufFlag[i] ) 
        radio.writeAckPayload(i, uiRxBuf[i].cCh, PAYLOAD_SIZE );
//    printf("%7lu) ## NK [%s:%d] Pipe:%d, Pkt TimeStamp : %lu, Counter : %lu\n", millis(), __func__, __LINE__, pipeNo, uiRxBuf.PL_TimeStamp, uiRxBuf.PL_Counter);
//    delay(10);  // very necessary ? why ?
    }
  }


#endif
}
