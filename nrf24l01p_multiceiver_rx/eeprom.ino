

#define EEPROM_ADDR_OFFSET (0)
#define EEPROM_CRC_SIZE (4)
#define EEPROM_CRC_OFFSET (1024 - EEPROM_CRC_SIZE)

struct EEPROM_AddrData {
  unsigned long long ullRXAddr;   // ex, 0xB3B4B5B6F1LL
  unsigned long long ullTXAddr;   // 
  unsigned long long ullAnyAddr;  // ex, 0xDEADBEEFFFLL
};

#define EEPROM_ADDR_SIZE  sizeof(struct EEPROM_AddrData)

union EEPROM_AddrDataRaw {
  struct EEPROM_AddrData stEEPROMAddrData;
  char cCh[EEPROM_ADDR_SIZE];
};

#define AD_RxAddr stEEPROMAddrData.ullRXAddr 
#define AD_TxAddr stEEPROMAddrData.ullTXAddr 
#define AD_AnyAddr stEEPROMAddrData.ullAnyAddr 

union EEPROM_AddrDataRaw uiEEPROMAddrData, uiEEPROMTmpAddrData;


unsigned long eeprom_crc(void) {

  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;

  for (int index = 0 ; index < EEPROM.length() - EEPROM_CRC_SIZE  ; ++index) {
    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }

  return crc;
}

unsigned long long EEPROM_InputAddrData(void)
{
  char ch;
  int i = 0;
  unsigned long long ullTmpData = 0;

  while( 1 ) {
    
    if( Serial.available() ) {
  
      ch = Serial.read();
    
      if( ch != '\n' && i < 16) {  // '\n' = 10(0x0A)
      
        if( ch >= '0' && ch <= '9' ) {
          ullTmpData <<= 4; // shift nibble (4 bits)
          ullTmpData |= (ch - '0');
        } else if( toUpperCase(ch) >= 'A' && toUpperCase(ch) <= 'F') {
          ullTmpData <<= 4; // shift nibble (4 bits)
          ullTmpData |= toUpperCase(ch) - 'A' + 10;
        } else {
        // Do nothing
        }
        i++;
      } else {
//        printf("0x%08lx%08lx\n", (unsigned long)(ullTmpData >> 32), (unsigned long)(ullTmpData & 0x00000000FFFFFFFF));
        break;
      }
    }
  }
  
  return ullTmpData;
}

void EEPROM_WriteAddrData(void)
{
  int i;
  unsigned long ulEEPROMCrc, ulEEPROMTmpCrc;
  unsigned long ulTmpData;

  // EEPROM writing mode
  for( i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }

  uiEEPROMAddrData.AD_RxAddr = 0xB3B4B5B6F1LL;
  uiEEPROMAddrData.AD_TxAddr = 0xB3B4B5B6F1LL;
  uiEEPROMAddrData.AD_AnyAddr = 0xDEADBEEFFFLL;

  for( i = 0; i < EEPROM_ADDR_SIZE; i++) {
    EEPROM.write(EEPROM_ADDR_OFFSET + i, uiEEPROMAddrData.cCh[i]);
  }

  // Generate CRC
  ulEEPROMCrc = eeprom_crc();
//  printf("EEPROM CRC after writing : 0x%08lx\n", ulEEPROMCrc);
  
  for( i = 0; i < EEPROM_CRC_SIZE; i++) {
//    printf("ulEEPROMCrc >> i : 0x%02lx\n", (ulEEPROMCrc >> i*8) & 0xFF);
    EEPROM.write(EEPROM.length() - EEPROM_CRC_SIZE + i, (ulEEPROMCrc >> i*8) & 0xFF);
  }

//  printf("EEPROM CRC Read after update : 0x%08lx\n", eeprom_crc());

  for( i = 0; i < EEPROM.length(); i++) {
    printf("0x%02x ", EEPROM.read(i));
    if( !((i+1) % 16) ) {
      printf("\n");
    }
  }
}


void EEPROM_ReadAddrData(void) 
{
  int i;
  unsigned long ulEEPROMCrc, ulEEPROMTmpCrc;
  unsigned long ulTmpData;
  
  // EEPROM reading mode
  ulEEPROMCrc = eeprom_crc();
  printf("CRC generated from EEPROM memory  : 0x%08lx\n", ulEEPROMCrc);  
  
  for( i = EEPROM_CRC_SIZE - 1, ulEEPROMTmpCrc = 0; i >= 0 ; i--) {
    ulTmpData = ((EEPROM.read(EEPROM.length() - EEPROM_CRC_SIZE + i) & 0xFF));
    ulEEPROMTmpCrc |= (ulTmpData << i*8);
  }
  printf("CRC from EEPROM memory  : 0x%08lx\n", ulEEPROMTmpCrc);

  if( ulEEPROMCrc == ulEEPROMTmpCrc ) {
    for( i = 0; i < EEPROM_ADDR_SIZE; i++) {
      uiEEPROMAddrData.cCh[i] = EEPROM.read(EEPROM_ADDR_OFFSET + i);
      printf("0x%02x ", (unsigned char)uiEEPROMAddrData.cCh[i]);
    }
    printf("\n");

    printf("EEPROM RxAddr  : 0x%08lx%08lx\n", (unsigned long)(uiEEPROMAddrData.AD_RxAddr >> 32), (unsigned long)(uiEEPROMAddrData.AD_RxAddr & 0xFFFFFFFF));
    printf("EEPROM TxAddr  : 0x%08lx%08lx\n", (unsigned long)(uiEEPROMAddrData.AD_TxAddr >> 32), (unsigned long)(uiEEPROMAddrData.AD_TxAddr & 0xFFFFFFFF));
    printf("EEPROM AnyAddr : 0x%08lx%08lx\n", (unsigned long)(uiEEPROMAddrData.AD_AnyAddr >> 32), (unsigned long)(uiEEPROMAddrData.AD_AnyAddr & 0xFFFFFFFF));
  } else {
    printf("CRC is different !! EEPROM data seems to be out-dated !! \n");
  }
}
