#include "inc/project.h"
#include "inc/basicDefinitions.h"
#include "inc/mpxh.h"
#include "inc/nodesManager.h"
#include "inc/alarmMonitor.h"
#include "inc/mainTimer.h"
#include "inc/utilities.h"
#include "inc/ext_eeprom.h"
#include "inc/pga.h"
#include "inc/PGA_Map.h"
#include "inc/EEPROM_MAP.h"
#include "inc/dateTime.h"
#include "inc/imClient.h"
#include "inc/imClient_cmds_regs.h"
#include "inc/MessagePoolManager.h"


#define CANTIDAD_NODOS_TEMPORIZADOS			20


uint32_t estadoNodos[8][4];
uint32_t nodosAPrender[8][4];
uint32_t nodosAApagar[8][4];
uint8_t unidadAutomatizacion;
uint8_t lastNoche;
uint16_t cambioDiaNoche;
uint16_t simular;
uint16_t apagarSimulador;
uint16_t apagarFototimer;
uint16_t apagarNoche;
uint8_t timer_simulador;
uint32_t timer_fototimer;
uint8_t hayQuePrender;
uint8_t hayQueApagar;
uint8_t timer_simulador1[16];
uint8_t timer_simulador2[16];
uint8_t layerProcesandoSetEstadoNodos;
uint8_t timer_mandarEventoEstadoNodos[8];
uint16_t cambiosParaElEventoOGet_programacionHoraria;
uint16_t cambiosParaElEventoOGet_fototimer;
uint16_t cambiosParaElEventoOGet_noche;
uint16_t cambiosParaElEventoOGet_simulador;

uint32_t timersParaMandarPorEvento;

typedef struct {
	uint8_t nodo;
	uint8_t layer;
	uint8_t horas;
	uint8_t minutos;
	uint16_t segundosTotales;
} Timer_t;

Timer_t timers[CANTIDAD_NODOS_TEMPORIZADOS];

static union ProcesandoMensajes{
	uint32_t w;
	struct {
		uint8_t procesandoSetEstadoNodos:1;
		uint8_t procesandoSetResetConfiguracionNodos:1;
		uint8_t procesandoGetConfiguracionNodos:1;
		uint8_t bit3:1;
		uint8_t bit4:1;
		uint8_t bit5:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
	} bits;
} procesandoMensajes;


static void procesarSetEstadoNodos (uint8_t* payload, uint8_t len, uint8_t layer);
static void procesarGetEstadoNodos (uint8_t layer);
static void armarPayloadGetEstadoNodos (imMessage_t* msg, uint8_t layer);
static void procesarResetEstadoNodos (uint8_t layer);
static void armarEventoEstadoNodos (uint8_t layer);
static void procesarSetConfiguracionNodos (uint8_t* payload, uint8_t len, uint8_t layer);
static void armarEventoConfiguracionNodos (void);
static void procesarResetConfiguracionNodos (uint8_t* payload, uint8_t len, uint8_t layer);
static void procesarGetConfiguracionNodos (uint8_t layer);
static void armarGetConfiguracionNodos (void);
static void procesarSetNodoTemporizado (uint8_t* payload, uint8_t len, uint8_t layer);
static void armarEventoNodoTemporizado (void);
static void procesarGetNodoTemporizado (uint8_t layer);

static void procesarApagarTodo (uint8_t layer);
static void procesarEncenderApagarNodo (uint8_t dataH, uint8_t dataL, uint8_t layer);
static void prenderNodo (uint8_t nodo, uint8_t layer);
static void apagarNodo (uint8_t nodo, uint8_t layer);




void nodesManager_init (void)
{
	for (uint8_t i = 0; i < 8; i++) {
		for (uint8_t j = 0; j < 4; j++) {
			estadoNodos[i][j] = 0;
			nodosAApagar[i][j] = 0;
			nodosAPrender[i][j] = 0;
		}
	}
	
	for (uint8_t i = 0; i < CANTIDAD_NODOS_TEMPORIZADOS; i++) {
		timers[i].layer = 0xff;
	}
}


void nodesManager_analizarMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer, uint8_t nbits)
{
	switch (nbits) {
		case MPXH_BITS_17:
		case MPXH_BITS_16:
		
		if (dataH == 0x0a && dataL == 0xd0) 						// 0AD - Apagar todo
		procesarApagarTodo (layer);
		else if (dataH >= 0x50 && dataH <= 0x57)					// 5xx - Encender nodo
		procesarEncenderApagarNodo (dataH, dataL, layer);
		else if (dataH >= 0x70 && dataH <= 0x77)					// 7xx - Apagar nodo
		procesarEncenderApagarNodo (dataH, dataL, layer);
		
		break;
		
	}
}


