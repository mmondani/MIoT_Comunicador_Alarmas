#include "inc/dateTime.h"
#include "inc/mpxhTimeDate.h"
#include "inc/dateTime.h"
#include "inc/WiFiManager.h"
#include "inc/socketManager.h"
#include "inc/softTimers.h"
#include "inc/mpxh.h"
#include "inc/project.h"
#include "inc/imClient_cmds_regs.h"
#include "inc/PGA_Map.h"
#include "inc/pga.h"
#include "inc/EEPROM_MAP.h"


static union ProcesandoMensajes{
	uint32_t w;
	struct {
		uint8_t procesandoSetFechayHora:1;
		uint8_t bit1:1;
		uint8_t bit2:1;
		uint8_t bit3:1;
		uint8_t bit4:1;
		uint8_t bit5:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
	} bits;
} procesandoMensajes;

union VariosDateTime{
	uint8_t byte;
	struct {
		uint8_t validTime:1;
		uint8_t bit1:1;
		uint8_t needToRequestTimeDate:1;
		uint8_t llegoTiempoServidor:1;
		uint8_t soyPatron097:1;
		uint8_t hayOtroPatron097:1;
		uint8_t tengoQueSincronizarAhora:1;
		uint8_t bit7:1;
	} bits;
} variosDateTime;

union ApDateTime{
	uint8_t byte;
	struct {
		uint8_t vino_097:1;
		uint8_t vino_0a8:1;
		uint8_t vino_0a9:1;
		uint8_t vino_0aa:1;
		uint8_t vino_0cx:1;
		uint8_t vino_0a6:1;
		uint8_t vino_0a7:1;
		uint8_t vino_625:1;
	} bits;
} apDateTime;

union ApDateTime2{
	uint8_t byte;
	struct {
		uint8_t ap_minuto:1;
		uint8_t ap_minuto_20_segs:1;
		uint8_t ap_hora:1;
		uint8_t ap_nuevo_dia:1;
		uint8_t ap_vencioTimerSincroInternet:1;
		uint8_t ap_sincronizarNo:1;
		uint8_t vinoComandoSincronizar:1;
		uint8_t bit7:1;
	} bits;
} apDateTime2;


uint8_t timer_mandarEventoConfiguracionTiempo;



/**************************************************************/
// FSM principal para el manejo de la lógica para obtener la
// fecha y hora desde un servidor NTP
/**************************************************************/
typedef enum    {
	dateTime_init_state = 0,
	dateTime_pedirFyh,
	dateTime_idle,
	dateTime_esperarRespuesta
}dateTime_state_t;
static dateTime_state_t fsmState;
static dateTime_state_t fsmState_previous;

static bool stateIn = true;
static bool stateOut = false;

static SoftTimer_t timerState;
mpxhTimeDate_t mpxhTimeDate;
dateTime_t dateTimeReceived;
uint8_t timer_sincronizacionInternet;

static void dateTime_gotoState (dateTime_state_t nextState);



/**************************************************************/
// FSM para determinar quíen es el patrón de tiempo.
/**************************************************************/
typedef enum    {
	dateTime_patron_esperandoHoraValida = 0,
	dateTime_patron_esperandoSacarHora,
	dateTime_patron_esperandoSacarFechaHora,
	dateTime_patron_esperandoRespuesta0A8,
	dateTime_patron_otroPatronrespondiendo,
	dateTime_patron_esperandoOtroPatron,
	dateTime_patron_esperandoSiguientesMensajes,
	dateTime_patron_soyPatron
}dateTime_patron_state_t;
static dateTime_patron_state_t fsmPatronState;
static dateTime_patron_state_t fsmPatronState_previous;

static bool patron_stateIn = true;
static bool patron_stateOut = false;

static SoftTimer_t patron_timerState;

static void dateTime_patron_gotoState (dateTime_patron_state_t nextState);
static void patron_pisarFechaHora (void);


static void socket_received (SOCKET id, tstrSocketRecvMsg* msg);
static void socket_closed (SOCKET id);
static void dateTimeToMpxhTimeDate (dateTime_t* dt, mpxhTimeDate_t* mpxhTd);
static void mpxhTimeDateToDateTime (mpxhTimeDate_t* mpxhTd, dateTime_t* dt);

static void procesarSetFecha(uint8_t* payload, uint8_t payloadLen);
static void procesarSetHora(uint8_t* payload, uint8_t payloadLen);
static void procesarSetfechaHora(uint8_t* payload, uint8_t payloadLen);
static void armarPedirFechaHora (void);
static void procesarRespPedirFechaHora(uint8_t* payload, uint8_t payloadLen);

static bool esHoraValida (uint8_t hora, uint8_t minutos, uint8_t segundos);
static bool esFechaValida (uint8_t dia, uint8_t mes, uint8_t anio);


void dateTime_init (void)
{
	mpxhTimeDate_init();
	
	
	variosDateTime.bits.validTime = 0;
	variosDateTime.bits.needToRequestTimeDate = 0;
	
	fsmState = dateTime_init_state;
	fsmState_previous = dateTime_init_state;
}


