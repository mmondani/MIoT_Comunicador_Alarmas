#include "asf.h"
#include "inc/pga.h"
#include "inc/ext_eeprom.h"
#include "inc/mpxh.h"
#include "inc/EEPROM_MAP.h"
#include "inc/utilities.h"


uint8_t pointerRAM;
uint32_t queue[PGA_SAVE_QUEUE_LENGTH][3];
uint8_t queueIn;
uint8_t queueOut;
bool fullResetRequested;

void loadValues (void);
void defaults (void);
void defaultNames (void);



void pga_init (void) 
{
	fullResetRequested = false;
	pointerRAM = 0xFF;
}

void pga_checkEeprom (void) 
{
    u8 aux;

    aux = ext_eeprom_read_byte(EE_FLAG_ADDR);

    if (aux != 0x29)
    {
	    defaults();
    }
    else
    {
	    loadValues();
    }
}


void pga_resetAll (void) 
{
	fullResetRequested = true;
}


uint8_t pga_dumpByte (void)
{
    u8 ret;

    if (pointerRAM > PGA_LENGTH_MPXH)
    {
	    ret = 1;       // 1 --> terminó
    }
    else
    {
	    if ( !mpxh_Ocupado() )   // entro si no esta ocupado
	    {
		    if (!pointerRAM)
		    {
			    // Antes de mandar los bytes de la PGA, se manda el identificador
			    // del equipo.
			    mpxh_ArmaMensaje( ID_DEVICE_H, ID_DEVICE_L, 0, MPXH_BITS_17 );
			    pointerRAM++;
		    }
		    else
		    {
			    mpxh_ArmaMensaje( ID_PGA_15BITS_MPXH, pgaData[PGA_ID_DISPOSITIVO + pointerRAM - 1], 0, MPXH_BITS_15 );
			    pointerRAM++;
		    }

		    ret = 0;     // 0 --> fala PGA por salir
	    }
	    else
	    {
		    ret = 1;
	    }

    }

    return ret;
}


void pga_dumpPga (void)
{
    pointerRAM = 0;
}


bool pga_hayqueDumpear (void)
{
    bool ret = true;

    if (pointerRAM > PGA_LENGTH_MPXH) {
		ret = false;
	}

    return ret;
}


void pga_enqueueSave (uint32_t arrayIndex, uint32_t cant, uint32_t addr)
{
	queue[queueIn][0] = arrayIndex;
	queue[queueIn][1] = cant;
	queue[queueIn][2] = addr;
	

	queueIn ++;
	if (queueIn >= PGA_SAVE_QUEUE_LENGTH) {
		queueIn = 0;
	}
}


void pga_saveRAM (void)
{
	uint32_t i;
	uint32_t index;
	uint32_t addr;
	uint32_t len;


	if (!pga_isQueueEmpty()) {
		mpxh_forceMPXHLow();
		disableWDT();
		system_interrupt_enter_critical_section();
		
		for (; queueIn != queueOut; queueOut ++) {

			if (queueOut >= PGA_SAVE_QUEUE_LENGTH) {
				queueOut = 0;
			}

			if (queueIn != queueOut) {
				index = queue[queueOut][0];
				len = queue[queueOut][1];
				addr = queue[queueOut][2];

				ext_eeprom_write_bytes(addr + i, &(pgaData[index + i]), len);
			}
			else {
				break;
			}
		}

		system_interrupt_leave_critical_section();
		enableWDT();
		mpxh_releaseMPXH();
	}
	else if (fullResetRequested) {
		
		defaults();
		
		mpxh_forceMPXHLow();
		disableWDT();
		system_interrupt_enter_critical_section();
		
			
		for (i = 0; i < EE_PARTICIONES_NOMBRES_NUMBER_PAGES; i++) {
			addr = EE_PARTICIONES_NOMBRES_ADDR + i * EXT_EEPROM_PAGE_SIZE;
			ext_eeprom_pageErase(addr);
		}
		
		for (i = 0; i < EE_ZONAS_NOMBRES_NUMBER_PAGES; i++) {
			addr = EE_ZONAS_NOMBRES_ADDR + i * EXT_EEPROM_PAGE_SIZE;
			ext_eeprom_pageErase(addr);
		}
		
		for (i = 0; i < EE_USUARIO_NOMBRES_NUMBER_PAGES; i++) {
			addr = EE_USUARIO_NOMBRES_ADDR + i * EXT_EEPROM_PAGE_SIZE;
			ext_eeprom_pageErase(addr);
		}
		
		for (i = 0; i < EE_NODOS_NOMBRES_NUMBER_PAGES; i++) {
			addr = EE_NODOS_NOMBRES_ADDR + i * EXT_EEPROM_PAGE_SIZE;
			ext_eeprom_pageErase(addr);
		}
		
		system_interrupt_leave_critical_section();
		enableWDT();
		mpxh_releaseMPXH();
		
		
		fullResetRequested = false;
	}

}