void nodesManager_handler (void)
{
	uint8_t nodo;
	uint32_t aux;
	uint8_t noche;
	uint32_t random1, random2;
	bool apagar;
	uint8_t layer;
	
	
	// Cada vez que llamo a este handler (cada 4ms), analizo un timer/fototimer/noche/simulador
	// de los 16 que hay
	unidadAutomatizacion++;
	if (unidadAutomatizacion >= 16)
	unidadAutomatizacion = 0;

	
	noche = (alarmMonitor_esDia() == 0)? 1 : 0;
	random1 = maintTimer_getRandom1();
	random2 = maintTimer_getRandom2();
	
	
	if (noche != lastNoche) {
		lastNoche = noche;
		cambioDiaNoche = 0xFFFF;
	}
	
	if (cambioDiaNoche != 0) {
		bit_clear(cambioDiaNoche, unidadAutomatizacion);
		
		if (lastNoche == 1) {
			// Era de día y se hizo de noche
			
			/************************** SIMULADOR / FOTOTIMER / NOCHE **********************************/
			timer_simulador = 150;				// Equivale a 5 horas
			
			for (uint8_t j = 0; j < 5; j++) {
				layer = pgaData[PGA_SIMULADOR_1_PARTICION + PGA_SIMULADOR_LARGO * unidadAutomatizacion];
				
				if (layer != 0xff) {
					if (alarmMonitor_estadoCentral(layer) == ESTADO_ALARMA_ACT_ME_VOY) {
						if (pgaData[PGA_SIMULADOR_1_NODOS + PGA_SIMULADOR_LARGO * unidadAutomatizacion + j] != 0xFF) {
							if (bit_test(random1 | random2, j))
							prenderNodo (pgaData[PGA_SIMULADOR_1_NODOS + j], layer);
							
							bit_set(simular, layer);
						}
					}
				}
				
				layer = pgaData[PGA_NOCHE_1_PARTICION + PGA_NOCHE_LARGO * unidadAutomatizacion];
				if (layer != 0xff)
				prenderNodo(pgaData[PGA_NOCHE_1_NODOS + PGA_NOCHE_LARGO * unidadAutomatizacion + j], layer);
				
				layer = pgaData[PGA_FOTOTIMER_1_PARTICION + PGA_FOTOTIMER_LARGO * unidadAutomatizacion];
				if (layer != 0xff)
				prenderNodo(pgaData[PGA_FOTOTIMER_1_NODOS + PGA_FOTOTIMER_LARGO * unidadAutomatizacion + j], layer);
			}
			
			timer_fototimer = 0;
		}
		else {
			// Era de noche y se hizo de día
			
			/************************** SIMULADOR / FOTOTIMER / NOCHE **********************************/
			for (uint8_t j = 0; j < 5; j++) {
				layer = pgaData[PGA_SIMULADOR_1_PARTICION + PGA_SIMULADOR_LARGO * unidadAutomatizacion];
				
				if (layer != 0xff) {
					if (bit_test(simular, unidadAutomatizacion))
					bit_set(apagarSimulador, unidadAutomatizacion);
					
					bit_clear(simular, unidadAutomatizacion);
				}
				
				layer = pgaData[PGA_NOCHE_1_PARTICION + PGA_NOCHE_LARGO * unidadAutomatizacion];
				if (layer != 0xff)
				bit_set(apagarNoche, unidadAutomatizacion);
				
				layer = pgaData[PGA_FOTOTIMER_1_PARTICION + PGA_FOTOTIMER_LARGO * unidadAutomatizacion];
				if (layer != 0xff)
				bit_set(apagarFototimer, unidadAutomatizacion);
			}
			
			timer_fototimer = 960;
			timer_simulador = 0;
		}
	}
	
	
	/************************** SIMULADOR **********************************/
	layer = pgaData[PGA_SIMULADOR_1_PARTICION + 6 * unidadAutomatizacion];
	
	if (layer != 0xff && bit_test(simular, unidadAutomatizacion)) {
		if (timer_simulador1[unidadAutomatizacion] == 0) {
			// Entra cada 128s promedio
			aux = 0;
			bit_set(aux, layer);
			timer_simulador1[unidadAutomatizacion] = (random1 ^ aux) & 0xff;
			
			timer_simulador2[unidadAutomatizacion] ++;
			if (timer_simulador2[unidadAutomatizacion] % 4 == 0) {
				// Entra cada 8.5 minutos promedio
				if (noche && alarmMonitor_estadoCentral(layer) == ESTADO_ALARMA_ACT_ME_VOY) {
					if (random1 % 2 == 0) {
						// Sigue cada 17 minutos promedio
						if (timer_simulador < 60) {
							if (bit_test(random1, 2) && bit_test(random2, 3))
							apagar = false;
							else
							apagar = true;
						}
						else {
							if (bit_test(random1, 2))
							apagar = false;
							else
							apagar = true;
						}
						
						aux = random1 ^ random2;
						for (uint8_t j = 0; j < 5; j++) {
							if (bit_test(aux, j)) {
								nodo = pgaData[PGA_SIMULADOR_1_NODOS + 6 * unidadAutomatizacion + j];
								
								if (apagar)
								apagarNodo(nodo, layer);
								else
								prenderNodo(nodo, layer);
							}
						}
					}
				}
			}
		}
	}
	
	
	/*********************** APAGA NODOS SIMULADOR ***************************/
	layer = pgaData[PGA_SIMULADOR_1_PARTICION + PGA_SIMULADOR_LARGO * unidadAutomatizacion];
	
	if (layer != 0xff) {
		if (bit_test(apagarSimulador, unidadAutomatizacion)) {
			bit_clear(apagarSimulador, unidadAutomatizacion);
			
			for (uint8_t j = 0; j < 5; j++)
			apagarNodo(pgaData[PGA_SIMULADOR_1_NODOS + PGA_SIMULADOR_LARGO * unidadAutomatizacion + j], layer);
		}
	}
	
	
	/*********************** APAGA NODOS FOTOTIMER ***************************/
	layer = pgaData[PGA_FOTOTIMER_1_PARTICION + PGA_FOTOTIMER_LARGO * unidadAutomatizacion];
	
	if (layer != 0xff) {
		if (bit_test(apagarFototimer, unidadAutomatizacion)) {
			bit_clear(apagarFototimer, unidadAutomatizacion);
			
			for (uint8_t j = 0; j < 5; j++)
			apagarNodo(pgaData[PGA_FOTOTIMER_1_NODOS + PGA_FOTOTIMER_LARGO * unidadAutomatizacion + j], layer);
		}
	}
	
	
	/*********************** APAGA NODOS NOCHE ***************************/
	layer = pgaData[PGA_NOCHE_1_PARTICION + PGA_NOCHE_LARGO * unidadAutomatizacion];
	
	if (layer != 0xff) {
		if (bit_test(apagarNoche, unidadAutomatizacion)) {
			bit_clear(apagarNoche, unidadAutomatizacion);
			
			for (uint8_t j = 0; j < 5; j++)
			apagarNodo(pgaData[PGA_NOCHE_1_PARTICION + PGA_NOCHE_LARGO * unidadAutomatizacion + j], layer);
		}
	}
	
	
	/*********************** DEPURAR DE NODOS ***************************/
	layer = unidadAutomatizacion / 2;
	
	for (uint8_t j = 0; j < 4; j++) {
		// Prender y apagar se anulan entre sí
		aux = nodosAPrender[layer][j] & nodosAApagar[layer][j];
		aux = ~aux;
		nodosAPrender[layer][j] &= aux;
		nodosAApagar[layer][j] &= aux;
		
		// No apaga lo que está apagado ni prende lo que está prendido
		aux = estadoNodos[layer][j];
		nodosAApagar[layer][j] &= aux;
		nodosAPrender[layer][j] &= ~aux;
		
		if (nodosAApagar[layer][j] != 0)
		bit_set(hayQueApagar, layer);
		
		if (nodosAPrender[layer][j] != 0)
		bit_set(hayQuePrender, layer);
	}
	
	
	if (layer == layerProcesandoSetEstadoNodos && !nodesManager_hayQueMandarPorMpxh()) {
		if (procesandoMensajes.bits.procesandoSetEstadoNodos) {
			procesandoMensajes.bits.procesandoSetEstadoNodos = 0;
		}
	}
}