void dateTime_now (dateTime_t* dateTime)
{
	mpxhTimeDate_getTimeDate(&mpxhTimeDate);
		
	dateTime->dia = mpxhTimeDate.d[MPXH_DATE_DIA2] * 10 + mpxhTimeDate.d[MPXH_DATE_DIA1];
	dateTime->mes = mpxhTimeDate.d[MPXH_DATE_MES2] * 10 + mpxhTimeDate.d[MPXH_DATE_MES1];
	dateTime->anio = mpxhTimeDate.d[MPXH_DATE_ANIO2] * 10 + mpxhTimeDate.d[MPXH_DATE_ANIO1];
	dateTime->horas = mpxhTimeDate.t[MPXH_TIME_HORA2] * 10 + mpxhTimeDate.t[MPXH_TIME_HORA1];
	dateTime->minutos = mpxhTimeDate.t[MPXH_TIME_MIN2] * 10 + mpxhTimeDate.t[MPXH_TIME_MIN1];
	dateTime->segundos = mpxhTimeDate.t[MPXH_TIME_SEG2] * 10 + mpxhTimeDate.t[MPXH_TIME_SEG1];
}


void dateTime_setSincronizarInternet (uint8_t sincronizar)
{
	if (sincronizar != pgaData[PGA_SINCRO_INTERNET]) {
		pgaData[PGA_SINCRO_INTERNET] = sincronizar;
		
		if(sincronizar == 0) {
			comandos1.bits.mandar_624 = 1;
			apDateTime2.bits.ap_sincronizarNo = 1;
		}
		else {
			timer_sincronizacionInternet = 0;
			comandos1.bits.mandar_623 = 1;
		}
		
		pga_enqueueSave(PGA_SINCRO_INTERNET, 1, EE_SINCRO_INTERNET_ADDR);
		
		timer_mandarEventoConfiguracionTiempo = 2;
	}
}


uint8_t dateTime_getSincronizarInternet (void)
{
	return pgaData[PGA_SINCRO_INTERNET];
}


void dateTime_setRegionCode (uint8_t code)
{
	if (pgaData[PGA_SINCRO_INTERNET] == 1 && code != pgaData[PGA_CODIGO_REGION]) {
		pgaData[PGA_CODIGO_REGION] = code;
		
		pga_enqueueSave(PGA_CODIGO_REGION, 1, EE_CODIGO_REGION_ADDR);
		
		timer_mandarEventoConfiguracionTiempo = 2;
	}
}


uint8_t dateTime_getRegionCode (void)
{
	return pgaData[PGA_CODIGO_REGION];
}


void dateTime_handler (void) 
{
	switch (fsmState) {
		case dateTime_init_state:
			if (stateIn)
            {				
                stateIn = false;
                stateOut = false;
				
				variosDateTime.bits.needToRequestTimeDate = 1;
				variosDateTime.bits.tengoQueSincronizarAhora = 1;
            }

            //**********************************************************************************************
			
			if (pgaData[PGA_SINCRO_INTERNET] == 1) {
				dateTime_gotoState(dateTime_idle);
			}
			else {
				dateTime_gotoState(dateTime_pedirFyh);
			}
			
            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
		
		
		case dateTime_pedirFyh:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				softTimer_init(&timerState, 10000);
				
				comandos1.bits.pedir_fyh = 1;
            }

            //**********************************************************************************************
			
			if (mpxhTimeDate_newTimeDate()) {
				dateTime_gotoState(dateTime_idle);
				variosDateTime.bits.needToRequestTimeDate = 0;
				variosDateTime.bits.tengoQueSincronizarAhora = 0;
				variosDateTime.bits.validTime = 1;
				
				// Cuando termina de llegar el tiempo por MPXH se manda un evento de fecha y hora
				dateTime_armarEventoFechaHora();
			}
			else if (softTimer_expired(&timerState)) {
				dateTime_gotoState(dateTime_idle);
				variosDateTime.bits.needToRequestTimeDate = 1;
				variosDateTime.bits.tengoQueSincronizarAhora = 0;
			}
			
            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
		
		
		case dateTime_idle:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				if (variosDateTime.bits.needToRequestTimeDate) {
					softTimer_init(&timerState, 60000);
				}
			}
            //**********************************************************************************************
			
			if (pgaData[PGA_SINCRO_INTERNET] == 1) {
				// Si está habilitado el sincronizar por internet
				// todos los días a las 23:55:00 pide la hora al server NTP.
				if (wifiManager_isWiFiConnected()) {
					mpxhTimeDate_getTimeDate(&mpxhTimeDate);
					if (mpxhTimeDate.t[MPXH_TIME_HORA2] == 2 &&
					mpxhTimeDate.t[MPXH_TIME_HORA1] == 3 &&
					mpxhTimeDate.t[MPXH_TIME_SEG2] == 5 &&
					mpxhTimeDate.t[MPXH_TIME_SEG2] == 5 ) {
						if (imClient_isClientConnected()) {
							armarPedirFechaHora();
							dateTime_gotoState(dateTime_esperarRespuesta);
						}
						else {
							variosDateTime.bits.needToRequestTimeDate = 1;
							dateTime_gotoState(dateTime_idle);
						}
							
					}
					else if ((softTimer_expired(&timerState) && variosDateTime.bits.needToRequestTimeDate) || variosDateTime.bits.tengoQueSincronizarAhora) {
						if (imClient_isClientConnected()) {
							variosDateTime.bits.tengoQueSincronizarAhora = 0;
							
							armarPedirFechaHora();
							dateTime_gotoState(dateTime_esperarRespuesta);
						}
					}
				}
			}
			else {
				if ((softTimer_expired(&timerState) && variosDateTime.bits.needToRequestTimeDate) || variosDateTime.bits.tengoQueSincronizarAhora) {
					// Se vuelve a pedir la hora por MPXH cada 60s
					dateTime_gotoState(dateTime_pedirFyh);
				}
			}
			
			if (mpxhTimeDate_newTimeDate()) {
				// Cuando termina de llegar el tiempo por MPXH se manda un evento de fecha y hora
				dateTime_armarEventoFechaHora();
			}
			
            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case dateTime_esperarRespuesta:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				softTimer_init(&timerState, 5000);
				variosDateTime.bits.llegoTiempoServidor = 0;
            }

            //**********************************************************************************************
			if (softTimer_expired(&timerState)) {
				dateTime_gotoState(dateTime_idle);
				//TO-DO marcar error
			}
			else if (variosDateTime.bits.llegoTiempoServidor) {			
				mpxhTimeDate.d[MPXH_DATE_DIA2] = dateTimeReceived.dia / 10;
				mpxhTimeDate.d[MPXH_DATE_DIA1] = dateTimeReceived.dia % 10;
				mpxhTimeDate.d[MPXH_DATE_MES2] = dateTimeReceived.mes / 10;
				mpxhTimeDate.d[MPXH_DATE_MES1] = dateTimeReceived.mes % 10;
				mpxhTimeDate.d[MPXH_DATE_ANIO4] = 2;
				mpxhTimeDate.d[MPXH_DATE_ANIO3] = 0;
				mpxhTimeDate.d[MPXH_DATE_ANIO2] = dateTimeReceived.anio / 10;
				mpxhTimeDate.d[MPXH_DATE_ANIO1] = dateTimeReceived.anio % 10;
				
				mpxhTimeDate.t[MPXH_TIME_HORA2] = dateTimeReceived.horas / 10;
				mpxhTimeDate.t[MPXH_TIME_HORA1] = dateTimeReceived.horas % 10;
				mpxhTimeDate.t[MPXH_TIME_MIN2] = dateTimeReceived.minutos / 10;
				mpxhTimeDate.t[MPXH_TIME_MIN1] = dateTimeReceived.minutos % 10;
				mpxhTimeDate.t[MPXH_TIME_SEG2] = dateTimeReceived.segundos / 10;
				mpxhTimeDate.t[MPXH_TIME_SEG1] = dateTimeReceived.segundos % 10;
				
				mpxhTimeDate_setTimeDate(&mpxhTimeDate);
					
				variosDateTime.bits.validTime = 1;
				timer_sincronizacionInternet = 0;
				variosDateTime.bits.needToRequestTimeDate = 0;
				
				// Cuando termina de llegar el tiempo por el server NTP se manda un evento de fecha y hora
				dateTime_armarEventoFechaHora();
				comandos1.bits.mandar_626 = 1;
				
				dateTime_gotoState(dateTime_idle);
			}
            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
	}
}


