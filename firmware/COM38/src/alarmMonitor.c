#include "inc/alarmMonitor.h"
#include "inc/mpxh.h"
#include "inc/project.h"
#include "inc/basicDefinitions.h"
#include "inc/EEPROM_MAP.h"
#include "inc/PGA_Map.h"
#include "inc/pga.h"
#include "inc/imClient.h"
#include "inc/imClient_cmds_regs.h"
#include "inc/configurationManager.h"


static bool yaMandeMensajeInicial;
static uint8_t timer_inicial;
uint8_t timer_centralNueva;
uint8_t estadoAlarma[8];
uint8_t timer_status[8];
uint8_t particionOk;
uint8_t statusOk;
uint8_t vino_huboDisparoDeAlarma;
uint8_t timer_huboDisparoDeAlarma[8];
uint8_t vino_activacionParcial;
uint8_t timer_activacionParcial[8];
uint8_t estoy;
uint8_t meVoy;
uint8_t n_usuario[8];
uint8_t timer_usuario[8];
uint8_t reg_sonando;
uint8_t contadorDisparos_robo[8];
uint8_t contadorDisparos_incendio[8];
uint8_t contadorDisparos_panico[8];
uint8_t contadorDisparos_tamper[8];
uint8_t reg_s[8];
uint8_t incendioManual;
uint8_t timer_robo[8];
uint8_t hayQueDispararPorRobo;
uint8_t timer_controlMpxh;
uint32_t zonasEstado[8];
uint32_t zonasMemoria[8];
uint32_t zonasInclusion[8];
uint8_t zonasCondicionalTemporizada[8];
uint8_t timer_red;
uint8_t perifericoEnProgramacion;
uint8_t timer_perifericoEnProgramacion[8];
uint8_t sonando;
uint8_t ready;
uint8_t timer_disparo[8];
uint8_t ultimaCausaDisparo[8];
uint8_t timer_openClose[8];
uint8_t timer_mandarEstadoZonas[8];
uint8_t timer_mandarMemoriaZonas[8];
uint8_t timer_mandarInclusion[8];
uint8_t zonasReplay[8];
uint8_t timer_replay;
uint8_t indexReplay;
uint8_t layerReplay;
uint8_t timer_eventos;
uint8_t eventos_contadorBytesTotales;
uint8_t eventos_contadorBytesPorEvento;
uint8_t eventos_buffer[30];
uint8_t eventos_indexBuffer;
uint8_t eventos_auxByte5;
uint8_t eventos_layer;
uint8_t codigoUsuarioAMandar[6];
uint8_t largoCodigoUsuarioAMandar;
uint8_t mandandoCambioModoAEstoy;
uint8_t mandandoCambioModoAMeVoy;
uint8_t timer_mandarEventoConfiguracionRetardos;
uint8_t timer_mandarEventoEstadoMpxh;
uint8_t timer_mandarEventoSonandoReady;
uint8_t lastFromIdReceived[4];

union BateriaRed{
	uint8_t byte;
	struct {
		uint8_t bateBaja:1;
		uint8_t bateDudosa:1;
		uint8_t bateBien:1;
		uint8_t red:1;
		uint8_t xred:1;
		uint8_t redOk:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
	} bits;
} bateriaRed;

union VariosCentral{
	uint8_t byte;
	struct {
		uint8_t noche:1;
		uint8_t hayLineaTelefonica:1;
		uint8_t hayTlcd:1;
		uint8_t esCentralNueva:1;
		uint8_t enReplay:1;
		uint8_t es_9108:1;
		uint8_t mande38368:1;
		uint8_t esRegistradorNuevo:1;
	} bits;
} variosCentral;


union VariosCentral2{
	uint8_t byte;
	struct {
		uint8_t recibiGetreplay:1;
		uint8_t recibiGetEventos:1;
		uint8_t bit2:1;
		uint8_t bit3:1;
		uint8_t bit4:1;
		uint8_t bit5:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
	} bits;
} variosCentral2;


union TipoCentral{
	uint8_t byte;
	struct {
		uint8_t tiene4zonas:1;
		uint8_t tiene8zonas:1;
		uint8_t tiene16zonas:1;
		uint8_t tiene32zonas:1;
		uint8_t bit4:1;
		uint8_t bit5:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
	} bits;
} tipoCentral;


static union ProcesandoMensajes{
	uint32_t w;
	struct {
		uint8_t procesandoSetEstado:1;
		uint8_t procesandoSetInclusion:1;
		uint8_t procesandoGetEventos:1;
		uint8_t procesandoGetReplay:1;
		uint8_t bit4:1;
		uint8_t bit5:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
	} bits;
} procesandoMensajes;


static union VariosMpxh{
	uint32_t w;
	struct {
		uint8_t errorMpxh:1;
		uint8_t vino_pirupiru:1;
		uint8_t bit2:1;
		uint8_t bit3:1;
		uint8_t bit4:1;
		uint8_t bit5:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
	} bits;
} variosMpxh;


static void parsear4ax (uint8_t dataL);
static void parsear49x (uint8_t dataL, uint8_t layer);
static void parsear4bx (uint8_t dataL, uint8_t layer);
static void parsear4cx (uint8_t dataL, uint8_t layer);
static void parsearSonidos (uint8_t dataL, uint8_t layer);
static void parsearDisparos (uint8_t dataL, uint8_t layer);
static void parsearEstadoZonas (uint8_t dataH, uint8_t dataL, uint8_t layer);
static void parsearMemoriaZonas (uint8_t dataH, uint8_t dataL, uint8_t layer);
static void parsearInclusionZonas (uint8_t dataH, uint8_t dataL, uint8_t layer);
static uint8_t busco_usuario(uint8_t layer);
static void obtenerEventoYZonaUsuarioDelEvento(uint8_t* evento, uint8_t* zona_usuario);
static void parsearEventosMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer);

static void armarPayloadGetInclusion (imMessage_t* msg, uint8_t layer);
static void armarPayloadGetEstadoZonas (imMessage_t* msg, uint8_t layer);
static void armarPayloadGetMemoria (imMessage_t* msg, uint8_t layer);
static void armarPayloadGetSonandoReady (imMessage_t* msg);
static void armarGetReplay(uint8_t layer, bool estaVacio);
static void armarGetEventos(uint8_t layer, uint8_t cantidadEventos);

static void procesarSetEstado(uint8_t* payload, uint8_t payloadLen, uint8_t layer);
static void procesarSetInclusion(uint8_t* payload, uint8_t payloadLen, uint8_t layer);
static void procesarSetSonandoReady(uint8_t* payload, uint8_t payloadLen, uint8_t layer);
static void procesarComandoDisparar(uint8_t* payload, uint8_t payloadLen, uint8_t layer);



void alarmMonitor_init (void) {
	bateriaRed.bits.red = 1;
	bateriaRed.bits.xred = 1;
	bateriaRed.bits.redOk = 0;
	bateriaRed.bits.bateBien = 1;
	bateriaRed.bits.bateDudosa = 0;
	bateriaRed.bits.bateBaja = 0;
	
	variosCentral.bits.esCentralNueva = 1;
	
	yaMandeMensajeInicial = false;
}


void alarmMonitor_analizarMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer, uint8_t nbits)
{
	switch (nbits) {
		case MPXH_BITS_17:
		case MPXH_BITS_16:
		
			if (dataH >= 0x30 && dataH <= 0x4d) {
				timer_status[layer] = 32;
				timer_controlMpxh = 0;
				
				// Si volvió a aparecer la partición o se fue el error en MPXH
				// mando el evento
				if (variosMpxh.bits.errorMpxh == 1 || bit_test(particionOk,layer) == 0) {
					variosMpxh.bits.errorMpxh = 0;
					bit_set(particionOk, layer);
					timer_mandarEventoEstadoMpxh = 4;
				}
				else {
					variosMpxh.bits.errorMpxh = 0;
					bit_set(particionOk, layer);
				}
					
					
				
			}
		
			if (layer == 0) {
				if (dataH == 0x08 && dataL == 0x90) {
					variosCentral.bits.esCentralNueva = true;
					timer_centralNueva = 0;
				}
				
				if (dataH == 0x0d) {
					if (dataL == 0x00) {					// 0D0 - existo TLCD*/PCLCD
						variosCentral.bits.hayTlcd = 1;
					}
					if (dataL == 0x30) {					// 0D3 - noche
						variosCentral.bits.noche = 1;
					}
					else if (dataL == 0x40) {				// 0D4 - día
						variosCentral.bits.noche = 0;
					}
					else if (dataL == 0xb0) {				// 0DB - falta línea telefónica
						//if (variosCentral.bits.hayLineaTelefonica)
						//	ap1.bits.ap_falta_lineaTelefonica = 1;
							
						variosCentral.bits.hayLineaTelefonica = 0;
					}
					else if (dataL == 0xc0) {				// 0DC - volvió línea telefónica
						//if (variosCentral.bits.hayLineaTelefonica == 0)
						//	ap1.bits.ap_recupero_lineaTelefonica = 1;
							
						variosCentral.bits.hayLineaTelefonica = 1;
					}
				}
				else if (dataH == 0x4a) {					// 4Ax - batería/red
					parsear4ax(dataL);
				}
			}
			
			if (dataH >= 0x30 && dataH <= 0x37) {				
				parsearEstadoZonas(dataH, dataL, layer);
			}
			else if (dataH >= 0x38 && dataH <= 0x3f) {			
				parsearMemoriaZonas(dataH, dataL, layer);
			}
			else if (dataH >= 0x40 && dataH <= 0x48) {
				parsearInclusionZonas(dataH, dataL, layer);
			}
			else if (dataH == 0x49) {					// 49x - estado
				parsear49x(dataL, layer);
			}
			else if (dataH == 0x4b) {					// 4bx - varios
				parsear4bx(dataL, layer);
			}
			else if (dataH == 0x4c) {					// 4cx - s_robo, s_incendio,
				parsear4cx(dataL, layer);
			}
			else if (dataH == 0x68 || dataH == 0x6c) {	// 68x/6cx - sonidos condicionales/incondicionales
				parsearSonidos(dataL, layer);
			}
			else if (dataH == 0x08) {
				if(dataL >= 0x10 && dataL <= 0x80) {
					parsearDisparos(dataL, layer);
				}
			}
			else if (dataH >= 0x02 && dataH <= 0x03) {		// 02x - 03x - Opera usuario
				n_usuario[layer] = nibble_swap(dataL);
				
				if (dataH == 0x03) 
					n_usuario[layer] |= 0x10;
					
				timer_usuario[layer] = 0;
			}
			else if (dataH == 0x00) {
				if (dataL == 0xb0)
					bit_clear(perifericoEnProgramacion, layer);		// F - saca a cualquier periférico de programación
				else if (dataL == 0xa0)
					timer_perifericoEnProgramacion[layer] = 0;		// P mantiene al periférico en programación
			}
			else if (dataH == 0x01) {
				if (dataL == 0x00) {								// 010 - Programación avanzada
					if(estadoAlarma[layer] != ESTADO_ALARMA_PROGRAMACION && 
							bit_test(perifericoEnProgramacion, layer) == 0) {
						timer_disparo[layer] = 2;
						ultimaCausaDisparo[layer] = DISPARO_EMERGENCIA_MEDICA;
						bit_set(mandarAgudoIncondicional,layer);
					}
				}
				else if (dataL == 0x10) {							// 011 - Evento incendio
					bit_set(incendioManual, layer);
				}
			}
			else if (dataH == 0x0a) {
				if (dataL == 0x00) {								// 0a0 - Asalto
					alarmMonitor_armarEventoDisparo(layer, DISPARO_ASALTO);
				}
				else if ((dataL == 0x20 || dataL == 0x30) && layer == layerReplay) {	// 0a2/0a3 - begin replay/borrar leds
						if (variosCentral.bits.enReplay == 0) 
							indexReplay = 0;
						
						variosCentral.bits.enReplay = 1;
						timer_replay = 8;
				}
				else if (dataL == 0x40 && layer == layerReplay) {	// 0a4 - end replay
					if (variosCentral.bits.enReplay) {
						timer_replay = 0;
						variosCentral.bits.enReplay = 0;
						
						if (variosCentral2.bits.recibiGetreplay)
							armarGetReplay(layerReplay, false);
							
						procesandoMensajes.bits.procesandoGetReplay = 0;
						variosCentral2.bits.recibiGetreplay = 0;
					}
				}
				else if (dataL == 0xb0) {							// 0ab - periférico entra en programación
					bit_set(perifericoEnProgramacion, layer);
					timer_perifericoEnProgramacion[layer] = 0;
				}
			}
			else if (dataH == 0x68 || dataH == 0x6c) {				//68x y 6cx - sonidos condicionales e incondicionales
				if (dataL == 0x40 && layer == 0)
					variosMpxh.bits.vino_pirupiru = 1;
			}
			
			break;
			
		case MPXH_BITS_15:
		
			if (dataH == 0b00001110 || dataH ==0b00001101) {		//T_xx o U_xx
				parsearEventosMpxh(dataH, dataL, layer);
			}
		
			break;
			
		default:
		
			break;
	}
}