void nodesManager_timers1s_handler (void)
{
	for (uint8_t i = 0; i < 16; i++) {
		timer_simulador1[i] ++;
	}
	
	for (uint8_t i = 0; i < 8; i++) {
		if (timer_mandarEventoEstadoNodos[i] > 0) {
			timer_mandarEventoEstadoNodos[i] --;
			
			if (timer_mandarEventoEstadoNodos[i] == 0)
			armarEventoEstadoNodos(i);
		}
	}
	
	
	for (uint8_t i = 0; i < CANTIDAD_NODOS_TEMPORIZADOS; i++) {
		if (timers[i].layer != 0xff) {
			if (timers[i].segundosTotales > 0) {
				timers[i].segundosTotales --;
				
				if (timers[i].segundosTotales == 0) {
					apagarNodo(timers[i].nodo, timers[i].layer);
					timers[i].layer = 0xff;
				}
			}
		}
	}
	
}


void nodesManager_timers1m_handler (void)
{
	dateTime_t now;
	uint32_t timer;
	
	
	dateTime_now (&now);
	
	// Cada 2 minutos
	if (timer_simulador != 0 && ((now.minutos % 2) == 0)) {
		timer_simulador --;
		
		if (timer_simulador == 0) {
			// Apago todos los simuladores
			apagarSimulador = simular;
			simular = 0;
		}
	}
	
	// Se incrementa hasta 960 (16 horas)
	if(timer_fototimer < 960)
	timer_fototimer ++;
	
	
	for (uint8_t i = 0; i < 16; i ++) {
		// Fototimer
		timer = pgaData[PGA_FOTOTIMER_1_HORAS + 7 * i] * 60;
		
		if (timer_fototimer == timer)
		bit_set(apagarFototimer, i);
		
		
		// Programación horaria
		if (pgaData[PGA_PROG_HORARIA_1_HORA_ENCENDIDO + 10 * i] == now.horas &&
		pgaData[PGA_PROG_HORARIA_1_HORA_ENCENDIDO + 10 * i + 1] == now.minutos) {
			
			for (uint8_t j = 0; j < 5; j++)
			prenderNodo(pgaData[PGA_PROG_HORARIA_1_NODOS + 10 * i + j], pgaData[PGA_PROG_HORARIA_1_PARTICION + 10 * i]);
		}
		
		if (pgaData[PGA_PROG_HORARIA_1_HORA_APAGADO + 10 * i] == now.horas &&
		pgaData[PGA_PROG_HORARIA_1_HORA_APAGADO + 10 * i + 1] == now.minutos) {
			
			for (uint8_t j = 0; j < 5; j++)
			apagarNodo(pgaData[PGA_PROG_HORARIA_1_NODOS + 10 * i + j], pgaData[PGA_PROG_HORARIA_1_PARTICION + 10 * i]);
		}
	}
}


bool nodesManager_hayQueMandarPorMpxh (void)
{
	return (hayQuePrender != 0 || hayQueApagar != 0);
}


void nodesManager_mandarPorMpxh (void)
{
	uint8_t layer;
	uint8_t grupo;
	uint8_t bit;
	uint8_t nodo;
	uint8_t wsendH, wsendL;
	bool hayQueMandar = false;
	
	
	if (hayQuePrender != 0) {
		layer = sacarLayer(hayQuePrender);
		bit_clear(hayQuePrender, layer);
		
		for (uint8_t i = 0; i < 4 && !hayQueMandar; i++) {
			for (uint8_t j = 0; j < 32 && !hayQueMandar; j++) {
				if (bit_test(nodosAPrender[layer][i], j)) {
					bit_clear(nodosAPrender[layer][i], j);
					
					nodo = i * 32 + j;
					wsendH = 0x50;
					wsendH |= ((nibble_swap(nodo)) & 0x0F);
					wsendL = (nibble_swap(nodo)) & 0xF0;
					
					// TO-DO: esto no está bien. Tendría que recibirme a mi mismo de MPXH
					// para ver qué lo mando.
					bit_set(estadoNodos[layer][i], j);
					
					hayQueMandar = true;
				}
			}
		}
	}
	else if (hayQueApagar != 0) {
		layer = sacarLayer(hayQueApagar);
		bit_clear(hayQueApagar, layer);
		
		for (uint8_t i = 0; i < 4 && !hayQueMandar; i++) {
			for (uint8_t j = 0; j < 32 && !hayQueMandar; j++) {
				if (bit_test(nodosAApagar[layer][i], j)) {
					bit_clear(nodosAApagar[layer][i], j);
					
					nodo = i * 32 + j;
					wsendH = 0x70;
					wsendH |= ((nibble_swap(nodo)) & 0x0F);
					wsendL = (nibble_swap(nodo)) & 0xF0;
					
					// TO-DO: esto no está bien. Tendría que recibirme a mi mismo de MPXH
					// para ver qué lo mando.
					bit_clear(estadoNodos[layer][i], j);
					
					hayQueMandar = true;
				}
			}
		}
	}
	
	if (hayQueMandar) {
		timer_mandarEventoEstadoNodos[layer] = 2;
		
		if (layer == 0)
		mpxh_ArmaMensaje(wsendH, wsendL, 0, MPXH_BITS_17);
		else
		mpxh_ArmaMensaje(wsendH, wsendL, layer, MPXH_BITS_16);
	}
}


