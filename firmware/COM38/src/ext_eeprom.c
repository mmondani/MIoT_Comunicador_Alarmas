#include <asf.h>
#include "./inc/project.h"
#include "./inc/ext_eeprom.h"
#include "./inc/EE25LCxxxx.h"




void ext_eeprom_init (void)
{
	ee25lcxxxx_init();
}


uint8_t ext_eeprom_read_byte (uint32_t addr)
{
	uint16_t data;
	
	ee25lcxxxx_busyPolling();
	ee25lcxxxx_read(addr, (uint8_t*)&data, 1);

	return data & 0x00ff;
}


void ext_eeprom_write_byte (uint32_t addr, uint8_t data)
{
	ee25lcxxxx_busyPolling();
	ee25lcxxxx_setWriteEnable();
	ee25lcxxxx_write(addr, (uint8_t*)&data, 1);
}


void ext_eeprom_read_bytes (uint32_t addr, uint8_t* data, uint32_t len)
{
	ee25lcxxxx_busyPolling();
	ee25lcxxxx_read(addr, data, len);
}


void ext_eeprom_write_bytes (uint32_t addr, uint8_t* data, uint32_t len)
{
	ee25lcxxxx_busyPolling();
	ee25lcxxxx_setWriteEnable();
	ee25lcxxxx_write(addr, data, len);
}


uint32_t ext_eeprom_read_uint32 (uint32_t addr)
{
	uint32_t data;
	
	ee25lcxxxx_busyPolling();
	ee25lcxxxx_read(addr, (uint8_t*)&data, 4);
	
	return data;
}


void ext_eeprom_write_uint32 (uint32_t addr, uint32_t data)
{
	ee25lcxxxx_busyPolling();
	ee25lcxxxx_setWriteEnable();
	ee25lcxxxx_write(addr, (uint8_t*)&data, 4);
}


float ext_eeprom_read_float (uint32_t addr)
{
	float data;
	
	ee25lcxxxx_busyPolling();
	ee25lcxxxx_read(addr, (uint8_t*)&data, 4);
	
	return data;
}


void ext_eeprom_write_float (uint32_t addr, float data)
{
	ee25lcxxxx_busyPolling();
	ee25lcxxxx_setWriteEnable();
	ee25lcxxxx_write(addr, (uint8_t*)&data, 4);
}


void ext_eeprom_erase (uint32_t addr, uint32_t len)
{
	ee25lcxxxx_busyPolling();
	ee25lcxxxx_setWriteEnable();
	ee25lcxxxx_erase(addr, len);
}


void ext_eeprom_pageErase (uint32_t addr)
{
	ee25lcxxxx_busyPolling();
	ee25lcxxxx_setWriteEnable();
	ee25lcxxxx_pageErase(addr);
}