uint8_t alarmMonitor_esDia (void)
{
	return (variosCentral.bits.noche == 0);
}


uint8_t alarmMonitor_hayLineaTelefonica (void)
{
	return (variosCentral.bits.hayLineaTelefonica);
}


uint8_t alarmMonitor_hayTlcd (void)
{
	return (variosCentral.bits.hayTlcd);
}


uint8_t alarmMonitor_hayRed (void)
{
	return (bateriaRed.bits.xred);
}


uint8_t alarmMonitor_estadoMpxh (void)
{
	return (variosMpxh.bits.errorMpxh == 0);
}


uint8_t alarmMonitor_estadoCentral (uint8_t layer)
{
	return estadoAlarma[layer];
}


uint8_t alarmMonitor_existeLayer (uint8_t layer)
{
	return (bit_test(particionOk,layer) != 0);
}


uint32_t alarmMonitor_estadoBateria (void)
{
	uint32_t ret = BATERIA_BIEN;
	
	if (bateriaRed.bits.bateBien == 1) 
		ret = BATERIA_BIEN;
	
	if (bateriaRed.bits.bateDudosa == 1) 
		ret = BATERIA_DUDOSA;
	
	if (bateriaRed.bits.bateBaja == 1) 
		ret = BATERIA_BAJA;
	
	
	return ret;
}


void alarmMonitor_timers1s_handler (void)
{
	// En el caso de una pérdida de conexión, se fuerza a que el WIFICOM100 mande un
	// mensaje cualquiera al backend, para que si hay una app abierta en ese momento
	// se actualice y vuelva a mostrar el WIFICOM100 conectado.
	// Solo se manda una vez este mensaje, hasta que se vuelva a desconectar del backend.
	if (imClient_isClientConnected()) {
		if (!yaMandeMensajeInicial) {
			alarmMonitor_armarEventoEstadoZonas(0);
			//alarmMonitor_armarEventoRed();
			//alarmMonitor_armarEventoBateria();
			for (uint8_t i = 0; i < 8; i++) {
				if (alarmMonitor_existeLayer(i))
				alarmMonitor_armarGetEstado(i);
			}
			yaMandeMensajeInicial = true;
		}
	}
	else {
		yaMandeMensajeInicial = false;
	}
	
	
	for (int i = 0; i < 8; i++) {
		if (timer_huboDisparoDeAlarma[i] != 0) {
			timer_huboDisparoDeAlarma[i] --;
			
			if (timer_huboDisparoDeAlarma[i] == 0) 
				bit_clear(vino_huboDisparoDeAlarma, i);
		}
		
		
		if (timer_activacionParcial[i] != 0) {
			timer_activacionParcial[i] --;
			
			if (timer_activacionParcial[i] == 0)
			bit_clear(vino_activacionParcial, i);
		}
		
		
		if (timer_usuario[i] < 8)
			timer_usuario[i] ++;
		
		
		// Luego de pasado el tiempo de retardo configurado, se manda el disparo de robo
		if (timer_robo[i] != 0) {
			timer_robo[i] --;
			
			if (timer_robo[i] == 0) {
				// Luego de pasado el tiempo de retardo, se manda el evento de robo/robo 24hs
				if (bit_test(hayQueDispararPorRobo, i)) {
					bit_clear(hayQueDispararPorRobo, i);
					
					alarmMonitor_armarEventoDisparo(i, DISPARO_ROBO);
				}
				
			}
		}
			
			
		// 32 segundos sin venir status en cada partición
		if (timer_status[i] != 0) {
			timer_status[i] --;
			
			if (timer_status[i] == 0) {
				bit_clear(particionOk, i);
				timer_mandarEventoEstadoMpxh = 4;
			}
		}
		
		
		if (timer_openClose[i] != 0) {
			timer_openClose[i] --;
			
			if (timer_openClose[i] == 0) {
				alarmMonitor_armarEventoOpenClose(i);
			}
		}
		
		
		if (timer_mandarEventoSonandoReady != 0) {
			timer_mandarEventoSonandoReady --;
			
			if (timer_mandarEventoSonandoReady == 0) {
				alarmMonitor_armarEventoSonandoReady();
			}
		}
		
		if (timer_disparo[i] != 0) {
			timer_disparo[i] --;
			
			if (timer_disparo[i] == 0) {
				alarmMonitor_armarEventoDisparo(i, ultimaCausaDisparo[i]);
			}
		}
		
		if (timer_mandarEstadoZonas[i] != 0) {
			timer_mandarEstadoZonas[i] --;
			
			if (timer_mandarEstadoZonas[i] == 0) {
				alarmMonitor_armarEventoEstadoZonas(i);
			}
		}
		
		if (timer_mandarMemoriaZonas[i] != 0) {
			timer_mandarMemoriaZonas[i] --;
			
			if (timer_mandarMemoriaZonas[i] == 0) {
				alarmMonitor_armarEventoMemoria(i);
			}
		}
		
		if (timer_mandarInclusion[i] != 0) {
			timer_mandarInclusion[i] --;
			
			if (timer_mandarInclusion[i] == 0) {
				alarmMonitor_armarEventoInclusion(i);
			}
		}
		
		if (timer_perifericoEnProgramacion[i] < 35) {
			timer_perifericoEnProgramacion[i] ++;
			
			if (timer_perifericoEnProgramacion[i] >= 35)
				bit_clear(perifericoEnProgramacion, i);
		}
	}
	
	
	if (timer_inicial < 128) {
		timer_inicial ++;
		
		// A los 6 segundos de encendido, pido status en todas las particiones
		if (timer_inicial == 6) {
			pedirStatusCentral = 0xFF;
		}
	}
	
	// 64 segundos sin recibir status en ninguna partición, da falla en MPXH
	if(timer_controlMpxh < 64) {
		timer_controlMpxh++;
		
		if (timer_controlMpxh >= 64) {
			tipoCentral.byte = 0;
			variosMpxh.bits.errorMpxh = 1;
			timer_mandarEventoEstadoMpxh = 4;
		}
	}
	
	
	// 16 segundos para dar falta o restore de red
	if (timer_red > 0) {
		timer_red--;
		
		if (timer_red == 0) {
			bateriaRed.bits.xred = bateriaRed.bits.red;
			
			alarmMonitor_armarEventoRed();
		}
	}

	// 8 segundos para recibir el replay de disparo
	if(timer_replay!=0) {
		timer_replay--;
		
		if(timer_replay==0) {
			variosCentral.bits.enReplay = 0;
			
			if (variosCentral2.bits.recibiGetreplay)
				armarGetReplay(layerReplay, false);
				
			procesandoMensajes.bits.procesandoGetReplay = 0;
			variosCentral2.bits.recibiGetreplay = 0;
		}
	}
	
	
	// 8 segudnos de timeout (por cada evento) para la llegada de mensajes de 15 bits
	// de eventos
	if (timer_eventos != 0) {
		timer_eventos --;
		
		if (timer_eventos == 0) {
			if (eventos_contadorBytesTotales != 250 && eventos_contadorBytesTotales != 48) {
				if (variosCentral.bits.mande38368) {
					variosCentral.bits.mande38368 = 0;
					
					// Si me queda algún evento por mandar y está completo, lo mando
					uint8_t cantidadDeEventos = eventos_indexBuffer / 6;
					
					if (cantidadDeEventos != 0) {
						armarGetEventos(eventos_layer, cantidadDeEventos);
					
						eventos_indexBuffer = 0;
					}
					
					procesandoMensajes.bits.procesandoGetEventos = 0;
					variosCentral2.bits.recibiGetEventos = 0;
				}
				else {
					if (variosCentral.bits.esRegistradorNuevo == 0 && variosCentral.bits.es_9108) {
						// mando la secuencia EVENT
						bufferTeclas[0] = 0x03;
						bufferTeclas[1] = 0x08;
						bufferTeclas[2] = 0x03;
						bufferTeclas[3] = 0x06;
						bufferTeclas[4] = 0x08;

						ptrTeclas = 0;
						lenTeclas = 5;
						bit_set(mandarTeclas,0);
					
						variosCentral.bits.mande38368 = 1;
					}
					else {
						// Si me queda algún evento por mandar y está completo, lo mando
						uint8_t cantidadDeEventos = eventos_indexBuffer / 6;
					
						if (cantidadDeEventos != 0) {
							armarGetEventos(eventos_layer, cantidadDeEventos);
					
							eventos_indexBuffer = 0;
						}
						
						procesandoMensajes.bits.procesandoGetEventos = 0;
						variosCentral2.bits.recibiGetEventos = 0;
					}
				}
			}
			else if (eventos_contadorBytesTotales == 48) {
				// Terminaron de venir los 8 eventos
				// Si me queda algún evento por mandar y está completo, lo mando
				uint8_t cantidadDeEventos = eventos_indexBuffer / 6;
				
				if (cantidadDeEventos != 0) {
					armarGetEventos(eventos_layer, cantidadDeEventos);
					
					eventos_indexBuffer = 0;
				}
				
				procesandoMensajes.bits.procesandoGetEventos = 0;
				variosCentral2.bits.recibiGetEventos = 0;
			}
		}
	}
	
	
	if (timer_mandarEventoConfiguracionRetardos > 0) {
		timer_mandarEventoConfiguracionRetardos--;
		
		if (timer_mandarEventoConfiguracionRetardos == 0) {
			configurationManager_armarEventoConfiguracionRobo();
		}
	}
	
	
	if (timer_mandarEventoEstadoMpxh > 0) {
		timer_mandarEventoEstadoMpxh --;
		
		if (timer_mandarEventoEstadoMpxh == 0)
			alarmMonitor_armarEventoEstadoMpxh();
	}
	
}