void dateTime_gotoState (dateTime_state_t nextState)
{
	fsmState_previous = fsmState;
	fsmState = nextState;
	
	stateIn = false;
	stateOut = true;
}


void dateTime_fsmPatronTiempo (void)
{
	
	if (fsmPatronState == dateTime_patron_esperandoHoraValida ||
		fsmPatronState == dateTime_patron_esperandoRespuesta0A8 ||
		fsmPatronState == dateTime_patron_otroPatronrespondiendo ||
		fsmPatronState == dateTime_patron_esperandoOtroPatron ||
		fsmPatronState == dateTime_patron_esperandoSiguientesMensajes ||
		fsmPatronState == dateTime_patron_soyPatron) {
		
		if (apDateTime2.bits.vinoComandoSincronizar) {
			variosDateTime.bits.needToRequestTimeDate = 1;
			variosDateTime.bits.tengoQueSincronizarAhora = 1;
			comandos2.bits.mandar_625 = 1;
			
			variosDateTime.bits.validTime = 0;
			dateTime_patron_gotoState(dateTime_patron_esperandoHoraValida);
		}		
		else if (apDateTime.bits.vino_625) {
			variosDateTime.bits.needToRequestTimeDate = 1;
			variosDateTime.bits.tengoQueSincronizarAhora = 1;
			
			variosDateTime.bits.validTime = 0;
			dateTime_patron_gotoState(dateTime_patron_esperandoHoraValida);
		}
		else if (apDateTime2.bits.ap_vencioTimerSincroInternet || apDateTime2.bits.ap_sincronizarNo) {
			variosDateTime.bits.validTime = 0;
			dateTime_patron_gotoState(dateTime_patron_esperandoHoraValida);
		}
	}
		
		
	switch (fsmPatronState) {
		case dateTime_patron_esperandoHoraValida:
			if (patron_stateIn)
            {				
                patron_stateIn = false;
                patron_stateOut = false;
				
				variosDateTime.bits.soyPatron097 = 0;
				variosDateTime.bits.hayOtroPatron097 = 1;
            }

            //**********************************************************************************************
			if (dateTime_hasValidTime() && pgaData[PGA_SINCRO_INTERNET] == 1) {
				comandos2.bits.mandar_0a8 = 1;
				dateTime_patron_gotoState(dateTime_patron_esperandoRespuesta0A8);
			}
			else if (apDateTime2.bits.ap_hora) {
				dateTime_patron_gotoState(dateTime_patron_esperandoSacarHora);
			}
			else if (apDateTime2.bits.ap_nuevo_dia) {
				dateTime_patron_gotoState(dateTime_patron_esperandoSacarFechaHora);
			}
			else if (apDateTime.bits.vino_625) {
				variosDateTime.bits.needToRequestTimeDate = 1;
				
				variosDateTime.bits.validTime = 0;
				dateTime_patron_gotoState(dateTime_patron_esperandoHoraValida);
			}
            //**********************************************************************************************
            if (patron_stateOut)
            {
                patron_stateIn = true;
                patron_stateOut = false;
            }

			break;
			
			
		case dateTime_patron_esperandoSacarHora:
			if (patron_stateIn)
            {				
                patron_stateIn = false;
                patron_stateOut = false;
				
				softTimer_init(&patron_timerState, 10000);
            }

            //**********************************************************************************************
			if (apDateTime.bits.vino_097 || apDateTime.bits.vino_0a9 || apDateTime.bits.vino_0aa || apDateTime.bits.vino_0cx)
				dateTime_patron_gotoState(dateTime_patron_esperandoHoraValida);
			else if (softTimer_expired(&patron_timerState)) {
				mpxhTimeDate_empezarSacarHora();
				dateTime_patron_gotoState(dateTime_patron_esperandoHoraValida);
			}	
            //**********************************************************************************************
            if (patron_stateOut)
            {
                patron_stateIn = true;
                patron_stateOut = false;
            }

			break;
			
			
		case dateTime_patron_esperandoSacarFechaHora:
			if (patron_stateIn)
            {				
                patron_stateIn = false;
                patron_stateOut = false;
				
				softTimer_init(&patron_timerState, 10000);
            }

            //**********************************************************************************************
			if (apDateTime.bits.vino_097 || apDateTime.bits.vino_0a9 || apDateTime.bits.vino_0aa || apDateTime.bits.vino_0cx)
				dateTime_patron_gotoState(dateTime_patron_esperandoHoraValida);
			else if (softTimer_expired(&patron_timerState)) {
				mpxhTimeDate_empezarSacarFyh();
				dateTime_patron_gotoState(dateTime_patron_esperandoHoraValida);
			}
            //**********************************************************************************************
            if (patron_stateOut)
            {
                patron_stateIn = true;
                patron_stateOut = false;
            }

			break;
			
			
		case dateTime_patron_esperandoRespuesta0A8:
			if (patron_stateIn)
            {				
                patron_stateIn = false;
                patron_stateOut = false;
				
				softTimer_init(&patron_timerState, 4000);
            }

            //**********************************************************************************************
			if (apDateTime.bits.vino_097)
				dateTime_patron_gotoState(dateTime_patron_otroPatronrespondiendo);
			else if (apDateTime.bits.vino_0a8 || apDateTime.bits.vino_0a9 || apDateTime.bits.vino_0aa || apDateTime.bits.vino_0cx) {
				dateTime_patron_gotoState(dateTime_patron_soyPatron);
				patron_pisarFechaHora();
			}
			else if (softTimer_expired(&patron_timerState)) {
				dateTime_patron_gotoState(dateTime_patron_soyPatron);
				patron_pisarFechaHora();
			}
				
            //**********************************************************************************************
            if (patron_stateOut)
            {
                patron_stateIn = true;
                patron_stateOut = false;
            }

			break;
			
			
		case dateTime_patron_otroPatronrespondiendo:
			if (patron_stateIn)
            {				
                patron_stateIn = false;
                patron_stateOut = false;
				
				variosDateTime.bits.soyPatron097 = 0;
				variosDateTime.bits.hayOtroPatron097 = 1;
				
				softTimer_init(&patron_timerState, 10000);
            }

            //**********************************************************************************************
			if (apDateTime.bits.vino_097 || apDateTime.bits.vino_0a9 || apDateTime.bits.vino_0aa || apDateTime.bits.vino_0cx)
				softTimer_init(&patron_timerState, 10000);
			else if (softTimer_expired(&patron_timerState))
				dateTime_patron_gotoState(dateTime_patron_esperandoOtroPatron);
			
            //**********************************************************************************************
            if (patron_stateOut)
            {
                patron_stateIn = true;
                patron_stateOut = false;
            }

			break;
			
			
		case dateTime_patron_esperandoOtroPatron:
			if (patron_stateIn)
            {				
                patron_stateIn = false;
                patron_stateOut = false;
				
            }

            //**********************************************************************************************
			if (apDateTime.bits.vino_097)
				dateTime_patron_gotoState(dateTime_patron_otroPatronrespondiendo);
			else if (apDateTime.bits.vino_0a8 || apDateTime.bits.vino_0a9 || apDateTime.bits.vino_0aa || apDateTime.bits.vino_0cx)
				dateTime_patron_gotoState(dateTime_patron_esperandoSiguientesMensajes);
			
            //**********************************************************************************************
            if (patron_stateOut)
            {
                patron_stateIn = true;
                patron_stateOut = false;
            }

			break;
			
			
		case dateTime_patron_esperandoSiguientesMensajes:
			if (patron_stateIn)
            {				
                patron_stateIn = false;
                patron_stateOut = false;
				
				softTimer_init(&patron_timerState, 4000);
            }

            //**********************************************************************************************
			if (apDateTime.bits.vino_097)
				dateTime_patron_gotoState(dateTime_patron_otroPatronrespondiendo);
			else if (apDateTime.bits.vino_0a9  || apDateTime.bits.vino_0aa || apDateTime.bits.vino_0cx) {
				dateTime_patron_gotoState(dateTime_patron_soyPatron);
				patron_pisarFechaHora();
			}
			else if (softTimer_expired(&patron_timerState))
				dateTime_patron_gotoState(dateTime_patron_soyPatron);
            //**********************************************************************************************
            if (patron_stateOut)
            {
                patron_stateIn = true;
                patron_stateOut = false;
            }

			break;
			
			
		case dateTime_patron_soyPatron:
			if (patron_stateIn)
            {				
                patron_stateIn = false;
                patron_stateOut = false;
				
				variosDateTime.bits.soyPatron097 = 1;
				variosDateTime.bits.hayOtroPatron097 = 0;
            }

            //**********************************************************************************************
			if (apDateTime.bits.vino_0a8  ||apDateTime.bits.vino_0a9  || apDateTime.bits.vino_0aa || apDateTime.bits.vino_0cx ||
				apDateTime.bits.vino_0a6  || apDateTime.bits.vino_0a7 )
				patron_pisarFechaHora();
			else if (apDateTime.bits.vino_097)
				dateTime_patron_gotoState(dateTime_patron_otroPatronrespondiendo);
			else {
				if (apDateTime2.bits.ap_minuto) {
					comandos1.bits.mandar_0a6 = 1;
					comandos1.bits.mandar_0a7 = 1;
				}
				else if (apDateTime2.bits.ap_minuto_20_segs) {
					comandos1.bits.mandar_0a7 = 1;
				}
				else if (apDateTime2.bits.ap_hora) {
					comandos1.bits.mandar_097 = 1;
					mpxhTimeDate_empezarSacarHora();
				}
				else if (apDateTime2.bits.ap_nuevo_dia) {
					comandos1.bits.mandar_097 = 1;
					mpxhTimeDate_empezarSacarFyh();
				}
			}
            //**********************************************************************************************
            if (patron_stateOut)
            {
                patron_stateIn = true;
                patron_stateOut = false;
            }

			break;
			
	}
	
	
	apDateTime.byte = 0;
	apDateTime2.byte = 0;
}