bool nodesManager_analizarIm (imMessage_t* msg)
{
	bool ret = false;

	if (msg->cmd == IM_CLIENT_CMD_GET) {
		if (msg->reg == IM_CLIENT_REG_ESTADO_NODOS) {
			procesarGetEstadoNodos(msg->part);
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_CONFIGURACION_NODOS) {
			procesarGetConfiguracionNodos(msg->part);
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_NODO_TEMPORIZADO) {
			procesarGetNodoTemporizado(msg->part);
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
	}
	else if (msg->cmd == IM_CLIENT_CMD_SET) {
		if (msg->reg == IM_CLIENT_REG_ESTADO_NODOS) {
			procesarSetEstadoNodos(msg->payload, msg->len, msg->part);
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_CONFIGURACION_NODOS) {
			procesarSetConfiguracionNodos(msg->payload, msg->len, msg->part);
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_NODO_TEMPORIZADO) {
			procesarSetNodoTemporizado(msg->payload, msg->len, msg->part);
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
	}
	else if (msg->cmd == IM_CLIENT_CMD_RESET) {
		if (msg->reg == IM_CLIENT_REG_ESTADO_NODOS) {
			procesarResetEstadoNodos(msg->part);
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_CONFIGURACION_NODOS) {
			procesarResetConfiguracionNodos(msg->payload, msg->len, msg->part);
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
	}
	
	
	return ret;
}


void nodesManager_determinarFinProcesamiento(void)
{
	if (procesandoMensajes.bits.procesandoSetResetConfiguracionNodos) {
		if (pga_isQueueEmpty()) {
			if (cambiosParaElEventoOGet_programacionHoraria != 0 ||
			cambiosParaElEventoOGet_fototimer != 0 ||
			cambiosParaElEventoOGet_noche != 0 ||
			cambiosParaElEventoOGet_simulador != 0)
			armarEventoConfiguracionNodos();
			else
			procesandoMensajes.bits.procesandoSetResetConfiguracionNodos = 0;
		}
	}
	else if (procesandoMensajes.bits.procesandoGetConfiguracionNodos) {
		if (cambiosParaElEventoOGet_programacionHoraria != 0 ||
		cambiosParaElEventoOGet_fototimer != 0 ||
		cambiosParaElEventoOGet_noche != 0 ||
		cambiosParaElEventoOGet_simulador != 0)
		armarGetConfiguracionNodos();
		else
		procesandoMensajes.bits.procesandoGetConfiguracionNodos = 0;
	}
}


bool nodesManager_procesandoMensaje(void)
{
	return (procesandoMensajes.w != 0);
}


void procesarSetEstadoNodos (uint8_t* payload, uint8_t len, uint8_t layer)
{
	uint8_t nodo, estado;
	
	
	for (uint8_t i = 0; (i+1) < len; i++) {
		nodo = payload[i];
		i++;
		estado = payload[i];
		
		if (estado == 0) {
			apagarNodo(nodo, layer);
			
			// Si estaba temporizado, se apaga el timer.
			for (uint8_t i = 0; i < CANTIDAD_NODOS_TEMPORIZADOS; i++) {
				if (timers[i].layer != 0xff && timers[i].nodo == nodo) {
					timers[i].layer = 0xff;
				}
			}
		}
		else if (estado == 1)
		prenderNodo(nodo, layer);
		
		
		procesandoMensajes.bits.procesandoSetEstadoNodos = 1;
		layerProcesandoSetEstadoNodos = layer;
	}
}


void procesarGetEstadoNodos (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();


	if (msg != NULL) {
		
		armarPayloadGetEstadoNodos(msg, layer);

		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_ESTADO_NODOS, layer, 0);
	}
}


void armarPayloadGetEstadoNodos (imMessage_t* msg, uint8_t layer)
{
	imClient_putPayloadBytes(msg, (uint8_t*)(&(estadoNodos[layer][0])), 4);
	imClient_putPayloadBytes(msg, (uint8_t*)(&(estadoNodos[layer][1])), 4);
	imClient_putPayloadBytes(msg, (uint8_t*)(&(estadoNodos[layer][2])), 4);
	imClient_putPayloadBytes(msg, (uint8_t*)(&(estadoNodos[layer][3])), 4);
}


void procesarResetEstadoNodos (uint8_t layer)
{
	bit_set(mandarApagarTodo, layer);
	procesarApagarTodo(layer);
}


void armarEventoEstadoNodos (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();


	if (msg != NULL) {
		
		armarPayloadGetEstadoNodos(msg, layer);

		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_ESTADO_NODOS, layer, 0);
	}
}


void procesarSetConfiguracionNodos (uint8_t* payload, uint8_t len, uint8_t layer)
{
	uint8_t i;
	uint8_t numero;
	uint8_t tipoAutomatizacion;
	uint8_t layerAuto;
	uint8_t aux1, aux2, aux3, aux4;
	uint32_t baseAddress, offset;
	
	
	if (len < 2)
	return;
	
	i = 0;
	while (i < len) {
		numero = payload[i];
		i++;
		
		// Si el número de unidad de automatización es mayor a 16 se descarta todo el mensaje
		if (numero > 16)
		return;
		
		tipoAutomatizacion = payload[i];
		i++;
		
		if (tipoAutomatizacion == 1 && (i+9) < len) {											// Programación horaria
			layerAuto = payload[i] - 1;
			i++;
			
			aux1 = payload[i];		// Hora inicio
			i++;
			
			aux2 = payload[i];		// Minutos inicio
			i++;
			
			aux3 = payload[i];		// Hora fin
			i++;
			
			aux4 = payload[i];		// Minutos fin
			i++;
			
			offset = (numero - 1) * PGA_PROG_HORARIA_LARGO;
			
			pgaData[PGA_PROG_HORARIA_1_HORA_ENCENDIDO + offset] = aux1;
			pgaData[PGA_PROG_HORARIA_1_HORA_ENCENDIDO + offset + 1] = aux2;
			pgaData[PGA_PROG_HORARIA_1_HORA_APAGADO + offset] = aux3;
			pgaData[PGA_PROG_HORARIA_1_HORA_APAGADO + offset + 1] = aux4;
			pgaData[PGA_PROG_HORARIA_1_PARTICION + offset] = layerAuto;
			
			for (uint8_t j = 0; j < 5; j++, i++) {
				pgaData[PGA_PROG_HORARIA_1_NODOS + offset + j] = payload[i];
			}
			
			pga_enqueueSave(PGA_PROG_HORARIA_1_HORA_ENCENDIDO + offset, PGA_PROG_HORARIA_LARGO, EE_PROG_HORARIA_1_HORA_ENCENDIDO_ADDR + offset);
			
			bit_set(cambiosParaElEventoOGet_programacionHoraria, numero - 1);
		}
		else if (tipoAutomatizacion == 2 && (i+6) < len) {										// Fototimer
			layerAuto = payload[i] - 1;
			i++;

			aux1 = payload[i];		// Horas
			i++;

			offset = (numero - 1) * PGA_FOTOTIMER_LARGO;

			pgaData[PGA_FOTOTIMER_1_HORAS + offset] = aux1;
			pgaData[PGA_FOTOTIMER_1_PARTICION + offset] = layerAuto;
			
			for (uint8_t j = 0; j < 5; j++, i++) {
				pgaData[PGA_FOTOTIMER_1_NODOS + offset + j] = payload[i];
			}
			
			
			pga_enqueueSave(PGA_FOTOTIMER_1_HORAS + offset, PGA_FOTOTIMER_LARGO, EE_FOTOTIMER_1_HORAS_ADDR + offset);
			
			bit_set(cambiosParaElEventoOGet_fototimer, numero - 1);
		}
		else if ((tipoAutomatizacion == 3 || tipoAutomatizacion == 4) && (i+5) < len) {			// Noche y Simulador
			layerAuto = payload[i] - 1;
			i++;

			offset = (numero - 1) * PGA_NOCHE_LARGO;

			if (tipoAutomatizacion == 3)
			baseAddress = PGA_NOCHE_1_PARTICION;
			else
			baseAddress = PGA_SIMULADOR_1_PARTICION;
			

			pgaData[baseAddress + offset] = layerAuto;
			
			for (uint8_t j = 0; j < 5; j++, i++) {
				pgaData[baseAddress + 1 + offset + j] = payload[i];
			}
			
			if (tipoAutomatizacion == 3) {
				pga_enqueueSave(PGA_NOCHE_1_PARTICION + offset, PGA_NOCHE_LARGO, EE_NOCHE_1_PARTICION_ADDR + offset);
				bit_set(cambiosParaElEventoOGet_noche, numero - 1);
			}
			else {
				pga_enqueueSave(PGA_SIMULADOR_1_PARTICION + offset, PGA_SIMULADOR_LARGO, EE_SIMULADOR_1_PARTICION_ADDR + offset);
				bit_set(cambiosParaElEventoOGet_simulador, numero - 1);
			}
		}
		else {
			return;
		}
	}
	
	
	if (cambiosParaElEventoOGet_programacionHoraria != 0 ||
	cambiosParaElEventoOGet_fototimer != 0 ||
	cambiosParaElEventoOGet_noche != 0 ||
	cambiosParaElEventoOGet_simulador != 0) {
		procesandoMensajes.bits.procesandoSetResetConfiguracionNodos = 1;
	}
}


void armarEventoConfiguracionNodos (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	uint32_t baseAddress, offset;
	uint8_t buffAux[PGA_PROG_HORARIA_LARGO];		// Es el más largo de los 4 tipos de unidades de automatización
	uint8_t bytes;
	

	if (msg != NULL) {
		
		bytes = 0;
		
		if (cambiosParaElEventoOGet_programacionHoraria != 0) {
			for (uint8_t i = 0; i < 16 && (bytes + 12) <= MESSAGE_POOL_MESSAGE_MAX_LEN; i++) {
				if (bit_test(cambiosParaElEventoOGet_programacionHoraria, i)) {
					bit_clear(cambiosParaElEventoOGet_programacionHoraria, i);
					
					offset = i * PGA_PROG_HORARIA_LARGO;
					
					ext_eeprom_read_bytes(EE_PROG_HORARIA_1_HORA_ENCENDIDO_ADDR + offset, buffAux, PGA_PROG_HORARIA_LARGO);
					
					imClient_putPayloadByte(msg, i + 1);
					imClient_putPayloadByte(msg, 1);

					// Si el layer es 0xff se manda eso en el mensaje, sino se le suma uno para mandar la partición
					if (buffAux[4] == 0xff)
					imClient_putPayloadByte(msg, 0xff);
					else
					imClient_putPayloadByte(msg, buffAux[4] + 1);

					imClient_putPayloadBytes(msg, &(buffAux[0]), 4);			// Hora encendido y apagado
					imClient_putPayloadBytes(msg, &(buffAux[5]), 5);			// Nodos
					
					bytes += 12;
				}
			}
		}
		
		if (cambiosParaElEventoOGet_fototimer != 0) {
			for (uint8_t i = 0; i < 16 && (bytes + 9) <= MESSAGE_POOL_MESSAGE_MAX_LEN; i++) {
				if (bit_test(cambiosParaElEventoOGet_fototimer, i)) {
					bit_clear(cambiosParaElEventoOGet_fototimer, i);
					
					offset = i * PGA_FOTOTIMER_LARGO;
					
					ext_eeprom_read_bytes(EE_FOTOTIMER_1_HORAS_ADDR + offset, buffAux, PGA_FOTOTIMER_LARGO);
					
					imClient_putPayloadByte(msg, i + 1);
					imClient_putPayloadByte(msg, 2);

					// Si el layer es 0xff se manda eso en el mensaje, sino se le suma uno para mandar la partición
					if (buffAux[1] == 0xff)
					imClient_putPayloadByte(msg, 0xff);
					else
					imClient_putPayloadByte(msg, buffAux[1] + 1);

					imClient_putPayloadByte(msg, buffAux[0]);					// Horas
					imClient_putPayloadBytes(msg, &(buffAux[2]), 5);			// Nodos
					
					bytes += 9;
				}
			}
		}
		
		if (cambiosParaElEventoOGet_noche != 0) {
			for (uint8_t i = 0; i < 16 && (bytes + 8) <= MESSAGE_POOL_MESSAGE_MAX_LEN; i++) {
				if (bit_test(cambiosParaElEventoOGet_noche, i)) {
					bit_clear(cambiosParaElEventoOGet_noche, i);
					
					offset = i * PGA_NOCHE_LARGO;
					
					ext_eeprom_read_bytes(EE_NOCHE_1_PARTICION_ADDR + offset, buffAux, PGA_NOCHE_LARGO);
					
					imClient_putPayloadByte(msg, i + 1);
					imClient_putPayloadByte(msg, 3);

					// Si el layer es 0xff se manda eso en el mensaje, sino se le suma uno para mandar la partición
					if (buffAux[0] == 0xff)
					imClient_putPayloadByte(msg, 0xff);
					else
					imClient_putPayloadByte(msg, buffAux[0] + 1);

					imClient_putPayloadBytes(msg, &(buffAux[1]), 5);			// Nodos
					
					bytes += 8;
				}
			}
		}
		
		if (cambiosParaElEventoOGet_simulador != 0) {
			for (uint8_t i = 0; i < 16 && (bytes + 8) <= MESSAGE_POOL_MESSAGE_MAX_LEN; i++) {
				if (bit_test(cambiosParaElEventoOGet_simulador, i)) {
					bit_clear(cambiosParaElEventoOGet_simulador, i);
					
					offset = i * PGA_SIMULADOR_LARGO;
					
					ext_eeprom_read_bytes(EE_SIMULADOR_1_PARTICION_ADDR + offset, buffAux, PGA_SIMULADOR_LARGO);
					
					imClient_putPayloadByte(msg, i + 1);
					imClient_putPayloadByte(msg, 4);

					// Si el layer es 0xff se manda eso en el mensaje, sino se le suma uno para mandar la partición
					if (buffAux[0] == 0xff)
					imClient_putPayloadByte(msg, 0xff);
					else
					imClient_putPayloadByte(msg, buffAux[0] + 1);

					imClient_putPayloadBytes(msg, &(buffAux[1]), 5);			// Nodos
					
					bytes += 8;
				}
			}
		}
		

		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_CONFIGURACION_NODOS, 0, 0);
	}
}