void alarmMonitor_timers1m_handler (void)
{
	if (variosCentral.bits.esCentralNueva) {
		timer_centralNueva++;
		
		if (timer_centralNueva >= 8)						// 8 minutos sin que demuestre ser una central nueva
			variosCentral.bits.esCentralNueva = 0;
	}
}


void alarmMonitor_timers1h_handler (void)
{

}


void alarmMonitor_setRetardo(uint8_t layer, uint8_t retardo)
{
	if (retardo != pgaData[PGA_RETARDO_P1 + layer]) {
		pgaData[PGA_RETARDO_P1 + layer] = retardo;
		
		pga_enqueueSave(PGA_RETARDO_P1 + layer, 1, EE_RETARDO_P1_ADDR + layer);
		
		timer_mandarEventoConfiguracionRetardos = 2;
	}
}


uint8_t alarmMonitor_getRetardo(uint8_t layer)
{
	return (pgaData[PGA_RETARDO_P1 + layer]);
}



uint8_t alarmMonitor_estadoAlarma (uint8_t layer)
{
	return estadoAlarma[layer];
}


uint8_t alarmMonitor_tipoCentral (void)
{
	uint8_t ret = TIPO_CENTRAL_NINGUNO;
	
	if(tipoCentral.bits.tiene32zonas)
		ret = TIPO_CENTRAL_N32;
	else if (tipoCentral.bits.tiene16zonas)
		ret = TIPO_CENTRAL_N16;
	else if (tipoCentral.bits.tiene8zonas)
		ret = TIPO_CENTRAL_N8;
	else if (tipoCentral.bits.tiene4zonas)
		ret = TIPO_CENTRAL_N4;
	
	return ret;
}


uint8_t alarmMonitor_cantidadZonasCentral (void)
{
	uint8_t ret = 32;
	
	if(tipoCentral.bits.tiene32zonas)
		ret = 32;
	else if (tipoCentral.bits.tiene16zonas)
		ret = 16;
	else if (tipoCentral.bits.tiene8zonas)
		ret = 8;
	else if (tipoCentral.bits.tiene4zonas)
		ret = 4;
	
	return ret;
}


bool alarmMonitor_hayPerifericoEnProgramacion (uint8_t layer)
{
	return (bit_test(perifericoEnProgramacion, layer) != 0);
}


void alarmMonitor_resetTimerPerifericoEnProgramacion (uint8_t layer)
{
	bit_set(perifericoEnProgramacion, layer);
	timer_perifericoEnProgramacion[layer] = 0;
}


void alarmMonitor_clearPerifericoEnProgramacion (uint8_t layer)
{
	bit_clear(perifericoEnProgramacion, layer);
	timer_perifericoEnProgramacion[layer] = 64;
}


bool alarmMonitor_analizarIm (imMessage_t* msg)
{
	bool ret = false;
	
	if (msg->cmd == IM_CLIENT_CMD_GET) {
		if (msg->reg == IM_CLIENT_REG_ESTADO) {
			alarmMonitor_armarGetEstado(msg->part);
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_INCLUSION) {
			alarmMonitor_armarGetInclusion(msg->part);
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_MEMORIA) {
			alarmMonitor_armarGetMemoria(msg->part);
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_ESTADO_ZONAS) {
			alarmMonitor_armarGetEstadoZonas(msg->part);
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_SONANDO_READY) {
			alarmMonitor_armarGetSonandoReady();
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_EVENTOS) {
			if (alarmMonitor_existeLayer(msg->part)) {
				eventos_layer = msg->part;
			
				bit_set(pedirEventos, eventos_layer);
				variosCentral.bits.mande38368 = 0;
				variosCentral.bits.es_9108 = 0;
				variosCentral.bits.esRegistradorNuevo = 0;
				eventos_contadorBytesTotales = 0;
				eventos_contadorBytesPorEvento = 0;
				eventos_indexBuffer = 0;
				variosMpxh.bits.vino_pirupiru = 0;
				timer_eventos = 8;
			
				procesandoMensajes.bits.procesandoGetEventos = 1;
				variosCentral2.bits.recibiGetEventos = 1;
			}
			else {
				// Si no está la partición, se devuelve un mensaje sin eventos.
				armarGetEventos(msg->part, 0);
			}
			
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_REPLAY) {
			
			if (alarmMonitor_existeLayer(msg->part)) {
			
				if (variosCentral.bits.enReplay == 0) {
					if (alarmMonitor_estadoMpxh()) {
						bit_set(pedirReplay, msg->part);
						timer_replay = 8;
						variosCentral.bits.enReplay = 0;
						layerReplay = msg->part;
						indexReplay = 0;
						for (uint8_t i = 0; i < 8; i++)
							zonasReplay[i] = 0;
					
						variosCentral2.bits.recibiGetreplay = 1;
						procesandoMensajes.bits.procesandoGetReplay = 1;
					}
					else {
						armarGetReplay(layerReplay, true);
					}
				}
				else {
					armarGetReplay(layerReplay, true);
				}
			}
			else {
				// Si no está la partición, se devuelve un mensaje sin eventos.
				armarGetReplay(layerReplay, true);
			}
			
			imClient_removeMessageToRead(0);
			ret = true;
		}
	}
	else if (msg->cmd == IM_CLIENT_CMD_SET) {
		if (msg->reg == IM_CLIENT_REG_ESTADO) {
			procesarSetEstado (msg->payload, msg->len, msg->part);
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_INCLUSION) {
			procesarSetInclusion (msg->payload, msg->len, msg->part);
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_SONANDO_READY) {
			procesarSetSonandoReady(msg->payload, msg->len, msg->part);
			imClient_removeMessageToRead(0);
			ret = true;
		}
	}
	else if (msg->cmd == IM_CLIENT_CMD_RESET) {
		if (msg->reg == IM_CLIENT_REG_MEMORIA) {
			if (alarmMonitor_existeLayer(msg->part)) {
				if (alarmMonitor_estadoAlarma(msg->part) == ESTADO_ALARMA_DESACTIVADA)
					bit_set(mandarBorrarMemoria,msg->part);
			}
			
			imClient_removeMessageToRead(0);
			ret = true;
		}
	}
	else if (msg->cmd == IM_CLIENT_CMD_DISPARAR) {
		procesarComandoDisparar(msg->payload, msg->len, msg->part);
		imClient_removeMessageToRead(0);
		ret = true;
	}
	
	return ret;
}


void alarmMonitor_determinarFinProcesamiento(void)
{
	if (procesandoMensajes.bits.procesandoSetEstado) {
		if (ptrTeclas >= lenTeclas) {
			// Se terminaron de sacar las teclas por MPXH
			procesandoMensajes.bits.procesandoSetEstado = 0;
		}	
	}
	else if (procesandoMensajes.bits.procesandoSetInclusion) {
		if (ptrTeclas >= lenTeclas) {
			// Se terminaron de sacar las teclas por MPXH
			procesandoMensajes.bits.procesandoSetInclusion = 0;
		}
	}
}


bool alarmMonitor_procesandoMensaje(void)
{
	return (procesandoMensajes.w != 0);
}


