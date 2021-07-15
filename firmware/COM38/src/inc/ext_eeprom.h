#ifndef EXT_EEPROM_H_
#define EXT_EEPROM_H_


#include "basicDefinitions.h"


#define EXT_EEPROM_PAGE_SIZE		256


void ext_eeprom_init (void);
uint8_t ext_eeprom_read_byte (uint32_t address);
void ext_eeprom_write_byte (uint32_t address, uint8_t data);
void ext_eeprom_read_bytes (uint32_t address, uint8_t* data, uint32_t len);
void ext_eeprom_write_bytes (uint32_t address, uint8_t* data, uint32_t len);
uint32_t ext_eeprom_read_uint32 (uint32_t address);
void ext_eeprom_write_uint32 (uint32_t address, uint32_t data);
float ext_eeprom_read_float (uint32_t address);
void ext_eeprom_write_float (uint32_t address, float data);
void ext_eeprom_erase (uint32_t address, uint32_t len);
void ext_eeprom_pageErase (uint32_t addr);



#endif /* EXT_EEPROM_H_ */