void procesarResetConfiguracionNodos (uint8_t* payload, uint8_t len, uint8_t layer)
{
	uint8_t i;
	uint8_t numero;
	uint8_t tipoAutomatizacion;
	uint32_t baseAddress, largo, offset;
	uint32_t eepromBaseAddress;
	
	
	if (len < 2)
	return;
	
	i = 0;
	while ((i+1) < len) {
		numero = payload[i];			// De 1 a 16
		i++;
		
		// Si el número de unidad de automatización es mayor a 16 se descarta todo el mensaje
		if (numero > 16)
		return;
		
		
		tipoAutomatizacion = payload[i];
		i++;
		
		if (tipoAutomatizacion < 1 || tipoAutomatizacion > 4)
		return;
		

		if (tipoAutomatizacion == 1) {												// Programación horaria
			baseAddress = PGA_PROG_HORARIA_1_HORA_ENCENDIDO;
			largo = PGA_PROG_HORARIA_LARGO;
			eepromBaseAddress = EE_PROG_HORARIA_1_HORA_ENCENDIDO_ADDR;
			
			bit_set(cambiosParaElEventoOGet_programacionHoraria, numero - 1);
		}
		else if (tipoAutomatizacion == 2) {											// Fototimer
			baseAddress = PGA_FOTOTIMER_1_HORAS;
			largo = PGA_FOTOTIMER_LARGO;
			eepromBaseAddress = EE_FOTOTIMER_1_HORAS_ADDR;
			
			bit_set(cambiosParaElEventoOGet_fototimer, numero - 1);
		}
		else if (tipoAutomatizacion == 3) {											// Noche
			baseAddress = PGA_NOCHE_1_PARTICION;
			largo = PGA_NOCHE_LARGO;
			eepromBaseAddress = EE_NOCHE_1_PARTICION_ADDR;
			
			bit_set(cambiosParaElEventoOGet_noche, numero - 1);
		}
		else if (tipoAutomatizacion == 4) {											// Simulador
			baseAddress = PGA_SIMULADOR_1_PARTICION;
			largo = PGA_SIMULADOR_LARGO;
			eepromBaseAddress = EE_SIMULADOR_1_PARTICION_ADDR;
			
			bit_set(cambiosParaElEventoOGet_simulador, numero - 1);
		}
		
		
		offset = largo * (numero - 1);
		
		for (uint8_t i = 0; i < largo; i++) {
			pgaData[baseAddress + offset + i] = 0xff;
		}
		
		pga_enqueueSave(baseAddress + offset, largo, eepromBaseAddress + offset);
	}
	
	
	if (cambiosParaElEventoOGet_programacionHoraria != 0 ||
	cambiosParaElEventoOGet_fototimer != 0 ||
	cambiosParaElEventoOGet_noche != 0 ||
	cambiosParaElEventoOGet_simulador != 0) {
		procesandoMensajes.bits.procesandoSetResetConfiguracionNodos = 1;
	}
}