void alarmMonitor_armarGetEstado (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	uint8_t* ptrZonasEstado = (uint8_t*)&zonasEstado[layer];
	uint8_t* ptrZonasMemoria = (uint8_t*)&zonasMemoria[layer];
	uint8_t* ptrZonasInclusion = (uint8_t*)&zonasInclusion[layer];
	
	
	if (msg != NULL) {
		imClient_putPayloadByte(msg, estadoAlarma[layer]);
		imClient_putPayloadByte(msg, alarmMonitor_hayRed());
		imClient_putPayloadByte(msg, alarmMonitor_estadoBateria());
		imClient_putPayloadByte(msg, alarmMonitor_estadoMpxh());
		
		if (bit_test(particionOk, layer))
			imClient_putPayloadByte(msg, 1);
		else
			imClient_putPayloadByte(msg, 0);
		
		if (bit_test(sonando, layer))
			imClient_putPayloadByte(msg, 1);
		else
			imClient_putPayloadByte(msg, 0);
		
		if (bit_test(ready, layer))
			imClient_putPayloadByte(msg, 1);
		else
			imClient_putPayloadByte(msg, 0);
		
		if (tipoCentral.bits.tiene32zonas)
			imClient_putPayloadByte(msg, 32);
		else if (tipoCentral.bits.tiene16zonas)
			imClient_putPayloadByte(msg, 16);
		else if (tipoCentral.bits.tiene8zonas)
			imClient_putPayloadByte(msg, 8);
		else if (tipoCentral.bits.tiene4zonas)
			imClient_putPayloadByte(msg, 4);
		else
			imClient_putPayloadByte(msg, TIPO_CENTRAL_NINGUNO);
		
		imClient_putPayloadByte(msg, PROJECT_FIRMWARE_VERSION_MAYOR);
		imClient_putPayloadByte(msg, PROJECT_FIRMWARE_VERSION_MENOR);
		imClient_putPayloadByte(msg, *(ptrZonasEstado+3));
		imClient_putPayloadByte(msg, *(ptrZonasEstado+2));
		imClient_putPayloadByte(msg, *(ptrZonasEstado+1));
		imClient_putPayloadByte(msg, *(ptrZonasEstado+0));
		imClient_putPayloadByte(msg, *(ptrZonasMemoria+3));
		imClient_putPayloadByte(msg, *(ptrZonasMemoria+2));
		imClient_putPayloadByte(msg, *(ptrZonasMemoria+1));
		imClient_putPayloadByte(msg, *(ptrZonasMemoria+0));
		
		if (bit_test(estoy, layer))
			imClient_putPayloadByte(msg, 1);
		else
			imClient_putPayloadByte(msg, 0);
		
		if (bit_test(meVoy, layer))
			imClient_putPayloadByte(msg, 1);
		else
			imClient_putPayloadByte(msg, 0);
		
		imClient_putPayloadByte(msg, *(ptrZonasInclusion+3));
		imClient_putPayloadByte(msg, *(ptrZonasInclusion+2));
		imClient_putPayloadByte(msg, *(ptrZonasInclusion+1));
		imClient_putPayloadByte(msg, *(ptrZonasInclusion+0));
		imClient_putPayloadByte(msg, zonasCondicionalTemporizada[layer]);
		

		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_ESTADO, layer, 0);
	}
}


void alarmMonitor_armarEventoOpenClose (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();


	if (msg != NULL) {
		imClient_putPayloadByte(msg, estadoAlarma[layer]);
		imClient_putPayloadByte(msg, busco_usuario(layer));

		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_OPEN_CLOSE, layer, 0);
	}
}


void alarmMonitor_armarEventoRed (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();

	if (msg != NULL) {
		imClient_putPayloadByte(msg, alarmMonitor_hayRed());

		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_RED, 0, 0);
	}
}


void alarmMonitor_armarEventoBateria (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();

	if (msg != NULL) {
		imClient_putPayloadByte(msg, alarmMonitor_estadoBateria());

		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_BATERIA, 0, 0);
	}
}


void alarmMonitor_armarEventoEstadoMpxh (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();

	if (msg != NULL) {
		imClient_putPayloadByte(msg, alarmMonitor_estadoMpxh());
		imClient_putPayloadByte(msg, particionOk);
		
			
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_ESTADO_MPXH, 0, 0);
	}
}


void alarmMonitor_armarGetSonandoReady (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();

	if (msg != NULL) {
		armarPayloadGetSonandoReady(msg);	

		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_SONANDO_READY, 0, 0);
	}
}


void alarmMonitor_armarEventoSonandoReady (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();


	if (msg != NULL) {
		armarPayloadGetSonandoReady(msg);	

		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_SONANDO_READY, 0, 0);
	}
}


void armarPayloadGetSonandoReady (imMessage_t* msg) 
{
	imClient_putPayloadByte(msg, sonando);
	imClient_putPayloadByte(msg, ready);
	imClient_putPayloadByte(msg, estoy);
	imClient_putPayloadByte(msg, meVoy);
		
	if (tipoCentral.bits.tiene32zonas)
		imClient_putPayloadByte(msg, 32);
	else if (tipoCentral.bits.tiene16zonas)
		imClient_putPayloadByte(msg, 16);
	else if (tipoCentral.bits.tiene8zonas)
		imClient_putPayloadByte(msg, 8);
	else if (tipoCentral.bits.tiene4zonas)
		imClient_putPayloadByte(msg, 4);
	else 
		imClient_putPayloadByte(msg, TIPO_CENTRAL_NINGUNO);
}


void alarmMonitor_armarGetInclusion (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		armarPayloadGetInclusion(msg, layer);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_INCLUSION, layer, 0);
	}
}


void alarmMonitor_armarEventoInclusion (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		armarPayloadGetInclusion(msg, layer);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_INCLUSION, layer, 0);
	}
}


void armarPayloadGetInclusion (imMessage_t* msg, uint8_t layer) 
{
	uint8_t* ptrZonasInclusion = (uint8_t*)&zonasInclusion[layer];
	
	imClient_putPayloadByte(msg, *(ptrZonasInclusion+3));
	imClient_putPayloadByte(msg, *(ptrZonasInclusion+2));
	imClient_putPayloadByte(msg, *(ptrZonasInclusion+1));
	imClient_putPayloadByte(msg, *(ptrZonasInclusion+0));
	imClient_putPayloadByte(msg, zonasCondicionalTemporizada[layer]);
}


void alarmMonitor_armarGetMemoria (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		armarPayloadGetMemoria(msg, layer);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_MEMORIA, layer, 0);
	}
}


void alarmMonitor_armarEventoMemoria (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		armarPayloadGetMemoria(msg, layer);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_MEMORIA, layer, 0);
	}
}


void armarPayloadGetMemoria (imMessage_t* msg, uint8_t layer)
{
	uint8_t* ptrZonasMemoria = (uint8_t*)&zonasMemoria[layer];
	
	imClient_putPayloadByte(msg, *(ptrZonasMemoria+3));
	imClient_putPayloadByte(msg, *(ptrZonasMemoria+2));
	imClient_putPayloadByte(msg, *(ptrZonasMemoria+1));
	imClient_putPayloadByte(msg, *(ptrZonasMemoria+0));
}


void alarmMonitor_armarGetEstadoZonas (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		armarPayloadGetEstadoZonas(msg, layer);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_ESTADO_ZONAS, layer, 0);
	}
}


void alarmMonitor_armarEventoEstadoZonas (uint8_t layer)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		armarPayloadGetEstadoZonas(msg, layer);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_ESTADO_ZONAS, layer, 0);
	}
}


void armarPayloadGetEstadoZonas (imMessage_t* msg, uint8_t layer) 
{
	uint8_t* ptrZonasEstado = (uint8_t*)&zonasEstado[layer];
	
	imClient_putPayloadByte(msg, *(ptrZonasEstado+3));
	imClient_putPayloadByte(msg, *(ptrZonasEstado+2));
	imClient_putPayloadByte(msg, *(ptrZonasEstado+1));
	imClient_putPayloadByte(msg, *(ptrZonasEstado+0));
}



void alarmMonitor_armarEventoDisparo (uint8_t layer, uint8_t disparo)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	uint8_t* ptrZonasEstado = (uint8_t*)&zonasEstado[layer];
	uint8_t* ptrZonasMemoria = (uint8_t*)&zonasMemoria[layer];
	
	if (msg != NULL) {
		imClient_putPayloadByte(msg, disparo);
		imClient_putPayloadByte(msg, busco_usuario(layer));
		imClient_putPayloadByte(msg, alarmMonitor_hayRed());
		imClient_putPayloadByte(msg, alarmMonitor_estadoBateria());
		imClient_putPayloadByte(msg, sonando);
		imClient_putPayloadByte(msg, ready);
		imClient_putPayloadByte(msg, *(ptrZonasEstado+3));
		imClient_putPayloadByte(msg, *(ptrZonasEstado+2));
		imClient_putPayloadByte(msg, *(ptrZonasEstado+1));
		imClient_putPayloadByte(msg, *(ptrZonasEstado+0));
		imClient_putPayloadByte(msg, *(ptrZonasMemoria+3));
		imClient_putPayloadByte(msg, *(ptrZonasMemoria+2));
		imClient_putPayloadByte(msg, *(ptrZonasMemoria+1));
		imClient_putPayloadByte(msg, *(ptrZonasMemoria+0));
		
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_DISPARO, layer, 0);
	}
}


void armarGetReplay(uint8_t layer, bool estaVacio) 
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		if (!estaVacio) {
			for (uint8_t i = 0; i < indexReplay; i++) {
				imClient_putPayloadByte(msg, zonasReplay[i]);
			}
		}
		
		toId[0] = lastFromIdReceived[0];
		toId[1] = lastFromIdReceived[1];
		toId[2] = lastFromIdReceived[2];
		toId[3] = lastFromIdReceived[3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_REPLAY, layer, 0);
	}
}


void armarGetEventos(uint8_t layer, uint8_t cantidadEventos)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	uint8_t evento;
	uint8_t zona_usuario;
	
	
	if (msg != NULL) {
		/*
		En el buffer eventos_buffer cada evento está codificado en 6 bytes
		con el siguiente orden:
		 - evento
		 - segundos
		 - minutos
		 - horas
		 - dia
		 - mes
		 
		 En el mensaje van a ser 7 bytes 
		*/
		for (uint8_t i = 0; i < cantidadEventos; i++) {
			evento = eventos_buffer[6*i + 0];
			obtenerEventoYZonaUsuarioDelEvento(&evento, &zona_usuario);
			
			imClient_putPayloadByte(msg, bcd2dec(eventos_buffer[6*i + 4]));
			imClient_putPayloadByte(msg, bcd2dec(eventos_buffer[6*i + 5]));
			imClient_putPayloadByte(msg, bcd2dec(eventos_buffer[6*i + 3]));
			imClient_putPayloadByte(msg, bcd2dec(eventos_buffer[6*i + 2]));
			imClient_putPayloadByte(msg, bcd2dec(eventos_buffer[6*i + 1]));
			imClient_putPayloadByte(msg, evento);
			imClient_putPayloadByte(msg, zona_usuario);
		}
		
		
		toId[0] = lastFromIdReceived[0];
		toId[1] = lastFromIdReceived[1];
		toId[2] = lastFromIdReceived[2];
		toId[3] = lastFromIdReceived[3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_EVENTOS, layer, 0);
	}
}