bool pga_isQueueEmpty (void)
{
	return (queueIn == queueOut);
}


void loadValues (void)
{
	ext_eeprom_read_bytes(EE_NOMBRE_DISP_ADDR, &(pgaData[PGA_NOMBRE_DISP]), 31);
	ext_eeprom_read_bytes(EE_PROG_HORARIA_1_HORA_ENCENDIDO_ADDR, &(pgaData[PGA_PROG_HORARIA_1_HORA_ENCENDIDO]), 160);
	ext_eeprom_read_bytes(EE_FOTOTIMER_1_HORAS_ADDR, &(pgaData[PGA_FOTOTIMER_1_HORAS]), 112);
	ext_eeprom_read_bytes(EE_NOCHE_1_PARTICION_ADDR, &(pgaData[PGA_NOCHE_1_PARTICION]), 96);
	ext_eeprom_read_bytes(EE_SIMULADOR_1_PARTICION_ADDR, &(pgaData[PGA_SIMULADOR_1_PARTICION]), 96);
	ext_eeprom_read_bytes(EE_SERVER_KEY, &(pgaData[PGA_SERVER_KEY]), PGA_SERVER_KEY_LARGO);
	ext_eeprom_read_bytes(EE_MONITOREADA, &(pgaData[PGA_MONITOREADA]), 2);
	ext_eeprom_read_bytes(EE_BROKER_PORT, &(pgaData[PGA_BROKER_PORT]), 2);
	ext_eeprom_read_bytes(EE_BROKER_URL, &(pgaData[PGA_BROKER_URL]), 70);
}


