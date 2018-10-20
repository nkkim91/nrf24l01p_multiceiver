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
#include <EEPROM.h>

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 
RF24 radio(9,10);

// Topology
const uint64_t pipes[2] = { 0xB3B4B5B6F1LL, 0xDEADBEEFFFLL };              // Radio pipe addresses for the 2 nodes to communicate.

// A single byte to keep track of the data being sent back and forth
byte counter = 1;

#define PAYLOAD_SIZE (32)

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

union PayloadRaw uiTxBuf, uiRxBuf;

#define EEPROM_BUTTON_PIN (5)

void setup()
{
  int i;
  unsigned long long ullData;

  Serial.begin(115200);
  printf_begin();
  printf("\n\r");
  printf("%7lu) ## NK [%s:%d] RF24 Multiceiver(Receiver Role)\n", millis(), __func__, __LINE__);

  if( PRawSize!=PAYLOAD_SIZE ) {
    printf("PayloadData size is bigger PAYLOAD_SIZE !!");
    while(1);
  }

  while(1) {
    ullData = EEPROM_InputAddrData();
    if( ullData ) 
      printf("%7lu) ## NK [%s:%d] 0x%08lx%08lx\n", millis(), __func__, __LINE__, (unsigned long)(ullData >> 32), (unsigned long)(ullData & 0x00000000FFFFFFFF));
  }

  printf("EEPROM.length() : %d\n", EEPROM.length());
  printf("unsigned long long : %d\n", sizeof(unsigned long long));
  printf("uint64_t : %d\n", sizeof(uint64_t));
  printf("unsigned long : %d\n", sizeof(unsigned long));

  pinMode(EEPROM_BUTTON_PIN, INPUT);
  
  if( digitalRead(EEPROM_BUTTON_PIN) ) {
    printf("EEPROM Button Pressed !!\n");
    printf("Initialize EEPROM and write the address !!\n");
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
  radio.setRetries(1,15);                 // Smallest time between retries, max no. of retries, NK - it's a key when payload size is bigger than 1
  radio.setPayloadSize(PAYLOAD_SIZE);     // Here we are sending 1-byte payloads to test the call-response speed
  radio.setDataRate(RF24_1MBPS);          // Set air data rate to 1 Mbps (Default)
  radio.setChannel(0x4C);                 // Set channel to 0x4C(76) - 2476 Hz

  // Below two lines are not necessary for RX but better to set with any specific address than reset default address
  radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing, Not necessary for RX
  radio.openReadingPipe(0,pipes[1]);      // Not necessary for RX
  
  radio.openReadingPipe(1,pipes[0]);
  radio.openReadingPipe(2,0xCDLL);
  radio.openReadingPipe(3,0xA3LL);
  radio.openReadingPipe(4,0x0FLL);
  radio.openReadingPipe(5,0x05LL);

  radio.enableDynamicPayloads();
//  radio.disableDynamicPayloads();
  radio.enableAckPayload();

  radio.startListening();                 // Start listening
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
}

void loop(void) {

  byte pipeNo;
  byte gotByte;                                       // Dump the payloads until we've gotten everything
  
  while( radio.available(&pipeNo)){
    radio.read( uiRxBuf.cCh, PAYLOAD_SIZE );
    radio.writeAckPayload(pipeNo,uiRxBuf.cCh, PAYLOAD_SIZE );
    printf("%7lu) ## NK [%s:%d] Pipe:%d, Pkt TimeStamp : %lu, Counter : %lu\n", millis(), __func__, __LINE__, pipeNo, uiRxBuf.PL_TimeStamp, uiRxBuf.PL_Counter);
 }
}