void patron_pisarFechaHora (void)
{
	comandos1.bits.mandar_097 = 1;
	mpxhTimeDate_empezarSacarFyh();
}


static void dateTime_patron_gotoState (dateTime_patron_state_t nextState)
{
	fsmPatronState_previous = fsmState;
	fsmPatronState = nextState;
	
	patron_stateIn = false;
	patron_stateOut = true;
}


// Debe ser llamada cada 4ms
void dateTime_timeKeepingHandler (void)
{
	mpxhTimeDate_handler();
}


void dateTime_timers1s_handler (void)
{
	dateTime_t dateTime;
	
	dateTime_getTimeDate(&dateTime);
				
	if (dateTime.horas == 0 && dateTime.minutos == 0 && dateTime.segundos == 0)
		apDateTime2.bits.ap_nuevo_dia = 1;
	else if (dateTime.minutos == 0 && dateTime.segundos == 0)
		apDateTime2.bits.ap_hora = 1;
					
	if (dateTime.segundos == 0) {			// En cada minuto redondo mando 0A6 y 0A7
		apDateTime2.bits.ap_minuto = 1;
	}
	else if (dateTime.segundos == 20) {		// A los 20 segundos solo mando 0A7 para resetear 
											// al timer de la central y que no saque 0A6
		apDateTime2.bits.ap_minuto_20_segs = 1;
	}
	
	
	// 2 segundos para cuando me cambian la configuración de SINCRO o de la región
	// Pasado este tiempo, manda el evento de la configuración y simula que vino el 
	// comando de sincronización para que se vuelva a pedir la fecha al servidor.
	if (timer_mandarEventoConfiguracionTiempo > 0) {
		timer_mandarEventoConfiguracionTiempo --;
		
		if (timer_mandarEventoConfiguracionTiempo == 0) {
			
			if (pgaData[PGA_SINCRO_INTERNET] == 1) {
				apDateTime2.bits.vinoComandoSincronizar = 1;
			}
		}
	}
}