void parsear4ax (uint8_t dataL)
{
	if (bit_test(dataL, 5)) {
		if (bateriaRed.bits.red == 0) {					// Volvió la red
			bateriaRed.bits.red = 1;
			timer_red = 2;
		}
	}
	else if (bit_test(dataL, 4) == 0) {
		if (bateriaRed.bits.red == 1) {					// Se fue la red
			bateriaRed.bits.red = 0;
			timer_red = 2;
		}
	}
	
	if (bit_test(dataL,7)) {
		if (bateriaRed.bits.bateBaja == 0) {
			bateriaRed.bits.bateBaja = 1;
			bateriaRed.bits.bateDudosa = 0;
			bateriaRed.bits.bateBien = 0;
			alarmMonitor_armarEventoBateria();
		}
		
		bateriaRed.bits.bateBaja = 1;
		bateriaRed.bits.bateDudosa = 0;
		bateriaRed.bits.bateBien = 0;
	}
	else {
		bateriaRed.bits.bateBaja = 0;
		
		if (bit_test(dataL,6)) {
			if (bateriaRed.bits.bateDudosa == 0) {
				bateriaRed.bits.bateDudosa = 1;
				bateriaRed.bits.bateBien = 0;
				alarmMonitor_armarEventoBateria();
			}
			
			bateriaRed.bits.bateDudosa = 1;
			bateriaRed.bits.bateBien = 0;
		}
		else {
			if (bateriaRed.bits.bateBien == 0) {
				bateriaRed.bits.bateDudosa = 0;
				bateriaRed.bits.bateBien = 1;
				alarmMonitor_armarEventoBateria();
			}
			
			bateriaRed.bits.bateDudosa = 0;
			bateriaRed.bits.bateBien = 1;
		}
	}
	
	
	// Se carga la primera ves el valor de xred
	if (bateriaRed.bits.redOk == 0) {
		bateriaRed.bits.redOk = 1;
		bateriaRed.bits.xred = bateriaRed.bits.red;
	}
}


void parsear49x (uint8_t dataL, uint8_t layer)
{
	uint8_t evento;
	
	
	if (bit_test(dataL, 6) == 0 && bit_test(dataL, 7) == 0) {				// Desactivada
		if (estadoAlarma[layer] == ESTADO_ALARMA_ACTIVADA) {
			bit_clear(hayQueDispararPorRobo,layer);
			// TO-DO llamar = 0;
			

			
			// TO-DO apagar simulador
		}
		
		if (estadoAlarma[layer] != ESTADO_ALARMA_DESACTIVADA) {
			estadoAlarma[layer] = ESTADO_ALARMA_DESACTIVADA;
			timer_openClose[layer] = 1;
		}
	}
	else if (bit_test(dataL, 6) && bit_test(dataL, 7)) {			// Activada		
		if (estadoAlarma[layer] != ESTADO_ALARMA_ACTIVADA &&
		estadoAlarma[layer] != ESTADO_ALARMA_ACT_ESTOY &&
		estadoAlarma[layer] != ESTADO_ALARMA_ACT_ME_VOY) {
			if (bit_test(statusOk, layer)) {
				
				estadoAlarma[layer] = ESTADO_ALARMA_ACTIVADA;
				
				if (bit_test(estoy, layer))
					estadoAlarma[layer] = ESTADO_ALARMA_ACT_ESTOY;
					
				else if (bit_test(meVoy, layer))
					estadoAlarma[layer] = ESTADO_ALARMA_ACT_ME_VOY;

				if (bit_test(vino_activacionParcial, layer))
					estadoAlarma[layer] = ESTADO_ALARMA_ACT_PARCIAL;
	
				timer_openClose[layer] = 1;
			}
			
			// TO-DO Prender nodos simulador
		}
	}
	else {
		if (estadoAlarma[layer] != ESTADO_ALARMA_PROGRAMACION) {
			estadoAlarma[layer] = ESTADO_ALARMA_PROGRAMACION;
			timer_openClose[layer] = 1;
		}
	}
	
	
	// Bit ready
	if (bit_test(dataL, 5)) {
		if (bit_test(ready, layer) == 0) {
			bit_set(ready, layer);
			timer_mandarEventoSonandoReady = 3;
		}
	}
	else {
		if (bit_test(ready, layer)) {
			bit_clear(ready, layer);
			timer_mandarEventoSonandoReady = 3;
		}
	}
	
	
	bit_set(statusOk, layer);
}


void parsear4bx (uint8_t dataL, uint8_t layer)
{	
	if (bit_test(dataL, 5)) {
		if (bit_test(dataL, 6)) {											// Está en Estoy
			if (bit_test(estoy,layer) == 0 || bit_test(meVoy,layer)) {
				timer_mandarEventoSonandoReady = 3;
				
				if (bit_test(mandandoCambioModoAEstoy,layer)) {
					bit_clear(mandandoCambioModoAEstoy,layer);
					
					// una vez cambiado el MODO mando el código que tenía guardado
					for (uint8_t i = 0; i < largoCodigoUsuarioAMandar; i++)
						bufferTeclas[i] = codigoUsuarioAMandar[i];
						
					ptrTeclas = 0;
					lenTeclas = largoCodigoUsuarioAMandar;
					bit_set(mandarTeclas,layer);
				}
				else if (bit_test(mandandoCambioModoAMeVoy,layer)) {
					bufferTeclas[0] = 0x0d;
					ptrTeclas = 0;
					lenTeclas = 1;
					bit_set(mandarTeclas,layer);
				}
			}
				
			bit_set(estoy,layer);
			bit_clear(meVoy,layer);
		}
		else {																// Está en Me Voy
			if (bit_test(estoy,layer) || bit_test(meVoy,layer) == 0) {
				timer_mandarEventoSonandoReady = 3;
				
				if (bit_test(mandandoCambioModoAEstoy,layer)) {				
					bufferTeclas[0] = 0x0d;
					ptrTeclas = 0;
					lenTeclas = 1;
					bit_set(mandarTeclas,layer);
				}
				else if (bit_test(mandandoCambioModoAMeVoy,layer)) {
					bit_clear(mandandoCambioModoAMeVoy,layer);
					
					// una vez cambiado el MODO mando el código que tenía guardado
					for (uint8_t i = 0; i < largoCodigoUsuarioAMandar; i++)
						bufferTeclas[i] = codigoUsuarioAMandar[i];
						
					ptrTeclas = 0;
					lenTeclas = largoCodigoUsuarioAMandar;
					bit_set(mandarTeclas,layer);
				}
			}
				
			bit_clear(estoy,layer);
			bit_set(meVoy,layer);
		}
	}
	else {																	// Poniendo Ninguno
		if (bit_test(estoy,layer) || bit_test(meVoy,layer))
			timer_mandarEventoSonandoReady = 3;
				
		bit_clear(estoy,layer);
		bit_clear(meVoy,layer);
	}
	
	if (bit_test(dataL, 4)) {
		
		if (bit_test(sonando, layer) == 0) {
			bit_set(sonando, layer);
			timer_mandarEventoSonandoReady = 3;
		}
		
		if (layer == 0 && variosCentral.bits.esCentralNueva)
			bit_set(reg_sonando, 7);
	}
	else {
		if (estadoAlarma[layer] != ESTADO_ALARMA_ACTIVADA) {
			// Llamar = 0
		}
		
		if (bit_test(sonando, layer)) {
			bit_clear(sonando, layer);
			timer_mandarEventoSonandoReady = 3;
		}
		
		contadorDisparos_robo[layer] = 0;
		contadorDisparos_incendio[layer] = 0;
		contadorDisparos_panico[layer] = 0;
		contadorDisparos_tamper[layer] = 0;
	}
	
	
	// Sonando para centrales viejas
	if(layer == 0 && variosCentral.bits.esCentralNueva == 0) {
		reg_sonando = reg_sonando >> 1;
	}
	
	if (variosCentral.bits.esCentralNueva == 0) {
		if (bit_test(reg_sonando, 7) && bit_test(reg_sonando, 6) == 0) {			// Empezó a sonar
			if (reg_s[0] != 0) {
				if (bit_test(sonando, 0) == 0) {
					bit_set(sonando, 0);
					timer_mandarEventoSonandoReady = 3;
				}
			}
		}
		else if (bit_test(reg_sonando, 7) == 0 && bit_test(reg_sonando, 6)) {		// Dejó de sonar
			if (reg_s[0] == 0) {
				if (bit_test(sonando, 0)) {
					bit_clear(sonando, 0);
					timer_mandarEventoSonandoReady = 3;
				}
			}
		}
	}
}


void parsear4cx (uint8_t dataL, uint8_t layer)
{
	if (variosCentral.bits.esCentralNueva == 0) {
		if (bit_test(dataL, 7)) {									// s_robo
			bit_set(hayQueDispararPorRobo, 0);
			
			if (timer_robo[0] == 0)
				timer_robo[0] = pgaData[PGA_RETARDO_P1];
		}
		if (bit_test(dataL, 6)) {									// s_incendio
			if (bit_test(incendioManual, layer)) {
				timer_disparo[layer] = 2;
				ultimaCausaDisparo[layer] = DISPARO_INCENDIO_MANUAL;
			}
			else {
				timer_disparo[layer] = 2;
				ultimaCausaDisparo[layer] = DISPARO_INCENDIO;
			}
			
			bit_clear(incendioManual, layer);
		}
		if (bit_test(dataL, 5)) {									// s_panico
				timer_disparo[layer] = 2;
				ultimaCausaDisparo[layer] = DISPARO_PANICO;
			}
		if (bit_test(dataL, 4)) {									// s_tamper
				timer_disparo[layer] = 2;
				ultimaCausaDisparo[layer] = DISPARO_TAMPER;
			}
			
		reg_s[0] = dataL;
		
	}
	else {
		reg_s[layer] = dataL;
	}
}


void parsearSonidos (uint8_t dataL, uint8_t layer)
{
	if (dataL == 0x20) {									// Activación parcial
		bit_set(vino_activacionParcial, layer);
		timer_activacionParcial[layer] = 4;					// 4 segundos
	}
	
	if (dataL == 0x80) {									// Hubo disparo de alarma
		bit_set(vino_huboDisparoDeAlarma, layer);
		timer_huboDisparoDeAlarma[layer] = 4;
	}
}


