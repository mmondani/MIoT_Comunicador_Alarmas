#include "inc/fsmProg.h"
#include "inc/mpxh.h"
#include "inc/project.h"
#include "inc/basicDefinitions.h"
#include "inc/EEPROM_MAP.h"
#include "inc/PGA_Map.h"
#include "inc/pga.h"
#include "inc/alarmMonitor.h"
#include "inc/displayRAM.h"
#include "inc/imClient_cmds_regs.h"
#include "inc/utilities.h"



static union Ap1{
	uint32_t w;
	struct {
		uint8_t ap_down:1;
		uint8_t ap_up:1;
		uint8_t ap_tecla:1;
		uint8_t ap_p2seg:1;
		uint8_t ap_resp_reset_claves:1;
		uint8_t bit5:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
	} bits;
} ap1;

uint8_t tecla;
uint8_t timer_mandarEnProgramacion;



/**************************************************************/
// FSM para gestionar la programación por MPXH del WIFICOM100
/**************************************************************/
u8 secProg[6] = {
	TECLA_W,
	TECLA_C,
	TECLA_O,
	TECLA_M,
	TECLA_TLCD_P,
	TECLA_TLCD_P
};
uint8_t ptrSecProg = 0;


u8 secResetClaves[4];
u8 secResetFabrica[4];
uint8_t ptrSecReset = 0;

typedef enum    {
	fsmProg_esperoProgCentral = 0,
	fsmProg_esperoSecuenciaWCOM,
	fsmProg_esperoPP,
	fsmProg_basica,
	fsmProg_avanzada,
	fsmProg_avanzada_espero8,
	fsmProg_avanzada_esperoOtro8,
	fsmProg_avanzada_vinoP88,
	fsmProg_vinoXXX,
	fsmProg_vino881,
	fsmProg_vino882,
	fsmProg_vino883,
	fsmProg_vino882_esperaRespuestaServidor,
	fsmProg_vino88x
}fsmProg_state_t;
static fsmProg_state_t fsmState = fsmProg_esperoProgCentral;
static fsmProg_state_t fsmState_previous = fsmProg_esperoProgCentral;

static bool stateIn = true;
static bool stateOut = false;

static uint8_t timerState;
uint8_t optionPointer;

static void gotoState (fsmProg_state_t nextState);

static uint8_t nextOption (void);
static uint8_t prevOption (void);
static void salirDeProgramacion (void);
static void armarComandoResetClaves (void);
 

/**************************************************************/
// Displays para el TLCD
/**************************************************************/
uint8_t* helpDisplay;

//                                          0             1516             31
//                                          |              ||              |
const u8 display_basica[] =                "   WIFICOM100    VERSION   &.&";
const u8 help_basica[] =                   "PRESIONE        P 2 SEGUNDOS";
const u8 display_avanzada[] =              "INGRESE P88     O BIEN du";
const u8 display_881[] =                   "ID WIFICOM100:  &&&&&&&&";
const u8 display_882[] =                   "RESTAURAR       CLAVES?";
const u8 display_883[] =                   "REST. VALORES DEFABRICA?";
const u8 display_882_restaurando[] =       "RESTAURANDO     CLAVES...";
const u8 display_882_ok[] =				   "CLAVES          RESTAURADAS";
const u8 display_882_error[] =		       "ERROR DE        RESTAURACION";
const u8 display_input_883[] =             "VALORES FABRICA RESTAURADOS";




void fsmProg_init(void)
{
	secResetClaves[0] = traductirHexaATeclado((pgaData[PGA_ID_DISPOSITIVO+2] & 0xF0) >> 4);
	secResetClaves[1] = traductirHexaATeclado(pgaData[PGA_ID_DISPOSITIVO+2] & 0x0F);
	secResetClaves[2] = traductirHexaATeclado((pgaData[PGA_ID_DISPOSITIVO+3] & 0xF0) >> 4);
	secResetClaves[3] = traductirHexaATeclado(pgaData[PGA_ID_DISPOSITIVO+3] & 0x0F);
	
	secResetFabrica[0] = traductirHexaATeclado((pgaData[PGA_ID_DISPOSITIVO+2] & 0xF0) >> 4);
	secResetFabrica[1] = traductirHexaATeclado(pgaData[PGA_ID_DISPOSITIVO+2] & 0x0F);
	secResetFabrica[2] = traductirHexaATeclado((pgaData[PGA_ID_DISPOSITIVO+3] & 0xF0) >> 4);
	secResetFabrica[3] = traductirHexaATeclado(pgaData[PGA_ID_DISPOSITIVO+3] & 0x0F);
}


