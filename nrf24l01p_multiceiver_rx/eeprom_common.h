
#define EEPROM_ADDR_OFFSET (0)
#define EEPROM_CRC_SIZE (4)
#define EEPROM_CRC_OFFSET (1024 - EEPROM_CRC_SIZE)

#define EEPROM_BUTTON_PIN (6)

struct EEPROM_AddrData {
  unsigned long long ullRXAddr;   // ex, 0xB3B4B5B6F1LL
  unsigned long long ullTXAddr;   // 
  unsigned long long ullAnyAddr;  // ex, 0xDEADBEEFFFLL
  unsigned int unRFChannel;       // ex, 0x1C (2400 + 28 = 2428 Mhz)
};

#define EEPROM_ADDR_SIZE  sizeof(struct EEPROM_AddrData)

union EEPROM_AddrDataRaw {
  struct EEPROM_AddrData stEEPROMAddrData;
  char cCh[EEPROM_ADDR_SIZE];
};

#define AD_RxAddr   stEEPROMAddrData.ullRXAddr 
#define AD_TxAddr   stEEPROMAddrData.ullTXAddr 
#define AD_AnyAddr  stEEPROMAddrData.ullAnyAddr 
#define AD_RFCh     stEEPROMAddrData.unRFChannel 