void parsearDisparos (uint8_t dataL, uint8_t layer)
{
	if (dataL == 0x10) {									// 081 - Disparo por robo
		if (contadorDisparos_robo[layer] < 4) {
			bit_set(hayQueDispararPorRobo,layer);
			
			contadorDisparos_robo[layer]++;
			if (timer_robo[layer] == 0)
				timer_robo[layer] = pgaData[PGA_RETARDO_P1 + layer];
		}
	}
	else if (dataL == 0x20) {								// 082 - Disparo por incendio
		if (contadorDisparos_incendio[layer] < 4) {
			contadorDisparos_incendio[layer]++;
			
			if (bit_test(incendioManual, layer)) {
				timer_disparo[layer] = 2;
				ultimaCausaDisparo[layer] = DISPARO_INCENDIO_MANUAL;
			}
			else {
				timer_disparo[layer] = 2;
				ultimaCausaDisparo[layer] = DISPARO_INCENDIO;
			}
			
			bit_clear(incendioManual, layer);
		}
	}
	else if (dataL == 0x30) {								// 083 - Disparo por pánico
		if (contadorDisparos_panico[layer] < 4) {
			contadorDisparos_panico[layer]++;
			
			timer_disparo[layer] = 2;
			ultimaCausaDisparo[layer] = DISPARO_PANICO;
		}
	}
	else if (dataL == 0x40) {								// 084 - Disparo por tamper
		if (contadorDisparos_tamper[layer] < 4) {
			contadorDisparos_tamper[layer]++;
			
			timer_disparo[layer] = 2;
			ultimaCausaDisparo[layer] = DISPARO_TAMPER;
		}
	}
	else if (dataL == 0x70) {								// 087 - Disparo por robo 24hs
		if (contadorDisparos_robo[layer] < 4) {
			bit_set(hayQueDispararPorRobo,layer);
			
			contadorDisparos_robo[layer]++;
			if (timer_robo[layer] == 0)
				timer_robo[layer] = pgaData[PGA_RETARDO_P1 + layer];
		}
	}
	
	variosCentral.bits.esCentralNueva = 1;
	timer_centralNueva = 0;
}


void parsearEstadoZonas (uint8_t dataH, uint8_t dataL, uint8_t layer)
{
	uint32_t numMensaje = dataH - 0x30;
	uint32_t nbit = 4 * numMensaje;
	uint8_t zonaReplay = 0;
	
	
	// Se determina la cantidad de zonas que tiene la central
	if (numMensaje >= 0) {
		if (tipoCentral.bits.tiene4zonas == 0)
			timer_mandarEventoSonandoReady = 5;
		tipoCentral.bits.tiene4zonas = 1;
	}
	if (numMensaje >= 1) {
		if (tipoCentral.bits.tiene8zonas == 0)
			timer_mandarEventoSonandoReady = 5;
		tipoCentral.bits.tiene8zonas = 1;
	}
	if (numMensaje >= 2) {
		if (tipoCentral.bits.tiene16zonas == 0)
			timer_mandarEventoSonandoReady = 5;
		tipoCentral.bits.tiene16zonas = 1;
	}
	if (numMensaje >= 4) {
		if (tipoCentral.bits.tiene32zonas == 0)
			timer_mandarEventoSonandoReady = 5;
		tipoCentral.bits.tiene32zonas = 1;
	}
		
	
	// Se analizan las 4 zonas que vinieron
	if (variosCentral.bits.enReplay == 0) {

		if(bit_test(dataL,7)) {
			if(bit_test(zonasEstado[layer], nbit) == 0) {
				bit_set(zonasEstado[layer], nbit);
				timer_mandarEstadoZonas[layer] = 3;
			}
		}
		else {
			if(bit_test(zonasEstado[layer], nbit)) {
				bit_clear(zonasEstado[layer], nbit);
				timer_mandarEstadoZonas[layer] = 3;
			}
		}
		
		if(bit_test(dataL,6)) {
			if(bit_test(zonasEstado[layer], nbit+1) == 0) {
				bit_set(zonasEstado[layer], nbit+1);
				timer_mandarEstadoZonas[layer] = 3;
			}
		}
		else {
			if(bit_test(zonasEstado[layer], nbit+1)) {
				bit_clear(zonasEstado[layer], nbit+1);
				timer_mandarEstadoZonas[layer] = 3;
			}
		}
				
		if(bit_test(dataL,5)) {
			if(bit_test(zonasEstado[layer], nbit+2) == 0) {
				bit_set(zonasEstado[layer], nbit+2);
				timer_mandarEstadoZonas[layer] = 3;
			}
		}
		else {
			if(bit_test(zonasEstado[layer], nbit+2)) {
				bit_clear(zonasEstado[layer], nbit+2);
				timer_mandarEstadoZonas[layer] = 3;
			}
		}
		
		
        if(bit_test(dataL,4)) {
			if(bit_test(zonasEstado[layer], nbit+3) == 0) {
				bit_set(zonasEstado[layer], nbit+3);
				timer_mandarEstadoZonas[layer] = 3;
			}
		}
		else {
			if(bit_test(zonasEstado[layer], nbit+3)) {
				bit_clear(zonasEstado[layer], nbit+3);
				timer_mandarEstadoZonas[layer] = 3;
			}
		}
	}
	else {
		// Solo veo las zonas de replay en el layer que me interesa
		if (layer == layerReplay) {
			if (bit_test(dataL,7))
				zonaReplay = nbit + 1;
			else if (bit_test(dataL,6))
				zonaReplay = nbit + 2;
			else if (bit_test(dataL,5))
				zonaReplay = nbit + 3;
			else if (bit_test(dataL,4))
				zonaReplay = nbit + 4;
			
			zonasReplay[indexReplay] = zonaReplay;
		
			indexReplay++;
			if(indexReplay > 7) {
				variosCentral.bits.enReplay = 0;
			
				if (variosCentral2.bits.recibiGetreplay)
					armarGetReplay(layerReplay, false);
				
				procesandoMensajes.bits.procesandoGetReplay = 0;
				variosCentral2.bits.recibiGetreplay = 0;
			}
		}
	}
}


void parsearMemoriaZonas (uint8_t dataH, uint8_t dataL, uint8_t layer)
{
	uint32_t numMensaje = dataH - 0x38;

	// Se analizan las 4 zonas que vinieron
	uint32_t nbit = 4 * numMensaje;
		
	if(bit_test(dataL,7)) {
		if(bit_test(zonasMemoria[layer], nbit) == 0) {
			bit_set(zonasMemoria[layer], nbit);
			timer_mandarMemoriaZonas[layer] = 3;
		}
	}
	else {
		if(bit_test(zonasMemoria[layer], nbit)) {
			bit_clear(zonasMemoria[layer], nbit);
			timer_mandarMemoriaZonas[layer] = 3;
		}
	}
		
	if(bit_test(dataL,6)) {
		if(bit_test(zonasMemoria[layer], nbit+1) == 0) {
			bit_set(zonasMemoria[layer], nbit+1);
			timer_mandarMemoriaZonas[layer] = 3;
		}
	}
	else {
		if(bit_test(zonasMemoria[layer], nbit+1)) {
			bit_clear(zonasMemoria[layer], nbit+1);
			timer_mandarMemoriaZonas[layer] = 3;
		}
	}
				
	if(bit_test(dataL,5)) {
		if(bit_test(zonasMemoria[layer], nbit+2) == 0) {
			bit_set(zonasMemoria[layer], nbit+2);
			timer_mandarMemoriaZonas[layer] = 3;
		}
	}
	else {
		if(bit_test(zonasMemoria[layer], nbit+2)) {
			bit_clear(zonasMemoria[layer], nbit+2);
			timer_mandarMemoriaZonas[layer] = 3;
		}
	}
		
		
    if(bit_test(dataL,4)) {
		if(bit_test(zonasMemoria[layer], nbit+3) == 0) {
			bit_set(zonasMemoria[layer], nbit+3);
			timer_mandarMemoriaZonas[layer] = 3;
		}
	}
	else {
		if(bit_test(zonasMemoria[layer], nbit+3)) {
			bit_clear(zonasMemoria[layer], nbit+3);
			timer_mandarMemoriaZonas[layer] = 3;
		}
	}
}