void fsmProg_analizarMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer, uint8_t nbits)
{
	switch (nbits) {
		case MPXH_BITS_17:
			if (dataH == 0) {								// 00x - Tecla
				if (dataL <= 0xb0) {
					ap1.bits.ap_tecla = 1;
					tecla = nibble_swap(dataL);
				}
			}
			else if (dataH == 0x01 && dataL == 0x00)			// 010 - P 2 segundos
				ap1.bits.ap_p2seg = 1;
			else if (dataH == 0x61) {
				if (dataL == 0x00)							// 610 - Tecla arriba
					ap1.bits.ap_up = 1;
				else if (dataL == 0x10)						// 611 - Tecla abajo
					ap1.bits.ap_down = 1;
			}
			break;
			
	}
}


bool fsmProg_analizarIm (imMessage_t* msg)
{
	uint8_t toId[4];
	bool ret = false;
	
	if (msg->cmd == IM_CLIENT_CMD_RESP_RESET_CLAVES) {
		ap1.bits.ap_resp_reset_claves = 1;
		imClient_removeMessageToRead(0);
		ret = true;
	}
	
	return ret;
}


void fsmProg_timers1s_handler (void)
{
	if (timerState < 255)
		timerState ++;
		
	if (fsmState >= fsmProg_basica) {
		timer_mandarEnProgramacion ++;
		
		if (timer_mandarEnProgramacion >= 16) {
			bit_set(mandarEnProgramacion, 0);
			timer_mandarEnProgramacion = 0;
		}
	}
}


bool fsmProg_enProgramacion (void)
{
	return (fsmState > fsmProg_esperoPP);
}


