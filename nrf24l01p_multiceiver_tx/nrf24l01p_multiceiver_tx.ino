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

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 
RF24 radio(9,10);

// Topology
const uint64_t pipes[2] = { 0xB3B4B5B6F1LL, 0xDEADBEEFFFLL };              // Radio pipe addresses for the 2 nodes to communicate.

// A single byte to keep track of the data being sent back and forth
unsigned long counter = 1;

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

void setup(){

  Serial.begin(115200);
  printf_begin();
  printf("\n\r");
  printf("%7lu) ## NK [%s:%d] RF24 Multiceiver(Transmitter Role)\n", millis(), __func__, __LINE__);

  if( PRawSize!=PAYLOAD_SIZE )
    printf("PayloadData size is bigger PAYLOAD_SIZE");
  

  // Setup and configure rf radio

  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(1,15);                 // Smallest time between retries, max no. of retries, NK - it's a key when payload size is bigger than 1
  radio.setPayloadSize(PAYLOAD_SIZE);     // Here we are sending 1-byte payloads to test the call-response speed
  radio.setDataRate(RF24_1MBPS);          // Set air data rate to 1 Mbps (Default)
  radio.setChannel(0x4C);                 // Set channel to 0x4C(76) - 2476 Hz

  radio.openWritingPipe(pipes[0]);
  // Below line is not necessary for TX but better to set with any specific address than reset default address
  radio.openReadingPipe(1,pipes[1]);      // Not necessary for TX    

  radio.enableDynamicPayloads();
//  radio.disableDynamicPayloads();
  radio.enableAckPayload();
  
  radio.printDetails();

  radio.stopListening();                                  // First, stop listening so we can talk.
}

void loop(void) {

  printf("Now sending %d as payload. ",counter);
  byte gotByte;  
  unsigned long time = micros();                          // Take the time, and send it.  This will block until complete   
                                                            //Called when STANDBY-I mode is engaged (User is finished sending)
  memset(uiTxBuf.cCh, 0, PAYLOAD_SIZE);
  uiTxBuf.PL_TimeStamp = millis();
  uiTxBuf.PL_Counter = counter;

  if (!radio.write( uiTxBuf.cCh, PAYLOAD_SIZE )){
    Serial.println(F("failed."));
  } else {

    if(!radio.available()){ 
      Serial.println(F("Blank Payload Received.")); 
    } else {
      while(radio.available() ){
        unsigned long tim = micros();
        radio.read( uiRxBuf.cCh, PAYLOAD_SIZE);
        printf("Got response %lu, round-trip delay: %lu microseconds\n\r",uiRxBuf.PL_Counter,tim-time);
        counter++;
      }
    }
  }
    
  // Try again later
  delay(1000);  // decreasing it, make it brutal
}
