#ifndef EE25LCXXXX_H_
#define EE25LCXXXX_H_

#include "basicDefinitions.h"


/* Comentar este define si se usa una EEPROM de menor tamaño */
#define EE25LCXXXX_25LC1024


void ee25lcxxxx_init (void);
void ee25lcxxxx_writeStatus (uint8_t data);
uint8_t ee25lcxxxx_readStatus (void);
void ee25lcxxxx_busyPolling (void);
void ee25lcxxxx_setWriteEnable (void);
void ee25lcxxxx_read (uint32_t address, uint8_t* data, uint32_t len);
void ee25lcxxxx_write (uint32_t address, uint8_t* data, uint32_t len);
void ee25lcxxxx_erase (uint32_t address, uint32_t len);
void ee25lcxxxx_pageErase (uint32_t address);


#endif /* EE25LCXXXX_H_ */