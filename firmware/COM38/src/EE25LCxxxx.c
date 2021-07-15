#include <asf.h>
#include "../inc/EE25LCxxxx.h"
#include "../inc/project.h"




// ********************************************************************************
// Comandos para las memorias EEPROM 25LCxxx
// ********************************************************************************
#define EE25LCXXX_WREN			0b00000110			// Write enable
#define EE25LCXXX_WRITE			0b00000010			// Write EEPROM
#define EE25LCXXX_READ			0b00000011			// Read EEPROM
#define EE25LCXXX_RDSR			0b00000101			// Read status register
#define EE25LCXXX_WRSR			0b00000001			// Write status register
#define EE25LCXXX_PE			0b01000010 			// Page erase
// ********************************************************************************


struct spi_module spi_master_instance;
struct spi_slave_inst slave;




void ee25lcxxxx_init (void) 
{
	enum status_code retCode;
	
	
	struct spi_config config_spi_master;
	struct spi_slave_inst_config slave_dev_config;


	spi_slave_inst_get_config_defaults(&slave_dev_config);
	slave_dev_config.ss_pin = SPI_25LC1024_SS_PIN;

	spi_attach_slave(&slave, &slave_dev_config);


	spi_get_config_defaults(&config_spi_master);
	config_spi_master.mode_specific.master.baudrate = 1000000;
	config_spi_master.transfer_mode = SPI_TRANSFER_MODE_0;
	config_spi_master.mux_setting = CONF_MASTER_MUX_SETTING;
	config_spi_master.pinmux_pad0 = CONF_MASTER_PINMUX_PAD0;
	config_spi_master.pinmux_pad1 = CONF_MASTER_PINMUX_PAD1;
	config_spi_master.pinmux_pad2 = CONF_MASTER_PINMUX_PAD2;
	config_spi_master.pinmux_pad3 = CONF_MASTER_PINMUX_PAD3;

	retCode = spi_init (&spi_master_instance, CONF_MASTER_SPI_MODULE, &config_spi_master);
	spi_enable(&spi_master_instance);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
}


void ee25lcxxxx_writeStatus (uint8_t data) 
{
	uint16_t aux;
	enum status_code retCode;
	
	
	spi_select_slave(&spi_master_instance, &slave, true);
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, EE25LCXXX_RDSR, &aux);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, data, &aux);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
	
	while (!spi_is_write_complete(&spi_master_instance));
	retCode = spi_select_slave(&spi_master_instance, &slave, false);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
}


uint8_t ee25lcxxxx_readStatus (void)
{
	uint16_t data;
	enum status_code retCode;
	
	spi_select_slave(&spi_master_instance, &slave, true);
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, EE25LCXXX_RDSR, &data);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
	
	while (!spi_is_write_complete(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, 0, &data);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
	
	retCode = spi_select_slave(&spi_master_instance, &slave, false);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
	
	return data & 0x00FF;
}


void ee25lcxxxx_busyPolling (void)
{
	uint16_t data;
	enum status_code retCode;
	uint32_t timeout = 0x0fff;
	
	do
	{
		spi_select_slave(&spi_master_instance, &slave, true);
		
		while (!spi_is_ready_to_write(&spi_master_instance));
		retCode = spi_transceive_wait(&spi_master_instance, EE25LCXXX_RDSR, &data);
		
		if (retCode != STATUS_OK)
			errores1.bits.errorEeprom = 1;
		
		while (!spi_is_write_complete(&spi_master_instance));
		retCode = spi_transceive_wait(&spi_master_instance, 0, &data);
		
		if (retCode != STATUS_OK)
			errores1.bits.errorEeprom = 1;
		
		retCode = spi_select_slave(&spi_master_instance, &slave, false);
		
		if (retCode != STATUS_OK)
			errores1.bits.errorEeprom = 1;

		timeout --;
	} while (data & 0x01 && timeout > 0); // se queda en el loop mientras la EEPROM está escribiendo.
	
	
	if (timeout == 0)
		errores1.bits.errorEeprom = 1;
}


void ee25lcxxxx_setWriteEnable (void)
{
	uint16_t data;
	enum status_code retCode;
	
	spi_select_slave(&spi_master_instance, &slave, true);
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, EE25LCXXX_WREN, &data);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
	
	while (!spi_is_write_complete(&spi_master_instance));
	retCode = spi_select_slave(&spi_master_instance, &slave, false);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
}