void procesarGetConfiguracionNodos (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	uint32_t offset;
	
	
	for (uint8_t tipoAutomatizacion = 1; tipoAutomatizacion <= 4; tipoAutomatizacion++) {
		
		if (tipoAutomatizacion == 1) {											// Programación horaria
			for (uint8_t i = 0; i < 16; i++) {
				offset = i * PGA_PROG_HORARIA_LARGO;
				
				if (pgaData[PGA_PROG_HORARIA_1_PARTICION + offset] != 0xff) {
					bit_set(cambiosParaElEventoOGet_programacionHoraria, i);
				}
			}
		}
		else if (tipoAutomatizacion == 2) {										// Fototimer
			for (uint8_t i = 0; i < 16; i++) {
				offset = i * PGA_FOTOTIMER_LARGO;
				
				if (pgaData[PGA_FOTOTIMER_1_PARTICION + offset] != 0xff) {
					bit_set(cambiosParaElEventoOGet_fototimer, i);
				}
			}
		}
		else if (tipoAutomatizacion == 3) {										// Noche
			for (uint8_t i = 0; i < 16; i++) {
				offset = i * PGA_NOCHE_LARGO;
				
				if (pgaData[PGA_NOCHE_1_PARTICION + offset] != 0xff) {
					bit_set(cambiosParaElEventoOGet_noche, i);
				}
			}
		}
		else if (tipoAutomatizacion == 4) {										// Simulador
			for (uint8_t i = 0; i < 16; i++) {
				offset = i * PGA_SIMULADOR_LARGO;
				
				if (pgaData[PGA_SIMULADOR_1_PARTICION + offset] != 0xff) {
					bit_set(cambiosParaElEventoOGet_simulador, i);
				}
			}
		}
	}
	
	if (cambiosParaElEventoOGet_programacionHoraria != 0 ||
	cambiosParaElEventoOGet_fototimer != 0 ||
	cambiosParaElEventoOGet_noche != 0 ||
	cambiosParaElEventoOGet_simulador != 0)
	procesandoMensajes.bits.procesandoGetConfiguracionNodos = 1;
	else {
		// Si no hay nada para mandar, mando el mensaje vacío
		if (msg != NULL) {
			toId[0] = pgaData[PGA_ID_DISPOSITIVO];
			toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
			toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
			toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
			imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_CONFIGURACION_NODOS, 0, 0);
		}
	}
}