void defaults (void)
{
    u8 i;

	pgaData[PGA_FLAG] = 0x29;

/*
	pgaData[EE_ID_DISPOSITIVO] = 0xFF;
	pgaData[EE_ID_DISPOSITIVO + 1] = 0xFF;
	pgaData[EE_ID_DISPOSITIVO + 2] = 0xFF;
	pgaData[EE_ID_DISPOSITIVO + 3] = 0xFF;
*/
	// SACARLO EN EL PRODUCTIVO!!!!!!!!!
	
	pgaData[PGA_ID_DISPOSITIVO] = 0x58;
	pgaData[PGA_ID_DISPOSITIVO + 1] = 0x43;
	pgaData[PGA_ID_DISPOSITIVO + 2] = 0xA1;
	pgaData[PGA_ID_DISPOSITIVO + 3] = 0x35;

	pgaData[PGA_NOMBRE_DISP] = '\0';
	pgaData[PGA_SINCRO_INTERNET] = 1;
	pgaData[PGA_CODIGO_REGION] = 1;
	pgaData[PGA_RETARDO_P1] = 8;
	pgaData[PGA_RETARDO_P2] = 8;
	pgaData[PGA_RETARDO_P3] = 8;
	pgaData[PGA_RETARDO_P4] = 8;
	pgaData[PGA_RETARDO_P5] = 8;
	pgaData[PGA_RETARDO_P6] = 8;
	pgaData[PGA_RETARDO_P7] = 8;
	pgaData[PGA_RETARDO_P8] = 8;
	
	pgaData[PGA_MONITOREADA] = 0;
	pgaData[PGA_APP] = 1;
	
	pgaData[PGA_BROKER_PORT] = 8884 & 0xFF;
	pgaData[PGA_BROKER_PORT+1] = (8884 >> 8) & 0xFF;
	
	
	
	/*
	pgaData[PGA_BROKER_URL+0] = 'b';
	pgaData[PGA_BROKER_URL+1] = 'r';
	pgaData[PGA_BROKER_URL+2] = 'o';
	pgaData[PGA_BROKER_URL+3] = 'k';
	pgaData[PGA_BROKER_URL+4] = 'e';
	pgaData[PGA_BROKER_URL+5] = 'r';
	pgaData[PGA_BROKER_URL+6] = '.';
	pgaData[PGA_BROKER_URL+7] = 'c';
	pgaData[PGA_BROKER_URL+8] = 'l';
	pgaData[PGA_BROKER_URL+9] = 'o';
	pgaData[PGA_BROKER_URL+10] = 'u';
	pgaData[PGA_BROKER_URL+11] = 'd';
	pgaData[PGA_BROKER_URL+12] = 'x';
	pgaData[PGA_BROKER_URL+13] = '2';
	pgaData[PGA_BROKER_URL+14] = '8';
	pgaData[PGA_BROKER_URL+15] = '.';
	pgaData[PGA_BROKER_URL+16] = 'c';
	pgaData[PGA_BROKER_URL+17] = 'o';
	pgaData[PGA_BROKER_URL+18] = 'm';
	pgaData[PGA_BROKER_URL+19] = '\0';
	*/
	
	
	
	pgaData[PGA_BROKER_URL+0] = 'w';
	pgaData[PGA_BROKER_URL+1] = 'i';
	pgaData[PGA_BROKER_URL+2] = 'f';
	pgaData[PGA_BROKER_URL+3] = 'i';
	pgaData[PGA_BROKER_URL+4] = 'c';
	pgaData[PGA_BROKER_URL+5] = 'o';
	pgaData[PGA_BROKER_URL+6] = 'm';
	pgaData[PGA_BROKER_URL+7] = '1';
	pgaData[PGA_BROKER_URL+8] = '0';
	pgaData[PGA_BROKER_URL+9] = '0';
	pgaData[PGA_BROKER_URL+10] = '.';
	pgaData[PGA_BROKER_URL+11] = 'd';
	pgaData[PGA_BROKER_URL+12] = 'y';
	pgaData[PGA_BROKER_URL+13] = 'n';
	pgaData[PGA_BROKER_URL+14] = 'd';
	pgaData[PGA_BROKER_URL+15] = 'n';
	pgaData[PGA_BROKER_URL+16] = 's';
	pgaData[PGA_BROKER_URL+17] = '.';
	pgaData[PGA_BROKER_URL+18] = 'o';
	pgaData[PGA_BROKER_URL+19] = 'r';
	pgaData[PGA_BROKER_URL+20] = 'g';
	pgaData[PGA_BROKER_URL+21] = '\0';
	
	
	
	for (int i = 0; i < 16; i++) {
		pgaData[PGA_PROG_HORARIA_1_HORA_ENCENDIDO + PGA_PROG_HORARIA_LARGO * i] = 0;
		pgaData[PGA_PROG_HORARIA_1_HORA_ENCENDIDO + 1  + PGA_PROG_HORARIA_LARGO * i] = 0;
		pgaData[PGA_PROG_HORARIA_1_HORA_APAGADO + PGA_PROG_HORARIA_LARGO * i] = 0;
		pgaData[PGA_PROG_HORARIA_1_HORA_APAGADO + 1 + PGA_PROG_HORARIA_LARGO * i] = 0;
		pgaData[PGA_PROG_HORARIA_1_PARTICION + PGA_PROG_HORARIA_LARGO * i] = 0xff;
		for (int j = 0; j < 5; j++) {
			pgaData[PGA_PROG_HORARIA_1_NODOS + j + PGA_PROG_HORARIA_LARGO * i] = 0xFF;
		}
	}
	
	for (int i = 0; i < 16; i++) {
		pgaData[PGA_FOTOTIMER_1_HORAS + PGA_FOTOTIMER_LARGO * i] = 0;
		pgaData[PGA_FOTOTIMER_1_PARTICION + PGA_FOTOTIMER_LARGO * i] = 0xff;
		for (int j = 0; j < 5; j++) {
			pgaData[PGA_FOTOTIMER_1_NODOS + j + PGA_FOTOTIMER_LARGO * i] = 0xFF;
		}
	}
	
	for (int i = 0; i < 16; i++) {
		pgaData[PGA_NOCHE_1_PARTICION + PGA_NOCHE_LARGO * i] = 0xff;
		for (int j = 0; j < 5; j++) {
			pgaData[PGA_NOCHE_1_NODOS + j + PGA_NOCHE_LARGO * i] = 0xFF;
		}
	}
	
	for (int i = 0; i < 16; i++) {
		pgaData[PGA_SIMULADOR_1_PARTICION + PGA_SIMULADOR_LARGO * i] = 0xff;
		for (int j = 0; j < 5; j++) {
			pgaData[PGA_SIMULADOR_1_NODOS + j + PGA_SIMULADOR_LARGO * i] = 0xFF;
		}
	}
	
	
	// SACARLO EN EL PRODUCTIVO!!!!!!!!!
	
	pgaData[PGA_SERVER_KEY + 0] = 'p';
	pgaData[PGA_SERVER_KEY + 1] = 'r';
	pgaData[PGA_SERVER_KEY + 2] = 'u';
	pgaData[PGA_SERVER_KEY + 3] = 'e';
	pgaData[PGA_SERVER_KEY + 4] = 'b';
	pgaData[PGA_SERVER_KEY + 5] = 'a';
	pgaData[PGA_SERVER_KEY + 6] = '\0';
	pgaData[PGA_SERVER_KEY + 7] = '\0';
	pgaData[PGA_SERVER_KEY + 8] = '\0';
	pgaData[PGA_SERVER_KEY + 9] = '\0';
	pgaData[PGA_SERVER_KEY + 10] = '\0';
	pgaData[PGA_SERVER_KEY + 11] = '\0';
	pgaData[PGA_SERVER_KEY + 12] = '\0';
	pgaData[PGA_SERVER_KEY + 13] = '\0';
	pgaData[PGA_SERVER_KEY + 14] = '\0';
	pgaData[PGA_SERVER_KEY + 15] = '\0';
	
	
	/*
	for (int i = 0; i < PGA_SERVER_KEY_LARGO; i ++)
		pgaData[PGA_SERVER_KEY + i] = 0xFF;
	*/
	
	/*
	for (int i = 0; i < PGA_SERVER_KEY_LARGO; i ++)
		pgaData[PGA_SERVER_KEY + i] = 0x00 + 0x11 * i;
	*/


    pga_enqueueSave(PGA_FLAG, 1, EE_FLAG_ADDR);
    pga_enqueueSave(PGA_NOMBRE_DISP, 30, EE_NOMBRE_DISP_ADDR);
    pga_enqueueSave(PGA_PROG_HORARIA_1_HORA_ENCENDIDO, 160, EE_PROG_HORARIA_1_HORA_ENCENDIDO_ADDR);
	pga_enqueueSave(PGA_FOTOTIMER_1_HORAS, 112, EE_FOTOTIMER_1_HORAS_ADDR);
	pga_enqueueSave(PGA_NOCHE_1_PARTICION, 96, EE_NOCHE_1_PARTICION_ADDR);
	pga_enqueueSave(PGA_SIMULADOR_1_PARTICION, 96, EE_SIMULADOR_1_PARTICION_ADDR);
	pga_enqueueSave(PGA_SERVER_KEY, PGA_SERVER_KEY_LARGO, EE_SERVER_KEY);
	pga_enqueueSave(PGA_MONITOREADA, 2, EE_MONITOREADA);
	pga_enqueueSave(PGA_BROKER_PORT, 2, EE_BROKER_PORT);
	pga_enqueueSave(PGA_BROKER_URL, 70, EE_BROKER_URL);
	
	// Reset de todos los nombres
	fullResetRequested = true;
	
}


void defaultNames (void) 
{

}