void dateTime_timers1h_handler (void)
{	
	if (pgaData[PGA_SINCRO_INTERNET] == 1)
		comandos1.bits.mandar_623 = 1;
		
	if (pgaData[PGA_SINCRO_INTERNET] == 1) {
		// 50 horas sin obtener hora de internet, considero que la hora no es precisa
		if (timer_sincronizacionInternet < 50) {
			timer_sincronizacionInternet++;
		
			if (timer_sincronizacionInternet >= 50) {
				apDateTime2.bits.ap_vencioTimerSincroInternet = 1;
			}
		}
	}
}


bool dateTime_hasValidTime(void) 
{
	return (variosDateTime.bits.validTime == 1);
}


bool dateTime_soyPatron(void)
{
	return (variosDateTime.bits.soyPatron097 == 1);
}


void dateTime_setTimeDate (dateTime_t* newTimeDate)
{
	mpxhTimeDate_t mpxhTimeDate;
	
	if (fsmPatronState == dateTime_patron_esperandoHoraValida ||
		fsmPatronState == dateTime_patron_esperandoSacarHora ||
		fsmPatronState == dateTime_patron_esperandoSacarFechaHora) {
			
		dateTimeToMpxhTimeDate(newTimeDate, &mpxhTimeDate);
		mpxhTimeDate_setTimeDate(&mpxhTimeDate);
		mpxhTimeDate_empezarSacarFyh();
	}
}