void armarGetConfiguracionNodos (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	uint32_t offset;
	uint8_t bytes;
	
	
	if (msg != NULL) {
		
		bytes = 0;

		if (cambiosParaElEventoOGet_programacionHoraria != 0) {
			for (uint8_t i = 0; i < 16  && (bytes + 12) <= MESSAGE_POOL_MESSAGE_MAX_LEN; i++) {
				if (bit_test(cambiosParaElEventoOGet_programacionHoraria, i)) {
					bit_clear(cambiosParaElEventoOGet_programacionHoraria, i);
					
					offset = i * PGA_PROG_HORARIA_LARGO;
					
					imClient_putPayloadByte(msg, i + 1);
					imClient_putPayloadByte(msg, 1);
					imClient_putPayloadByte(msg, pgaData[PGA_PROG_HORARIA_1_PARTICION + offset] + 1);
					imClient_putPayloadBytes(msg, &(pgaData[PGA_PROG_HORARIA_1_HORA_ENCENDIDO + offset]), 4);
					imClient_putPayloadBytes(msg, &(pgaData[PGA_PROG_HORARIA_1_NODOS + offset]), 5);
					
					bytes += 12;
				}
			}
		}
		
		if (cambiosParaElEventoOGet_fototimer != 0) {
			for (uint8_t i = 0; i < 16 && (bytes + 9) <= MESSAGE_POOL_MESSAGE_MAX_LEN; i++) {
				if (bit_test(cambiosParaElEventoOGet_fototimer, i)) {
					bit_clear(cambiosParaElEventoOGet_fototimer, i);
					
					offset = i * PGA_FOTOTIMER_LARGO;
					
					imClient_putPayloadByte(msg, i + 1);
					imClient_putPayloadByte(msg, 2);
					imClient_putPayloadByte(msg, pgaData[PGA_FOTOTIMER_1_PARTICION + offset] + 1);
					imClient_putPayloadByte(msg, pgaData[PGA_FOTOTIMER_1_HORAS + offset]);
					imClient_putPayloadBytes(msg, &(pgaData[PGA_FOTOTIMER_1_NODOS + offset]), 5);
					
					bytes += 9;
				}
			}
		}
		
		if (cambiosParaElEventoOGet_noche != 0) {
			for (uint8_t i = 0; i < 16 && (bytes + 8) <= MESSAGE_POOL_MESSAGE_MAX_LEN; i++) {
				if (bit_test(cambiosParaElEventoOGet_noche, i)) {
					bit_clear(cambiosParaElEventoOGet_noche, i);
					
					offset = i * PGA_NOCHE_LARGO;

					imClient_putPayloadByte(msg, i + 1);
					imClient_putPayloadByte(msg, 3);
					imClient_putPayloadByte(msg, pgaData[PGA_NOCHE_1_PARTICION + offset] + 1);
					imClient_putPayloadBytes(msg, &(pgaData[PGA_NOCHE_1_NODOS + offset]), 5);
					
					bytes += 8;
				}
			}
		}
		
		if (cambiosParaElEventoOGet_simulador != 0) {
			for (uint8_t i = 0; i < 16 && (bytes + 8) <= MESSAGE_POOL_MESSAGE_MAX_LEN; i++) {
				if (bit_test(cambiosParaElEventoOGet_simulador, i)) {
					bit_clear(cambiosParaElEventoOGet_simulador, i);
					
					offset = i * PGA_SIMULADOR_LARGO;
					
					imClient_putPayloadByte(msg, i + 1);
					imClient_putPayloadByte(msg, 4);
					imClient_putPayloadByte(msg, pgaData[PGA_SIMULADOR_1_PARTICION + offset] + 1);
					imClient_putPayloadBytes(msg, &(pgaData[PGA_SIMULADOR_1_NODOS + offset]), 5);
					
					bytes += 8;
				}
			}
		}

		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_CONFIGURACION_NODOS, 0, 0);
	}
}


