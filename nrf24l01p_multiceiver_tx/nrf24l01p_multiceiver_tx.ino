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

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 
RF24 radio(9,10);

// Topology
const uint64_t pipes[2] = { 0xB3B4B5B6F1LL, 0xDEADBEEFFFLL };              // Radio pipe addresses for the 2 nodes to communicate.

// A single byte to keep track of the data being sent back and forth
unsigned long counter = 1;

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

union PayloadRaw uiTxBuf, uiRxBuf;

extern union EEPROM_AddrDataRaw uiEEPROMAddrData;

void setup()
{

  Serial.begin(115200);
  printf_begin();
  printf("\n\r");
  printf("%7lu) ## NK [%s:%d] RF24 Multiceiver(Transmitter Role)\n", millis(), __func__, __LINE__);

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

  radio.openWritingPipe(uiEEPROMAddrData.AD_TxAddr);
  // Below line is not necessary for TX but better to set with any specific address than reset default address
  radio.openReadingPipe(1,uiEEPROMAddrData.AD_AnyAddr);      // Not necessary for TX    

  radio.enableDynamicPayloads();
//  radio.disableDynamicPayloads();
  radio.enableAckPayload();
  
  radio.stopListening();                                  // First, stop listening so we can talk.
  radio.printDetails();  
}

void loop(void) {

  byte pipeNo;  
  byte gotByte;  
  unsigned long time;                          // Take the time, and send it.  This will block until complete   
                                               //Called when STANDBY-I mode is engaged (User is finished sending)
  unsigned char ucObserveTx;

  printf("Now sending %d as payload. \n",counter);  
                                                              
  memset(uiTxBuf.cCh, 0, PAYLOAD_SIZE);
  uiTxBuf.PL_TimeStamp = millis();
  uiTxBuf.PL_Counter = counter++;

  time = micros();
  radio.flush_rx(); // NK 
  if (!radio.write( uiTxBuf.cCh, PAYLOAD_SIZE )){
    printf("failed.\n");

    radio.setChannel(uiEEPROMAddrData.AD_RFCh); // reset the OBSERVE_TX:PLOS_CNT
    radio.printDetails();

  } else {
//    delay(10);  // 
    pipeNo = -1;
    if(!radio.available(&pipeNo)){
      printf("Pipe:%d, Blank Payload Received.", pipeNo); 
    } else {
      while(radio.available(&pipeNo) ){
        unsigned long tim = micros();
        radio.read( uiRxBuf.cCh, PAYLOAD_SIZE);
        printf("Got response %lu, round-trip delay: %lu microseconds\n\r",uiRxBuf.PL_Counter,tim-time);
        if( tim-time > 2000 ) {
          radio.printDetails();
        }

        printf("Observe Tx : 0x%02x\n", ucObserveTx );
        printf("Status     : 0x%02x\n", radio.getStatus());
        if( ucObserveTx & 0xF0 == 0xFF ) {
          radio.setChannel(uiEEPROMAddrData.AD_RFCh);
        }
//        counter++;
      }
    }
  }
    
  // Try again later
  delay(3000);  // decreasing it, make it brutal
}