void dateTime_setTime (dateTime_t* newTime)
{
	mpxhTimeDate_t mpxhTimeDate;
	
	if (fsmPatronState == dateTime_patron_esperandoHoraValida ||
		fsmPatronState == dateTime_patron_esperandoSacarHora ||
		fsmPatronState == dateTime_patron_esperandoSacarFechaHora) {
			
		dateTimeToMpxhTimeDate(newTime, &mpxhTimeDate);
		mpxhTimeDate_setTime(&mpxhTimeDate);
		mpxhTimeDate_empezarSacarFyh();
	}
}


void dateTime_setDate (dateTime_t* newDate)
{
	mpxhTimeDate_t mpxhTimeDate;
	
	if (fsmPatronState == dateTime_patron_esperandoHoraValida ||
		fsmPatronState == dateTime_patron_esperandoSacarHora ||
		fsmPatronState == dateTime_patron_esperandoSacarFechaHora) {
			
		dateTimeToMpxhTimeDate(newDate, &mpxhTimeDate);
		mpxhTimeDate_setDate(&mpxhTimeDate);
		mpxhTimeDate_empezarSacarFyh();
	}
}


void dateTime_getTimeDate (dateTime_t* timeDate)
{
	mpxhTimeDate_t mpxhTimeDate;
	
	mpxhTimeDate_getTimeDate(&mpxhTimeDate);
	mpxhTimeDateToDateTime(&mpxhTimeDate, timeDate);
}


void dateTime_analizarMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer, uint8_t nbits)
{
	switch (nbits) {
		case MPXH_BITS_17:
		case MPXH_BITS_16:
		
			if (dataH == 0x01 && dataL == 0xd0 && layer == 0) {				// 01D - Pedir status a la central
				if (pgaData[PGA_SINCRO_INTERNET] == 1)
					comandos1.bits.mandar_623 = 1;
			}
			else if (dataH == 0x09 && dataL == 0x70 && layer == 0) {		// 097 - Vino soy GPS (o patrón de hora "fuerte")
				apDateTime.bits.vino_097 = 1;
			}
			else if (dataH == 0x0a) {			
				if (dataL == 0x60) {										// 0A6 - Minuto patrón
					apDateTime.bits.vino_0a6 = 1;
					
					if (variosDateTime.bits.soyPatron097 == 0)
						mpxhTimeDate_vinoMinutoPatron();
				}
				else if (dataL == 0x70) {									// 0A7 - Reset minuto patrón
					apDateTime.bits.vino_0a7 = 1;
				}
				else if (dataL == 0x90) {									// 0A9 - Empieza fecha y hora (van a venir 14 nibbles)
					apDateTime.bits.vino_0a9 = 1;
					
					// Aunque sea patrón o después lo vaya a pisar, igual reseteo el tiempo
					// si lo tiene que pisar la fsm para definir el patrón se va a encargar.
					mpxhTimeDate_resetTimeDate();
					mpxhTimeDate_detenerSacarFyh();	
					comandos1.bits.mandar_626 = 0;
					// TO-DO revisar si lo que está saliendo por MPXH es algo relacionado con la fecha/hora

				}
				else if (dataL == 0x80) {									// 0A8 - Pedir fecha y hora
					apDateTime.bits.vino_0a8 = 1;
				}
				else if (dataL == 0xa0) {									// 0AA - Empieza la hora (van a venir 6 nibbles)
					apDateTime.bits.vino_0aa = 1;
					
					// Aunque sea patrón o después lo vaya a pisar, igual reseteo la fecha y el tiempo
					// si lo tiene que pisar la fsm para definir el patrón se va a encargar.
					mpxhTimeDate_resetTime();
					mpxhTimeDate_detenerSacarFyh();	
					comandos1.bits.mandar_626 = 0;
					// TO-DO revisar si lo que está saliendo por MPXH es algo relacionado con la fecha/hora
				}
			}
			else if (dataH == 0x0c) {										// 0Cx - Nibble fecha u hora
				apDateTime.bits.vino_0cx = 1;
				
				mpxhTimeDate_newNibble(dataL);
			}
			else if (dataH == 0x62 && dataL == 0x50) {						// 625 - Sincronizar ahora
				if (pgaData[PGA_SINCRO_INTERNET] == 1) {
					//variosDateTime.bits.needToRequestTimeDate = 1;
					apDateTime.bits.vino_625 = 1;
				}
			}
			
			break;
			
		
		default:
			break;		
	}
}


bool dateTime_analizarIm (imMessage_t* msg)
{
	bool ret = false;
	
	if (msg->cmd == IM_CLIENT_CMD_GET) {
		if (msg->reg == IM_CLIENT_REG_FECHA) {
			dateTime_armarGetFecha();
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_HORA) {
			dateTime_armarGetHora();
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_FECHA_HORA) {
			dateTime_armarGetFechaHora();
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
	}
	else if (msg->cmd == IM_CLIENT_CMD_SET) {
		if (msg->reg == IM_CLIENT_REG_FECHA) {
			procesarSetFecha(msg->payload, msg->len);
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_HORA) {
			procesarSetHora(msg->payload, msg->len);
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_FECHA_HORA) {
			procesarSetfechaHora(msg->payload, msg->len);
			imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
			ret = true;
		}
	}
	else if (msg->cmd == IM_CLIENT_CMD_SINCRO_FYH) {
		apDateTime2.bits.vinoComandoSincronizar = 1;
		imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
		ret = true;
	}
	else if (msg->cmd == IM_CLIENT_CMD_RESP_PEDIR_FYH) {
		procesarRespPedirFechaHora(msg->payload, msg->len);
		imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
		ret = true;
	}
	
	return ret;
}


void dateTime_armarGetFecha (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	mpxhTimeDate_t mpxhTimeDate;
	dateTime_t dateTime;
		
	if (msg != NULL) {
		mpxhTimeDate_getTimeDate(&mpxhTimeDate);
		mpxhTimeDateToDateTime(&mpxhTimeDate, &dateTime);
		
		imClient_putPayloadByte(msg, dateTime.dia);
		imClient_putPayloadByte(msg, dateTime.mes);
		imClient_putPayloadByte(msg, dateTime.anio);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_FECHA, 0, 0);
	}
}