void parsearInclusionZonas (uint8_t dataH, uint8_t dataL, uint8_t layer)
{
	uint32_t nbit;
	uint32_t numMensaje = dataH - 0x40;

	// Se analizan las zonas que vinieron
	
		
	if (numMensaje == 0 || numMensaje == 1) {		// Zonas 1 y 2 o 3 y 4
		nbit = 2 * numMensaje;
		
		if (bit_test(dataL, 5)) {					// Incluida e Instantanea
			if (bit_test(zonasInclusion[layer], nbit) == 0 || bit_test(zonasCondicionalTemporizada[layer], nbit))
				timer_mandarInclusion[layer] = 3;
			bit_set(zonasInclusion[layer],nbit);
			bit_clear(zonasCondicionalTemporizada[layer],nbit);
		}
		else {
			if (bit_test(dataL,4)) {				// Incluida y condicional
				if (bit_test(zonasInclusion[layer], nbit) == 0 || bit_test(zonasCondicionalTemporizada[layer], nbit) == 0)
					timer_mandarInclusion[layer] = 3;
				bit_set(zonasInclusion[layer],nbit);
				bit_set(zonasCondicionalTemporizada[layer],nbit);
			}
			else {									// Excluida
				if (bit_test(zonasInclusion[layer], nbit) || bit_test(zonasCondicionalTemporizada[layer], nbit))
					timer_mandarInclusion[layer] = 3;
				bit_clear(zonasInclusion[layer],nbit);
				bit_clear(zonasCondicionalTemporizada[layer],nbit);
			}
		}
		
		if (bit_test(dataL, 7)) {					// Incluida e Instantanea
			if (bit_test(zonasInclusion[layer], nbit+1) == 0 || bit_test(zonasCondicionalTemporizada[layer], nbit+1))
				timer_mandarInclusion[layer] = 3;
			bit_set(zonasInclusion[layer],nbit+1);
			bit_clear(zonasCondicionalTemporizada[layer],nbit+1);
		}
		else {
			if (bit_test(dataL,6)) {				// Incluida y condicional
				if (bit_test(zonasInclusion[layer], nbit+1) == 0 || bit_test(zonasCondicionalTemporizada[layer], nbit+1) == 0)
					timer_mandarInclusion[layer] = 3;
				bit_set(zonasInclusion[layer],nbit+1);
				bit_set(zonasCondicionalTemporizada[layer],nbit+1);
			}
			else {									// Excluida
				if (bit_test(zonasInclusion[layer], nbit+1) || bit_test(zonasCondicionalTemporizada[layer], nbit+1))
					timer_mandarInclusion[layer] = 3;
				bit_clear(zonasInclusion[layer],nbit+1);
				bit_clear(zonasCondicionalTemporizada[layer],nbit+1);
			}
		}
	}
	else {
		nbit = 4 * (numMensaje - 1);
		
		if(bit_test(dataL,7)) {
			if(bit_test(zonasInclusion[layer], nbit) == 0) {
				bit_set(zonasInclusion[layer], nbit);
				timer_mandarInclusion[layer] = 3;
			}
		}
		else {
			if(bit_test(zonasInclusion[layer], nbit)) {
				bit_clear(zonasInclusion[layer], nbit);
				timer_mandarInclusion[layer] = 3;
			}
		}
		
		if(bit_test(dataL,6)) {
			if(bit_test(zonasInclusion[layer], nbit+1) == 0) {
				bit_set(zonasInclusion[layer], nbit+1);
				timer_mandarInclusion[layer] = 3;
			}
		}
		else {
			if(bit_test(zonasInclusion[layer], nbit+1)) {
				bit_clear(zonasInclusion[layer], nbit+1);
				timer_mandarInclusion[layer] = 3;
			}
		}
				
		if(bit_test(dataL,5)) {
			if(bit_test(zonasInclusion[layer], nbit+2) == 0) {
				bit_set(zonasInclusion[layer], nbit+2);
				timer_mandarInclusion[layer] = 3;
			}
		}
		else {
			if(bit_test(zonasInclusion[layer], nbit+2)) {
				bit_clear(zonasInclusion[layer], nbit+2);
				timer_mandarInclusion[layer] = 3;
			}
		}
		
		
		if(bit_test(dataL,4)) {
			if(bit_test(zonasInclusion[layer], nbit+3) == 0) {
				bit_set(zonasInclusion[layer], nbit+3);
				timer_mandarInclusion[layer] = 3;
			}
		}
		else {
			if(bit_test(zonasInclusion[layer], nbit+3)) {
				bit_clear(zonasInclusion[layer], nbit+3);
				timer_mandarInclusion[layer] = 3;
			}
		}
	}

}


uint8_t busco_usuario(uint8_t layer)
{
	uint8_t usuario;
	
	usuario=0;								//desactiva usuario 32, control remoto, modo ninguno
	
	if(variosCentral.bits.esCentralNueva == 0)
	{
		//if(vino_teclazo)					//activa usuario 1, modo ninguno
		//	usuario = 1;
	}
	
	if(timer_usuario[layer]< 8)
		usuario = n_usuario[layer];
		
	timer_usuario[layer] = 8;				//por las dudas, por si se vuelve a activar enseguida por master o control remoto
	
	
	return usuario;
}


void parsearEventosMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer) 
{
	if (variosCentral2.bits.recibiGetEventos == 0)
		return;
	
	if (dataH == 0b00001101)				// T_xx (registrador viejo)
		variosCentral.bits.es_9108 = 1;
	else									// U_xx (registrador nuevo)
		variosCentral.bits.es_9108 = 0;
	
	if (eventos_contadorBytesTotales < 250) {
		if (eventos_contadorBytesTotales == 0) {
			if (variosMpxh.bits.vino_pirupiru)
				variosCentral.bits.esRegistradorNuevo = 1;
		}
		
		timer_eventos = 8;
		
			
		/*
			El registrador de eventos saca 5 bytes por cada evento (saca 50 eventos) con el siguiente formato:
			 
			7                   0
			+-----------------------+
			| evento                |
			+-----------------------+
			| segundos (BCD)        |
			+-----------------------+
			| mes,0 | minutos (BCD) |
			+-----------------------+
			| mes,2,1 | horas(BCD)  |
			+-----------------------+
			| mes,4,3 | días(BCD)   |
			+-----------------------+
			 
			Al final en el buffer van a quedar 6 bytes por evento, porque se va a reconstruir el byte de mes.
		*/
		switch(eventos_contadorBytesPorEvento) {
			case 0:
			case 1:
				eventos_buffer[eventos_indexBuffer] = dataL;
				eventos_indexBuffer++;
				break;
					
			case 2:
				if (bit_test(dataL,7))
					bit_set(eventos_auxByte5,0);
				bit_clear(dataL,7);
					
				eventos_buffer[eventos_indexBuffer] = dataL;
				eventos_indexBuffer++;
				break;
					
			case 3:
				if (bit_test(dataL,6))
					bit_set(eventos_auxByte5,1);
				bit_clear(dataL,6);
					
				if (bit_test(dataL,7))
					bit_set(eventos_auxByte5,2);
				bit_clear(dataL,7);
					
				eventos_buffer[eventos_indexBuffer] = dataL;
				eventos_indexBuffer++;
				break;
					
			case 4:
				if (bit_test(dataL,6))
					bit_set(eventos_auxByte5,3);
				bit_clear(dataL,6);
					
				if (bit_test(dataL,7))
					bit_set(eventos_auxByte5,4);
				bit_clear(dataL,7);
					
				eventos_buffer[eventos_indexBuffer] = dataL;
				eventos_indexBuffer++;
					
				eventos_buffer[eventos_indexBuffer] = eventos_auxByte5;
				eventos_indexBuffer++;
				break;
		}
			
		eventos_contadorBytesPorEvento ++;
		eventos_contadorBytesTotales ++;
			
		if (eventos_contadorBytesPorEvento >= 5) {
			eventos_contadorBytesPorEvento = 0;
			eventos_auxByte5 = 0;
		}
				
		if (eventos_indexBuffer >= 30) {
			armarGetEventos(eventos_layer , 5);
				
			eventos_indexBuffer = 0;
		}	
			
		if (eventos_contadorBytesTotales >= 250) {
			procesandoMensajes.bits.procesandoGetEventos = 0;
			variosCentral2.bits.recibiGetEventos = 0;
		}
		
	}
}


void obtenerEventoYZonaUsuarioDelEvento(uint8_t* evento, uint8_t* zona_usuario)
{
	uint8_t ev = *evento;
	uint8_t numero;
	
	// Determino que evento es
	if(ev>=0 && ev<=0x1f)							// Desactiva usuario
		*evento=1;
    else if(ev>=0x20 && ev<=0x3f)					// Activa usuario
		*evento=2;
    else if(ev>=0x40 && ev<=0x5f)					// Activa estoy usuario
		*evento=3;
    else if(ev>=0x60 && ev<=0x7f)					// Activa me voy usuario
		*evento=4;  
    else if(ev>=0x80 && ev<=0x9f)					// Se registra usuario
		*evento=5;
    else if(ev>=0xa0 && ev<=0xbf)					// Asalto usuario
		*evento=6;
    else if(ev>=0xc0 && ev<=0xc7)					// Disparo zona
		*evento=7;
    else if(ev>=0xd8 && ev<=0xef)					// Disparo zona
		*evento=7;
    else if(ev==0xc8)								// Disparo pánico
		*evento=8;
    else if(ev==0xc9)								// Disparo incendio
		*evento=9;
    else if(ev==0xca)								// Disparo sabotaje
		*evento=10;			
    else if(ev==0xcb)								// Robo
		*evento=11;			
    else if(ev==0xcc)								// Emergencia médica
		*evento=12;					
    else if(ev==0xcd)								// Fin sirenas
		*evento=13;			
    else if(ev==0xce)								// Cancelación de sirenas
		*evento=14;
    else if(ev==0xcf)								// Pedido de replay
		*evento=15;
    else if(ev==0xd0)								// Borrado de memorias
		*evento=16;
    else if(ev==0xd1)								// Batería dudosa
		*evento=17;
    else if(ev==0xd2)								// Batería baja
		*evento=18;
    else if(ev==0xd3)								// Batería bien
		*evento=19;
    else if(ev==0xd4)								// Corte de red eléctrica
		*evento=20;
    else if(ev==0xd5)								// Red eléctrica normal
		*evento=21;
    else if(ev==0xd6)								// Corte de línea telefónica
		*evento=22;
    else if(ev==0xd7)								// Linea telefónica normal
		*evento=23;
    else if(ev==0xf4)								// Falla sensor/dispositivo
		*evento=24;
    else if(ev==0xf5)								// Sensor/dispositivo reestablecido
		*evento=25;
    else											// Evento no definido
		*evento=250;
		
	
	
	// Determino qué número de zona o de usuario está asociado al evento (si corresponde)
	*zona_usuario = 0;
	
	if(ev >= 0 && ev <= 0xbf) {						// Número de usuario
		
		while (ev >= 0x20) 
			ev -= 0x20;
		
		if (ev == 0)								// 0 ---> Usuario 32
			ev = 32;
		
		*zona_usuario = ev;
	}
	else if (ev >= 0xc0 && ev <= 0xc7) {			// Número de zona
		if (variosCentral.bits.es_9108) {
			ev -= 0xc0;
			
			if (ev == 0)
				ev = 8;
				
			*zona_usuario = ev;
		}
		else {
			*zona_usuario = ev - 0xc0 + 1;
		}
	}
	else if (ev >= 0xd8 && ev <= 0xef) {
		*zona_usuario = ev - 0xd8 + 9;				// Arranca en la zona 9
	}
}