void fsmProg_handler(void)
{	
	
	// Si pasaron más de 32 segundos sin interección, salgo de programación
	if (timerState >= 32 && fsmState >= fsmProg_esperoPP) {
		salirDeProgramacion();
		
		// Saco una F por MPXH
		bufferTeclas[0] = 0x0b;
		ptrTeclas = 0;
		lenTeclas = 1;
	}
	
	// Si la programación actual tiene una ayuda, a los 6 segundos de estar mostrándola
	// muestro la ayuda
	if (helpDisplay != NULL && timerState == 6) {
		
		displayRAM_cargarDR(helpDisplay, 0);
		
		// Para no volver a entrar
		timerState++;
	}
	
	if (fsmState >= fsmProg_basica) {
		if (ap1.bits.ap_up) {
			optionPointer = nextOption();
			gotoState(fsmProg_vinoXXX + optionPointer);
		}
		else if (ap1.bits.ap_down) {
			optionPointer = prevOption();
			gotoState(fsmProg_vinoXXX + optionPointer);
		}
		else if (ap1.bits.ap_tecla && tecla == TECLA_TLCD_F) {
			// Salgo de programación
			salirDeProgramacion();
			
			bit_set(mandarGracias, 0);
		}
	}
	
	if (fsmState >= fsmProg_avanzada) {
		if (ap1.bits.ap_tecla && tecla == TECLA_TLCD_P) {
			gotoState(fsmProg_avanzada);
		}
	}
	
	
	switch(fsmState) {
		case fsmProg_esperoProgCentral:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				helpDisplay = NULL;
			}
			//**********************************************************************************************
			
			if (alarmMonitor_estadoCentral(0) == ESTADO_ALARMA_PROGRAMACION)
				gotoState(fsmProg_esperoSecuenciaWCOM);
			
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
				
				ptrSecProg = 0;
            }
			
			break;
			
			
		case fsmProg_esperoSecuenciaWCOM:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				helpDisplay = NULL;
			}
			//**********************************************************************************************
			
			if (ap1.bits.ap_tecla) {
				gotoState(fsmProg_esperoSecuenciaWCOM);
				
				if (tecla == secProg[ptrSecProg])
					ptrSecProg ++;
				else 
					gotoState(fsmProg_esperoProgCentral);
					
				// Llegó WCOM
				if (ptrSecProg == 4) {
					if (!alarmMonitor_hayPerifericoEnProgramacion(0)) {
						bit_set(mandarPiruPiru, 0);
						
						gotoState(fsmProg_esperoPP);
					}
					else
						ptrSecProg = 0;
				}
			}
			
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
		
		case fsmProg_esperoPP:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				helpDisplay = NULL;
			}
			//**********************************************************************************************
			
			if (ap1.bits.ap_tecla) {
				gotoState(fsmProg_esperoPP);
				
				if (tecla == secProg[ptrSecProg])
					ptrSecProg ++;
				else {
					ptrSecProg = 0;
					gotoState(fsmProg_esperoProgCentral);
				}
				
				// Llegó WCOMPP
				if (ptrSecProg == 6) {
					bit_set(mandarPiruPiru, 0);
					bit_set(mandarProgramacion, 0);
					bit_set(mandarEnProgramacion, 0);
					
					optionPointer = 0;
					alarmMonitor_resetTimerPerifericoEnProgramacion(0);
					gotoState(fsmProg_basica);
				}
			}
			
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
		
		case fsmProg_basica:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				displayRAM_cargarDR(display_basica, 0);
				displayRAM_putU8Hex(27, PROJECT_FIRMWARE_VERSION_MAYOR, false);
				displayRAM_putU8Hex(29, PROJECT_FIRMWARE_VERSION_MENOR, false);
				
				helpDisplay = help_basica;
			}
			//**********************************************************************************************
			if (ap1.bits.ap_tecla && tecla == TECLA_TLCD_P)
				gotoState(fsmProg_basica);
				
			if (ap1.bits.ap_p2seg) {
				bit_set(mandarPiruPiru, 0);
				bit_set(mandarAvanzada, 0);
				
				gotoState(fsmProg_avanzada);
			}
			
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
			
		case fsmProg_avanzada:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				displayRAM_cargarDR(display_avanzada, 0);
				
				helpDisplay = NULL;
			}
			//**********************************************************************************************
			
			if (ap1.bits.ap_tecla) {
				if (tecla == TECLA_TLCD_P)
					gotoState(fsmProg_avanzada_espero8);
				else {
					bit_set(mandarGrave, 0);
					bit_set(mandarError, 0);
					gotoState(fsmProg_avanzada);
				}
			}
			
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
				
			break;
			
			
		case fsmProg_avanzada_espero8:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				displayRAM_cargarDR(display_avanzada, 0);
				helpDisplay = NULL;
			}
			//**********************************************************************************************
			
			if (ap1.bits.ap_tecla) {
				if (tecla == TECLA_8)
					gotoState(fsmProg_avanzada_esperoOtro8);
				else {
					bit_set(mandarGrave, 0);
					bit_set(mandarError, 0);
					gotoState(fsmProg_avanzada);
				}
			}

			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
			
		case fsmProg_avanzada_esperoOtro8:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				helpDisplay = NULL;
			}
			//**********************************************************************************************
			
			if (ap1.bits.ap_tecla) {
				if (tecla == TECLA_8)
					gotoState(fsmProg_avanzada_vinoP88);
				else {
					bit_set(mandarGrave, 0);
					bit_set(mandarError, 0);
					gotoState(fsmProg_avanzada);
				}
			}
			
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
				
			break;
			
			
		case fsmProg_avanzada_vinoP88:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				helpDisplay = NULL;
			}
			//**********************************************************************************************
			
			if (ap1.bits.ap_tecla) {
				if (tecla < TECLA_1 || tecla > TECLA_3) {
					bit_set(mandarGrave, 0);
					bit_set(mandarError, 0);
					gotoState(fsmProg_avanzada);
				}
				else {
					optionPointer = tecla;
					gotoState(fsmProg_vinoXXX + tecla);
				}
			}
			
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
				
			break;
			
		
		case fsmProg_vino881:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				displayRAM_cargarDR(display_881, 0);
				displayRAM_putU8BCD(16, pgaData[PGA_ID_DISPOSITIVO], true);
				displayRAM_putU8BCD(18, pgaData[PGA_ID_DISPOSITIVO + 1], true);
				displayRAM_putU8BCD(20, pgaData[PGA_ID_DISPOSITIVO + 2], true);
				displayRAM_putU8BCD(22, pgaData[PGA_ID_DISPOSITIVO + 3], true);
				
				helpDisplay = NULL;
			}
			//**********************************************************************************************
			
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
		
		
		case fsmProg_vino882:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				displayRAM_cargarDR(display_882, 0);
				
				helpDisplay = NULL;
				ptrSecReset = 0;
			}
			//**********************************************************************************************
			if (ap1.bits.ap_tecla) {
				if (tecla == secResetClaves[ptrSecReset])
					ptrSecReset ++;
				else {
					bit_set(mandarGrave, 0);
					bit_set(mandarError, 0);
					gotoState(fsmProg_avanzada);
				}
	
				// Llegó BORR
				if (ptrSecReset == 4) {
					bit_set(mandarPiruPiru, 0);
					
					armarComandoResetClaves();
					
					gotoState(fsmProg_vino882_esperaRespuestaServidor);
				}
			}
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
			
		case fsmProg_vino882_esperaRespuestaServidor:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				displayRAM_cargarDR(display_882_restaurando, 0);
				
				helpDisplay = NULL;
				ptrSecReset = 0;
			}
			//**********************************************************************************************
			if (ap1.bits.ap_resp_reset_claves) {
				bit_set(mandarPiruPiru, 0);
				displayRAM_cargarDR(display_882_ok, 0);
				
				gotoState(fsmProg_vino88x);
			}
			else if (timerState > 10) {
				// No hubo respuesta del servidor
				bit_set(mandarGrave, 0);
				bit_set(mandarError, 0);
				displayRAM_cargarDR(display_882_error, 0);
				
				gotoState(fsmProg_vino88x);
				
			}
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
			
		case fsmProg_vino883:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				displayRAM_cargarDR(display_883, 0);
				
				helpDisplay = NULL;
				ptrSecReset = 0;
			}
			//**********************************************************************************************
			if (ap1.bits.ap_tecla) {
				if (tecla == secResetFabrica[ptrSecReset])
					ptrSecReset ++;
				else {
					bit_set(mandarGrave, 0);
					bit_set(mandarError, 0);
					gotoState(fsmProg_avanzada);
				}
	
				// Llegó BORR
				if (ptrSecReset == 4) {
					bit_set(mandarPiruPiru, 0);
					
					pga_resetAll();
					
					displayRAM_cargarDR(display_input_883, 0);
					gotoState(fsmProg_vino88x);
				}
			}
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
			
		case fsmProg_vino88x:
			if (stateIn)
			{
				stateIn = false;
				stateOut = false;
				
				helpDisplay = NULL;
			}
			//**********************************************************************************************
			if (timerState >= 5)
				gotoState(fsmProg_avanzada);
			//**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
	}
	
	
	
	
	
	// Clereo APs
	ap1.w = 0;
}


void gotoState (fsmProg_state_t nextState)
{
	fsmState_previous = fsmState;
	fsmState = nextState;
	
	stateIn = false;
	stateOut = true;
	
	timerState = 0;
}


uint8_t nextOption (void)
{
	u8 ret = 0;
	switch (optionPointer)
	{
		case 1: ret = 2; break;             // 881 - 
		case 2: ret = 3; break;             // 882 - 
		case 3: ret = 1; break;             // 883 -
		
		default: ret = 1; break;
	}
	return ret;
}

uint8_t prevOption (void)
{
	u8 ret = 0;
	switch (optionPointer)
	{
		case 1: ret = 3; break;             // 881 - 
		case 2: ret = 1; break;             // 882 -
		case 3: ret = 2; break;             // 883 -
		
		default: ret = 1; break;
	}
	return ret;
}


void salirDeProgramacion (void)
{
	ptrSecProg = 0;
	helpDisplay = NULL;
	alarmMonitor_clearPerifericoEnProgramacion(0);
		
	displayRAM_volver();
		
	gotoState(fsmProg_esperoProgCentral);
}


void armarComandoResetClaves (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_IM_CLIENT, IM_CLIENT_CMD_RESET_CLAVES, 0, 0, 0);
	}
}