void ee25lcxxxx_read (uint32_t address, uint8_t* data, uint32_t len)
{
	uint16_t data8;
	enum status_code retCode;


	spi_select_slave(&spi_master_instance, &slave, true);
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, EE25LCXXX_READ, &data8);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;

	// En las 25LC1024, las direcciones son de 24 bits. En todas las otras son de 16 bits.
#ifdef EE25LCXXXX_25LC1024
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, (address >> 16) & 0x000000ff, &data8);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
#endif

	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, (address >> 8) & 0x000000ff, &data8);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, address & 0x000000ff, &data8);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;


	// Se leen los datos
	while (len > 0)
	{
		while (!spi_is_write_complete(&spi_master_instance));
		retCode = spi_transceive_wait(&spi_master_instance, 0, &data8);
		*data = data8;
		
		if (retCode != STATUS_OK)
			errores1.bits.errorEeprom = 1;

		data++;
		len--;
	}
	
	while (!spi_is_write_complete(&spi_master_instance));
	retCode = spi_select_slave(&spi_master_instance, &slave, false);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
}


void ee25lcxxxx_write (uint32_t address, uint8_t* data, uint32_t len)
{
	uint16_t data8;
	uint16_t aux;
	enum status_code retCode;


	spi_select_slave(&spi_master_instance, &slave, true);
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, EE25LCXXX_WRITE, &data8);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;

	// En las 25LC1024, las direcciones son de 24 bits. En todas las otras son de 16 bits.
#ifdef EE25LCXXXX_25LC1024
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, (address >> 16) & 0x000000ff, &data8);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
#endif

	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, (address >> 8) & 0x000000ff, &data8);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, address & 0x000000ff, &data8);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;


	// Se escriben los datos.
	while (len > 0)
	{
		data8 = *data;
		while (!spi_is_ready_to_write(&spi_master_instance));
		retCode = spi_transceive_wait(&spi_master_instance, data8, &aux);
		
		if (retCode != STATUS_OK)
			errores1.bits.errorEeprom = 1;

		data++;
		len--;
	}

	while (!spi_is_write_complete(&spi_master_instance));
	retCode = spi_select_slave(&spi_master_instance, &slave, false);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
}


void ee25lcxxxx_erase (uint32_t address, uint32_t len)
{
	uint16_t aux;
	enum status_code retCode;
	
	
	spi_select_slave(&spi_master_instance, &slave, true);
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, EE25LCXXX_WRITE, &aux);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;

	// EN las 25LC1024, las direccionesson de 24 bits. En todas las otras son de 16 bits.
#ifdef EE25LCXXXX_25LC1024
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, (address >> 16) & 0x000000ff, &aux);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
#endif

	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, (address >> 8) & 0x000000ff, &aux);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, address & 0x000000ff, &aux);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;


	// Se escriben los datos.
	while (len > 0)
	{
		while (!spi_is_ready_to_write(&spi_master_instance));
		retCode = spi_transceive_wait(&spi_master_instance, 0xff, &aux);
		
		if (retCode != STATUS_OK)
			errores1.bits.errorEeprom = 1;

		len--;
	}

	while (!spi_is_write_complete(&spi_master_instance));
	retCode = spi_select_slave(&spi_master_instance, &slave, false);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
}


void ee25lcxxxx_pageErase (uint32_t address)
{
	uint16_t aux;
	enum status_code retCode;
	
	
	spi_select_slave(&spi_master_instance, &slave, true);
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, EE25LCXXX_PE, &aux);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;

	// EN las 25LC1024, las direccionesson de 24 bits. En todas las otras son de 16 bits.
#ifdef EE25LCXXXX_25LC1024
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, (address >> 16) & 0x000000ff, &aux);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
#endif

	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, (address >> 8) & 0x000000ff, &aux);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
	
	while (!spi_is_ready_to_write(&spi_master_instance));
	retCode = spi_transceive_wait(&spi_master_instance, address & 0x000000ff, &aux);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;


	while (!spi_is_write_complete(&spi_master_instance));
	retCode = spi_select_slave(&spi_master_instance, &slave, false);
	
	if (retCode != STATUS_OK)
		errores1.bits.errorEeprom = 1;
}