void procesarSetEstado(uint8_t* payload, uint8_t payloadLen, uint8_t layer)
{
	uint8_t estadoAlarma = alarmMonitor_estadoAlarma(layer);
	uint8_t lenCode = payloadLen - 1;
	uint8_t index = 0;
	
	if (alarmMonitor_existeLayer(layer) == 0)
		return;
	
	switch (payload[0]) {
		case 1:								// Desactivada
			if ((estadoAlarma != ESTADO_ALARMA_DESACTIVADA && estadoAlarma != ESTADO_ALARMA_PROGRAMACION) || bit_test(sonando, layer)) {
				bufferTeclas[0] = 0x0b;
				
				for (uint8_t i = 1; i < (lenCode + 1); i++)
					bufferTeclas[i] = payload[i];
					
				ptrTeclas = 0;
				lenTeclas = lenCode + 1;
				bit_set(mandarTeclas,layer);
				
				procesandoMensajes.bits.procesandoSetEstado = 1;
			}
			break;
			
			
		case 2:								// Activada
			if (estadoAlarma == ESTADO_ALARMA_DESACTIVADA) {
				bufferTeclas[0] = 0x0b;
				
				for (uint8_t i = 1; i < (lenCode + 1); i++)
					bufferTeclas[i] = payload[i];
					
				ptrTeclas = 0;
				lenTeclas = lenCode + 1;
				bit_set(mandarTeclas,layer);
				
				procesandoMensajes.bits.procesandoSetEstado = 1;
			}
			break;
			
			
		case 3:								// Activada estoy
		case 4:								// Activada me voy
			if (estadoAlarma == ESTADO_ALARMA_DESACTIVADA) {
				bufferTeclas[index] = 0x0b;
				index++;
				
				// Si hay que cambiar el modo se va  mandar una vez la tecla MODO y se va a chequear si fue al modo correcto
				if ((bit_test(estoy,layer) == 0 && payload[0] == 3) || (bit_test(meVoy,layer) == 0 && payload[0] == 4)) {
					bufferTeclas[1] = 0x0d;			// Cambia de modo
					index++;
					
					if (payload[0] == 3)
						bit_set(mandandoCambioModoAEstoy,layer);
					else if (payload[0] == 4)
						bit_set(mandandoCambioModoAMeVoy,layer);
						
						
					// También se guarda el código recibido para mandarlo después
					for (uint8_t i = 1; i < (lenCode + 1); i++)
						codigoUsuarioAMandar[i-1] = payload[i];	
						
					largoCodigoUsuarioAMandar = lenCode;			
				}
				else {
					// Si no hay que cambiar de modo se manda el código de usuario
					for (uint8_t i = 1; i < (lenCode + 1); i++, index++)
						bufferTeclas[index] = payload[i];
				}
					
				ptrTeclas = 0;
				lenTeclas = index;
				bit_set(mandarTeclas,layer);
				
				procesandoMensajes.bits.procesandoSetEstado = 1;
			}
			break;
			
			
		default:
			break;
	}
}


void procesarSetInclusion(uint8_t* payload, uint8_t payloadLen, uint8_t layer) 
{
	uint8_t zona;
	uint8_t zonaBit;
	uint8_t modoInclusion;
	uint8_t i;
	uint8_t indexTecla = 0;
	uint8_t repeticionDeSecuencia;
	
	
	if (alarmMonitor_existeLayer(layer) == 0)
		return;
	
	// Solo se puede cambiar la inclusión de zonas si la alarma está desactivada.
	if (estadoAlarma[layer] != ESTADO_ALARMA_DESACTIVADA)
		return;


	// (payloadLen - 1) porque tiene que venir una cantidad par de bytes, ya que 
	// tiene que venir número de zona y tipo de inclusión
	for (i = 0; i < (payloadLen - 1); i = i+2) {
		zona = payload[i];
		zonaBit = zona - 1;
		modoInclusion = payload[i+1];
		
		
		if (modoInclusion > 2) 
			continue;
		
		// Se saltea las zonas que no corresponden al tipo de central que se tiene
		else if (zona == 0)
			continue;
		else if (zona > 32)
			continue;
		else if (zona > 16 && tipoCentral.bits.tiene32zonas == 0)
			continue;
		else if (zona > 8 && tipoCentral.bits.tiene16zonas == 0)
			continue;
		else if (zona > 4 && tipoCentral.bits.tiene8zonas == 0)
			continue;
			
		repeticionDeSecuencia = 0;
			
		/* 
		Usando la combinación de teclas Zona + XX para ir cambiando los modos de inclusión
		de una zona, la misma sigue el siguiente diagrama de estados:
		
		Zonas 1 a 4
		---------------
			Zonas 1 y 3
			--------------
			
						*----------*
				  -----?| INCLUIDA |------
				  |		*----------*	 |
				  |						 |
				  |						 ?
			*----------*			*-------------*
			| EXCLUIDA |?-----------| TEMPORIZADA |
			*----------*			*-------------*
			
		
			Zonas 2 y 4
			--------------
						
						  *----------*
				---------?| EXCLUIDA |-----------
				| 		  *----------*			|
				|								|
				|								|
				|	*----------------------*	|
				----| INCLUIDA/CONDICIONAL |?----
					*----------------------*
					
					
			Zonas 5 a 32
			--------------
			
						  *----------*
				    -----?| EXCLUIDA |-------
					|	  *----------*		|
					|						|
					|						|
					|	  *---------*		|
					------| INCLUIDA |?------
						  *----------*
						
		*/
		if (zona == 1 || zona == 3) {
			
			if (modoInclusion == 0) {											// Excluyendo
				if (bit_test(zonasCondicionalTemporizada[layer], zonaBit))  	// Está temporizada
					repeticionDeSecuencia = 1;
				else if (bit_test(zonasInclusion[layer], zonaBit))				// Está incluida
					repeticionDeSecuencia = 2;
				else															// Está excluida
					repeticionDeSecuencia = 0;
			}
			else if (modoInclusion == 1) {										// Incluyendo
				if (bit_test(zonasCondicionalTemporizada[layer], zonaBit))  	// Está temporizada
					repeticionDeSecuencia = 2;
				else if (bit_test(zonasInclusion[layer], zonaBit))				// Está incluida
					repeticionDeSecuencia = 0;
				else															// Está excluida
					repeticionDeSecuencia = 1;
			}
			else {																// Temporizando
				if (bit_test(zonasCondicionalTemporizada[layer], zonaBit))  	// Está temporizada
					repeticionDeSecuencia = 0;
				else if (bit_test(zonasInclusion[layer], zonaBit))				// Está incluida
					repeticionDeSecuencia = 1;
				else															// Está excluida
					repeticionDeSecuencia = 2;
			}
		}
		else if (zona == 2 || zona == 4) {
			if (modoInclusion == 0)	{										// Excluyendo
				if (bit_test(zonasInclusion[layer], zonaBit) == 0)			// Está excluida
					repeticionDeSecuencia = 0;
				else														// Está incluida o incluida/condicional
					repeticionDeSecuencia = 1;
			}
			else if (modoInclusion == 1) {									// Incluyendo
				if (bit_test(zonasInclusion[layer], zonaBit) == 0)			// Está excluida
					repeticionDeSecuencia = 1;
				else														// Está incluida o incluida/condicional
					repeticionDeSecuencia = 0;
			}
			else {															// Haciendola condicional
				if (bit_test(zonasInclusion[layer], zonaBit) == 0)			// Está excluida
					repeticionDeSecuencia = 1;
				else														// Está incluida o incluida/condicional
					repeticionDeSecuencia = 0;
			}
		}
		else {
			if ((modoInclusion == 0 && bit_test(zonasInclusion[layer], zonaBit)) ||
				(modoInclusion == 1 && bit_test(zonasInclusion[layer], zonaBit) == 0))
				repeticionDeSecuencia = 1;	

		}
		
		
		// Se cargan en el buffer de teclas tantas secuencias Z xx 
		// como repeticionDeSecuencia indica
		for (uint8_t repeticion = 0; repeticion < repeticionDeSecuencia; repeticion ++) {
			bufferTeclas[indexTecla] = 0x0c;
			bufferTeclas[indexTecla+1] = zona / 10;
			bufferTeclas[indexTecla+2] = zona % 10;
				
			indexTecla += 3;
		}
	}
	
	if (indexTecla > 0) {
		// Se agrega una F al final para que la central salga del modo de inclusión de zonas
		bufferTeclas[indexTecla] = 0x0b;
		indexTecla++;
		
		ptrTeclas = 0;
		lenTeclas = indexTecla;
		bit_set(mandarTeclas,layer);
		procesandoMensajes.bits.procesandoSetInclusion = 1;
	}
	
}


void procesarSetSonandoReady(uint8_t* payload, uint8_t payloadLen, uint8_t layer) 
{
	if (payloadLen < 1)
		return;
		
	if (payload[0] == 1) {								// Poniendo en estoy
		if (bit_test(estoy, layer) == 0) {
			bufferTeclas[0] = 0x0b;
			bufferTeclas[1] = 0x0d;
			ptrTeclas = 0;
			lenTeclas = 2;
			bit_set(mandarTeclas,layer);
			
			bit_set(mandandoCambioModoAEstoy,layer);
			largoCodigoUsuarioAMandar = 0;				// Es para que no mande nada cuando termina de cambiar el modo
		}
	}
	else if (payload[0] == 2) {							// Poniendo en me voy
		if (bit_test(meVoy, layer) == 0) {
			bufferTeclas[0] = 0x0b;
			bufferTeclas[1] = 0x0d;
			ptrTeclas = 0;
			lenTeclas = 2;
			bit_set(mandarTeclas,layer);
			
			bit_set(mandandoCambioModoAMeVoy,layer);
			largoCodigoUsuarioAMandar = 0;				// Es para que no mande nada cuando termina de cambiar el modo
		}
	}
}


void procesarComandoDisparar(uint8_t* payload, uint8_t payloadLen, uint8_t layer)
{
	if (payloadLen != 1)
		return;
		
	if (payload[0] > 3)
		return;
		
	switch(payload[0]) {
		case 0:												// No definido		
			break;
			
			
		case 1:												// Pánico
			bit_set(mandarPanico, layer);
			break;
			
			
		case 2:												// Incendio
			bit_set(mandarIncendioManual, layer);
			bit_set(incendioManual, layer);
			break;
			
			
		case 3:												// Emergencia médica
			if(estadoAlarma[layer] != ESTADO_ALARMA_PROGRAMACION && 
					bit_test(perifericoEnProgramacion, layer) == 0) {
						
				timer_disparo[layer] = 2;
				ultimaCausaDisparo[layer] = DISPARO_EMERGENCIA_MEDICA;
				bit_set(mandarAgudoIncondicional,layer);
				
				bit_set(mandarEmergenciaMedica, layer);
			}
			break;
	}
}