void dateTime_armarGetHora (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	mpxhTimeDate_t mpxhTimeDate;
	dateTime_t dateTime;
		
	if (msg != NULL) {
		mpxhTimeDate_getTimeDate(&mpxhTimeDate);
		mpxhTimeDateToDateTime(&mpxhTimeDate, &dateTime);
		
		imClient_putPayloadByte(msg, dateTime.horas);
		imClient_putPayloadByte(msg, dateTime.minutos);
		imClient_putPayloadByte(msg, dateTime.segundos);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_HORA, 0, 0);
	}
}


void dateTime_armarGetFechaHora (void)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	mpxhTimeDate_t mpxhTimeDate;
	dateTime_t dateTime;
		
	if (msg != NULL) {
		mpxhTimeDate_getTimeDate(&mpxhTimeDate);
		mpxhTimeDateToDateTime(&mpxhTimeDate, &dateTime);
		
		imClient_putPayloadByte(msg, dateTime.dia);
		imClient_putPayloadByte(msg, dateTime.mes);
		imClient_putPayloadByte(msg, dateTime.anio);
		imClient_putPayloadByte(msg, dateTime.horas);
		imClient_putPayloadByte(msg, dateTime.minutos);
		imClient_putPayloadByte(msg, dateTime.segundos);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_FECHA_HORA, 0, 0);
	}
}


void dateTime_armarEventoFecha (void)
{
	uint8_t toId[4];
	dateTime_t dateTime;
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	
	if (msg != NULL) {
		dateTime_getTimeDate(&dateTime);
		
		imClient_putPayloadByte(msg, dateTime.dia);
		imClient_putPayloadByte(msg, dateTime.mes);
		imClient_putPayloadByte(msg, dateTime.anio);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_FECHA, 0, 0);
	}
}


void dateTime_armarEventoHora (void)
{
	uint8_t toId[4];
	dateTime_t dateTime;
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		dateTime_getTimeDate(&dateTime);
		
		imClient_putPayloadByte(msg, dateTime.horas);
		imClient_putPayloadByte(msg, dateTime.minutos);
		imClient_putPayloadByte(msg, dateTime.segundos);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_HORA, 0, 0);
	}
}


void dateTime_armarEventoFechaHora (void)
{
	uint8_t toId[4];
	dateTime_t dateTime;
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		dateTime_getTimeDate(&dateTime);
		
		imClient_putPayloadByte(msg, dateTime.dia);
		imClient_putPayloadByte(msg, dateTime.mes);
		imClient_putPayloadByte(msg, dateTime.anio);
		imClient_putPayloadByte(msg, dateTime.horas);
		imClient_putPayloadByte(msg, dateTime.minutos);
		imClient_putPayloadByte(msg, dateTime.segundos);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_FECHA_HORA, 0, 0);
	}
}


void dateTime_determinarFinProcesamiento(void)
{
	if (procesandoMensajes.bits.procesandoSetFechayHora) {
		if (!mpxhTimeDate_hayQueSacarFyh()) {
			// Terminó de sacar la fecha y/u hora por MPXH
			procesandoMensajes.bits.procesandoSetFechayHora = 0;
		}
	}
}


bool dateTime_procesandoMensaje(void)
{
	return (procesandoMensajes.w != 0);
}


void dateTimeToMpxhTimeDate (dateTime_t* dt, mpxhTimeDate_t* mpxhTd) 
{
	mpxhTd->d[MPXH_DATE_DIA2] = dt->dia / 10;
	mpxhTd->d[MPXH_DATE_DIA1] = dt->dia % 10;
	mpxhTd->d[MPXH_DATE_MES2] = dt->mes / 10;
	mpxhTd->d[MPXH_DATE_MES1] = dt->mes % 10;
	mpxhTd->d[MPXH_DATE_ANIO4] = 2;
	mpxhTd->d[MPXH_DATE_ANIO3] = 0;
	mpxhTd->d[MPXH_DATE_ANIO2] = dt->anio / 10;
	mpxhTd->d[MPXH_DATE_ANIO1] = dt->anio % 10;
	mpxhTd->t[MPXH_TIME_HORA2] = dt->horas / 10;
	mpxhTd->t[MPXH_TIME_HORA1] = dt->horas % 10;
	mpxhTd->t[MPXH_TIME_MIN2] = dt->minutos / 10;
	mpxhTd->t[MPXH_TIME_MIN1] = dt->minutos % 10;
	mpxhTd->t[MPXH_TIME_SEG2] = dt->segundos / 10;
	mpxhTd->t[MPXH_TIME_SEG1] = dt->segundos % 10;
}