void procesarSetNodoTemporizado (uint8_t* payload, uint8_t len, uint8_t layer)
{
	uint8_t i, j;
	uint8_t nodo, particion, horas, minutos;
	
	if (len < 4)
	return;
	
	
	timersParaMandarPorEvento = 0;
	i = 0;
	while ((i+3) < len) {
		nodo = payload[i];
		i++;
		
		particion = payload[i];
		i++;
		
		horas = payload[i];
		i++;
		
		minutos = payload[i];
		i++;
		
		// Busco un timer libre
		for (j = 0; j < CANTIDAD_NODOS_TEMPORIZADOS; j++) {
			if (timers[j].layer == 0xff) {
				timers[j].layer = particion - 1;
				timers[j].nodo = nodo;
				timers[j].horas = horas;
				timers[j].minutos = minutos;
				timers[j].segundosTotales = minutos * 60 + horas * 60 * 60;
				
				prenderNodo(nodo, particion - 1);
				bit_set(timersParaMandarPorEvento, j);
				break;
			}
		}
		
		// Si no encontró lugar se deja de analizar el mensaje que llegó
		// y se devuelve un evento vacío
		if (j >= CANTIDAD_NODOS_TEMPORIZADOS) {
			break;
		}
	}
	
	armarEventoNodoTemporizado();
}


void armarEventoNodoTemporizado (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();


	if (msg != NULL) {
		
		for (uint8_t i = 0; i < CANTIDAD_NODOS_TEMPORIZADOS; i ++) {
			if (bit_test(timersParaMandarPorEvento, i)) {
				bit_clear(timersParaMandarPorEvento, i);
				
				imClient_putPayloadByte(msg, timers[i].nodo);
				imClient_putPayloadByte(msg, timers[i].layer + 1);
				imClient_putPayloadByte(msg, timers[i].horas);
				imClient_putPayloadByte(msg, timers[i].minutos);
			}
		}
		

		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_NODO_TEMPORIZADO, 0, 0);
	}
}


void procesarGetNodoTemporizado (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();


	if (msg != NULL) {
		
		for (uint8_t i = 0; i < CANTIDAD_NODOS_TEMPORIZADOS; i ++) {
			if (timers[i].layer != 0xff) {
				imClient_putPayloadByte(msg, timers[i].nodo);
				imClient_putPayloadByte(msg, timers[i].layer + 1);
				imClient_putPayloadByte(msg, timers[i].horas);
				imClient_putPayloadByte(msg, timers[i].minutos);
			}
		}
		

		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_NODO_TEMPORIZADO, layer, 0);
	}
}


void procesarApagarTodo (uint8_t layer)
{
	for (uint8_t i = 0; i < 4; i++) {
		nodosAApagar[layer][i] = 0;
		nodosAPrender[layer][i] = 0;
		estadoNodos[layer][i] = 0;
		
		timer_mandarEventoEstadoNodos[layer] = 2;
	}
	
	// Se apagan todos los timers que estén habilitados
	for (uint8_t i = 0; i < CANTIDAD_NODOS_TEMPORIZADOS; i++) {
		timers[i].layer = 0xff;
	}
}


void procesarEncenderApagarNodo (uint8_t dataH, uint8_t dataL, uint8_t layer)
{
	uint8_t grupo;
	uint8_t nodo;
	uint8_t bit;
	
	
	nodo = ((dataH & 0x0f) << 4) | (dataL >> 4) & 0x0f;
	grupo = nodo / 32;
	bit = nodo % 32;
	
	if (dataH < 0x70) {
		// Se prendió un nodo
		bit_set(estadoNodos[layer][grupo], bit);
		timer_mandarEventoEstadoNodos[layer] = 2;
	}
	else {
		// Se apagó un nodo
		bit_clear(estadoNodos[layer][grupo], bit);
		timer_mandarEventoEstadoNodos[layer] = 2;
		
		// Si estaba temporizado, se apaga el timer.
		for (uint8_t i = 0; i < CANTIDAD_NODOS_TEMPORIZADOS; i++) {
			if (timers[i].layer != 0xff && timers[i].nodo == nodo) {
				timers[i].layer = 0xff;
			}
		}
	}
}


void prenderNodo (uint8_t nodo, uint8_t layer)
{
	uint8_t grupo;
	uint8_t bit;
	
	
	if (nodo < 128) {
		grupo = nodo / 32;
		bit = nodo % 32;
		
		bit_set(nodosAPrender[layer][grupo], bit);
	}
}


void apagarNodo (uint8_t nodo, uint8_t layer)
{
	uint8_t grupo;
	uint8_t bit;
	
	
	if (nodo < 128) {
		grupo = nodo / 32;
		bit = nodo % 32;
		
		bit_set(nodosAApagar[layer][grupo], bit);
	}
}