void mpxhTimeDateToDateTime (mpxhTimeDate_t* mpxhTd, dateTime_t* dt) 
{
	dt->dia = mpxhTd->d[MPXH_DATE_DIA2] * 10 + mpxhTd->d[MPXH_DATE_DIA1];
	dt->mes = mpxhTd->d[MPXH_DATE_MES2] * 10 + mpxhTd->d[MPXH_DATE_MES1];
	dt->anio = mpxhTd->d[MPXH_DATE_ANIO2] * 10 + mpxhTd->d[MPXH_DATE_ANIO1];

	dt->horas = mpxhTd->t[MPXH_TIME_HORA2] * 10 + mpxhTd->t[MPXH_TIME_HORA1];
	dt->minutos = mpxhTd->t[MPXH_TIME_MIN2] * 10 + mpxhTd->t[MPXH_TIME_MIN1];
	dt->segundos = mpxhTd->t[MPXH_TIME_SEG2] * 10 + mpxhTd->t[MPXH_TIME_SEG1];
}


void procesarSetFecha(uint8_t* payload, uint8_t payloadLen)
{
	dateTime_t dateTime;
	
	
	if (fsmPatronState == dateTime_patron_esperandoHoraValida ||
		fsmPatronState == dateTime_patron_esperandoSacarHora ||
		fsmPatronState == dateTime_patron_esperandoSacarFechaHora) {
		if(payloadLen < 3)
			return;
	
		if (esFechaValida(payload[0], payload[1], payload[2])) {
			dateTime.dia = payload[0];		
			dateTime.mes = payload[1];
			dateTime.anio = payload[2];
		
			dateTime_setDate(&dateTime);
			procesandoMensajes.bits.procesandoSetFechayHora = 1;
		}
	}
	
	
	// responde al cambio de fecha con un event (haya podido cambiar la fecha o no)
	dateTime_armarEventoFecha();
}


void procesarSetHora(uint8_t* payload, uint8_t payloadLen)
{
	dateTime_t dateTime;
	
	
	if (fsmPatronState == dateTime_patron_esperandoHoraValida ||
		fsmPatronState == dateTime_patron_esperandoSacarHora ||
		fsmPatronState == dateTime_patron_esperandoSacarFechaHora) {
		if(payloadLen < 3)
			return;
	
		if (esHoraValida(payload[0], payload[1], payload[2])) {
			dateTime.horas = payload[0];		
			dateTime.minutos = payload[1];
			dateTime.segundos = payload[2];
		
			dateTime_setTime(&dateTime);
			procesandoMensajes.bits.procesandoSetFechayHora = 1;
		}
	}
	
	
	// responde al cambio de hora con un event (haya podido cambiar la hora o no)
	dateTime_armarEventoHora();
}


void procesarSetfechaHora(uint8_t* payload, uint8_t payloadLen)
{
	dateTime_t dateTime;
	
	
	if (fsmPatronState == dateTime_patron_esperandoHoraValida ||
		fsmPatronState == dateTime_patron_esperandoSacarHora ||
		fsmPatronState == dateTime_patron_esperandoSacarFechaHora) {
		if(payloadLen < 6)
			return;
	
		if (esFechaValida(payload[0], payload[1], payload[2]) &&
			esHoraValida(payload[3], payload[4], payload[5])) {
			dateTime.dia = payload[0];		
			dateTime.mes = payload[1];
			dateTime.anio = payload[2];
			dateTime.horas = payload[3];		
			dateTime.minutos = payload[4];
			dateTime.segundos = payload[5];
		
			dateTime_setTimeDate(&dateTime);
			procesandoMensajes.bits.procesandoSetFechayHora = 1;
		}

	}
	
	
	// responde al cambio de fecha y hora con un event (haya podido cambiar la fecha y hora o no)
	dateTime_armarEventoFechaHora();
}


void procesarRespPedirFechaHora(uint8_t* payload, uint8_t payloadLen)
{
	if (payloadLen >= 6) {
		
		if (esFechaValida(payload[0], payload[1], payload[2]) &&
			esHoraValida(payload[3], payload[4], payload[5])) {
				
			dateTimeReceived.dia = payload[0];
			dateTimeReceived.mes = payload[1];
			dateTimeReceived.anio = payload[2];
			dateTimeReceived.horas = payload[3];
			dateTimeReceived.minutos = payload[4];
			dateTimeReceived.segundos = payload[5];
		
			variosDateTime.bits.llegoTiempoServidor = 1;
		}
	}
}


void armarPedirFechaHora (void) 
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		imClient_putPayloadByte(msg, pgaData[PGA_CODIGO_REGION]);
		
		toId[0] = 0;
		toId[1] = 0;
		toId[2] = 0;
		toId[3] = 0;
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_IM_CLIENT, IM_CLIENT_CMD_PEDIR_FYH, 0, 0, 0);
	}
}


bool esHoraValida (uint8_t hora, uint8_t minutos, uint8_t segundos)
{
	bool ret = false;
	
	if (hora >= 24)
		ret = false;
	else if (minutos >= 60)	
		ret = false;
	else if (segundos >= 60)
		ret = false;
	else
		ret = true;
	
	
	return ret;
}


bool esFechaValida (uint8_t dia, uint8_t mes, uint8_t anio)
{
	bool ret = false;
	
	if (dia >= 32)
		ret = false;
	else if (dia == 0)
		ret = false;
	else if (mes >= 13)	
		ret = false;
	else if (mes == 0)
		ret = false;
	else if (dia > mpxhTimeDate_getDiasMesConBisiesto(mes, anio))
		ret = false;
	else
		ret = true;
	
	
	return ret;
}