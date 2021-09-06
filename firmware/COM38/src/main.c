#include "asf.h"
#include "common/include/nm_common.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"
#include "config/conf_board.h"
#include "inc/project.h"
#include "inc/debounce.h"
#include "inc/mainTimer.h"
#include "inc/softTimers.h"
#include "inc/stopWatch.h"
#include "inc/socketManager.h"
#include "inc/WiFiManager.h"
#include "inc/imClient.h"
#include "inc/mpxh.h"
#include "inc/project.h"
#include "inc/imClient_cmds_regs.h"
#include "inc/MessagePoolManager.h"
#include "inc/BlinkingLed.h"
#include "inc/BlinkingPatterns.h"
#include "inc/mainFsm.h"
#include "inc/dateTime.h"
#include "inc/mpxhTimeDate.h"
#include "inc/pga.h"
#include "inc/alarmMonitor.h"
#include "inc/configurationManager.h"
#include "inc/utilities.h"
#include "inc/fsmProg.h"
#include "inc/displayRAM.h"
#include "inc/nodesManager.h"




#define STRING_EOL    "\r\n"
#define STRING_HEADER "-- WINC1500 TCP client example --"STRING_EOL \
	"-- "BOARD_NAME " --"STRING_EOL	\
	"-- Compiled: "__DATE__ " "__TIME__ " --"STRING_EOL

/** UART module for debug. */
static struct usart_module probador_uart_module;
static struct usart_module console_uart_module;


uint8_t dataH, dataL, dataLayer;

uint8_t sacarLayer (uint8_t dato) ;



/**
 * \brief Configure UART console.
 */
static void configure_console(void)
{
	struct usart_config usart_conf;

	usart_get_config_defaults(&usart_conf);
	

	usart_conf.mux_setting = EDBG_CONSOLE_SERCOM_MUX_SETTING;
	usart_conf.pinmux_pad0 = EDBG_CONSOLE_SERCOM_PINMUX_PAD0;
	usart_conf.pinmux_pad1 = EDBG_CONSOLE_SERCOM_PINMUX_PAD1;
	usart_conf.pinmux_pad2 = EDBG_CONSOLE_SERCOM_PINMUX_PAD2;
	usart_conf.pinmux_pad3 = EDBG_CONSOLE_SERCOM_PINMUX_PAD3;
	usart_conf.baudrate    = 115200;

	stdio_serial_init(&console_uart_module, EDBG_CONSOLE_MODULE, &usart_conf);
	usart_enable(&console_uart_module);



	usart_conf.mux_setting = PROBADOR_UART_SERCOM_MUX_SETTING;
	usart_conf.pinmux_pad0 = PROBADOR_UART_SERCOM_PINMUX_PAD0;
	usart_conf.pinmux_pad1 = PROBADOR_UART_SERCOM_PINMUX_PAD1;
	usart_conf.pinmux_pad2 = PROBADOR_UART_SERCOM_PINMUX_PAD2;
	usart_conf.pinmux_pad3 = PROBADOR_UART_SERCOM_PINMUX_PAD3;
	usart_conf.baudrate    = 115200;

	while (usart_init(&probador_uart_module, PROBADOR_UART_MODULE, &usart_conf) != STATUS_OK);
	usart_enable(&probador_uart_module);

}



static void init_leds_button (void) 
{
	struct port_config pin_conf;
	
	port_get_config_defaults(&pin_conf);

	pin_conf.direction  = PORT_PIN_DIR_INPUT;
	pin_conf.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(BUTTON_0_PIN, &pin_conf);
	
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	pin_conf.input_pull = SYSTEM_PINMUX_PIN_PULL_NONE;
	port_pin_set_config(LED_VERDE_PIN, &pin_conf);
	port_pin_set_output_level(LED_VERDE_PIN, false);
	port_pin_set_config(LED_ROJO_PIN, &pin_conf);
	port_pin_set_output_level(LED_ROJO_PIN, false);
	port_pin_set_config(DUTY_INT, &pin_conf);
	port_pin_set_output_level(DUTY_INT, false);
	port_pin_set_config(DUTY_PAL, &pin_conf);
	port_pin_set_output_level(DUTY_PAL, false);
}


static void configure_wdt (void)
{
	enableWDT();
}



int main(void)
{
	uint8_t layer;

	/* Initialize the board. */
	system_init();


	/* Initialize the UART console. */
	configure_console();
#ifdef DEBUG_PRINTF
	printf(STRING_HEADER);
#endif

	mainTimer_init();
	mpxh_init();
	init_leds_button();
	
	configure_wdt();

	mainFsm_init(&probador_uart_module);
	

	while (1) {
		mainFsm_handler();
		
		/* Handle pending events from network controller. */
		m2m_wifi_handle_events(NULL);
		
		if (mainFsm_hayErrorCritico())
			continue;
			
			
		imClient_handler();
		
		wifiManager_ProvisioningFsmHandler();
		
		dateTime_handler();
		dateTime_fsmPatronTiempo();
		

		if (mainTimer_expired(TIMER_4MSEG)) {
			maintTimer_clearExpired(TIMER_4MSEG);

			port_pin_set_output_level(DUTY_PAL, true);

			/*****************************************************************************/
			//  RECEPCIÓN MPXH : 17 bits
			/*****************************************************************************/
			if ( mpxh_recibio ( MPXH_BITS_17 ) )
			{
				mpxh_clearRecFlag( MPXH_BITS_17 );
				mpxh_getRecibido( &dataH, &dataL, &dataLayer);

	
				mainFsm_analizarMpxh(dataH, dataL, dataLayer, MPXH_BITS_17);
				alarmMonitor_analizarMpxh(dataH, dataL, dataLayer, MPXH_BITS_17);
				dateTime_analizarMpxh(dataH, dataL, dataLayer, MPXH_BITS_17);
				fsmProg_analizarMpxh(dataH, dataL, dataLayer, MPXH_BITS_17);
				nodesManager_analizarMpxh(dataH, dataL, dataLayer, MPXH_BITS_17);
			}
			
			/*****************************************************************************/
			//  RECEPCIÓN MPXH : 16 bits
			/*****************************************************************************/
			if ( mpxh_recibio ( MPXH_BITS_16 ) )
			{
				mpxh_clearRecFlag( MPXH_BITS_16 );
				mpxh_getRecibido( &dataH, &dataL, &dataLayer);

				alarmMonitor_analizarMpxh(dataH, dataL, dataLayer, MPXH_BITS_16);
				dateTime_analizarMpxh(dataH, dataL, dataLayer, MPXH_BITS_16);
				nodesManager_analizarMpxh(dataH, dataL, dataLayer, MPXH_BITS_16);
			}
			
			/*****************************************************************************/
			//  RECEPCIÓN MPXH : 15 bits
			/*****************************************************************************/
			if ( mpxh_recibio ( MPXH_BITS_15 ) )
			{
				mpxh_clearRecFlag( MPXH_BITS_15 );
				mpxh_getRecibido( &dataH, &dataL, &dataLayer);

				alarmMonitor_analizarMpxh(dataH, dataL, dataLayer, MPXH_BITS_15);
			}
			
			/*****************************************************************************/
			//  RECEPCIÓN MPXH : 12 bits
			/*****************************************************************************/
			if ( mpxh_recibio ( MPXH_BITS_12 ) )
			{
				mpxh_clearRecFlag( MPXH_BITS_12 );
				mpxh_getRecibido( &dataH, &dataL, &dataLayer);
			}
			
			/*****************************************************************************/
			//  RECEPCIÓN MPXH : 9 bits
			/*****************************************************************************/
			if ( mpxh_recibio ( MPXH_BITS_9 ) )
			{
				mpxh_clearRecFlag( MPXH_BITS_9 );
				mpxh_getRecibido( &dataH, &dataL, &dataLayer);

				displayRAM_detenerDisplay();
				
			}
			
			/*****************************************************************************/
			//  RECEPCIÓN MPXH : 4 bits
			/*****************************************************************************/
			if ( mpxh_recibio ( MPXH_BITS_4 ) )
			{
				mpxh_clearRecFlag( MPXH_BITS_4 );
				mpxh_getRecibido( &dataH, &dataL, &dataLayer);

			}
			
			
			
			/*****************************************************************************/
			//  RECEPCIÓN IM
			/*****************************************************************************/
			if (!alarmMonitor_procesandoMensaje() &&
			!dateTime_procesandoMensaje() &&
			!nodesManager_procesandoMensaje()) {
				imMessage_t* msg = imClient_getMessageToRead(MESSAGE_POOL_FLOW_WCOM);
				if (msg != NULL && (msg->flow == MESSAGE_POOL_FLOW_WCOM || msg->flow == MESSAGE_POOL_FLOW_WCOM_STRINGS)) {
					if (alarmMonitor_analizarIm (msg)) {
						
					}
					else if (dateTime_analizarIm(msg)) {

					}
					else if (configurationManager_analizarIm(msg)) {

					}
					else if (nodesManager_analizarIm(msg)) {
						
					}
					else if (fsmProg_analizarIm(msg)) {
						
					}
					else
					imClient_removeMessageToRead(MESSAGE_POOL_FLOW_WCOM);
				}
			}
			
			
			

			/*****************************************************************************/
			//  HANDLERS
			/*****************************************************************************/
			dateTime_timeKeepingHandler();
			nodesManager_handler();
			
			fsmProg_handler();
			
			alarmMonitor_determinarFinProcesamiento();
			dateTime_determinarFinProcesamiento();
			nodesManager_determinarFinProcesamiento();


			/*****************************************************************************/
			//  ACCIONES
			/*****************************************************************************/


			
			/*****************************************************************************/
			//  SACARBUF MPXH
			/*****************************************************************************/
			if (!mpxh_Ocupado()) {
				if (comandos1.bits.pedir_fyh) {
					comandos1.bits.pedir_fyh = 0;
					mpxh_ArmaMensaje(0x0a, 0x80, 0, MPXH_BITS_17);
				}
				else if (comandos1.bits.mandar_0a6) {
					comandos1.bits.mandar_0a6 = 0;
					mpxh_ArmaMensaje(0x0a, 0x60, 0, MPXH_BITS_17);
				}
				else if (comandos1.bits.mandar_0a7) {
					comandos1.bits.mandar_0a7 = 0;
					mpxh_ArmaMensaje(0x0a, 0x70, 0, MPXH_BITS_17);
				}
				else if (pedirStatusCentral != 0) {
					layer = sacarLayer(pedirStatusCentral);
					bit_clear(pedirStatusCentral, layer);
					
					if (layer == 0) 
						mpxh_ArmaMensaje(0x01, 0xd0, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x01, 0xd0, layer, MPXH_BITS_16);
				}
				else if (pedirReplay != 0) {
					layer = sacarLayer(pedirReplay);
					bit_clear(pedirReplay, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x01, 0x50, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x01, 0x50, layer, MPXH_BITS_16);
				}
				else if (mandarBorrarMemoria != 0) {
					layer = sacarLayer(mandarBorrarMemoria);
					bit_clear(mandarBorrarMemoria, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x01, 0x40, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x01, 0x40, layer, MPXH_BITS_16);
				}
				else if (pedirEventos != 0) {
					layer = sacarLayer(pedirEventos);
					bit_clear(pedirEventos, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x08, 0x00, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x08, 0x00, layer, MPXH_BITS_16);
				}
				else if (mandarTeclas != 0) {
					if (ptrTeclas < lenTeclas) {
						layer = sacarLayer(mandarTeclas);
					
						if (layer == 0)
							mpxh_ArmaMensaje(0x00, nibble_swap(bufferTeclas[ptrTeclas]), 0, MPXH_BITS_17);
						else
							mpxh_ArmaMensaje(0x00, nibble_swap(bufferTeclas[ptrTeclas]), layer, MPXH_BITS_16);
						
						ptrTeclas ++;
						if (ptrTeclas >= lenTeclas)
							bit_clear(mandarTeclas,layer);
					}
				}
				else if (comandos1.bits.mandar_097) {
					comandos1.bits.mandar_097 = 0;
					mpxh_ArmaMensaje(0x09, 0x70, 0, MPXH_BITS_17);
				}
				else if (mpxhTimeDate_hayQueSacarFyh()) {
					mpxhTimeDate_sendNextNibble();
				}
				else if (comandos1.bits.mandar_623) {
					comandos1.bits.mandar_623 = 0;
					mpxh_ArmaMensaje(0x62, 0x30, 0, MPXH_BITS_17);
				}
				else if (comandos1.bits.mandar_624) {
					comandos1.bits.mandar_624 = 0;
					mpxh_ArmaMensaje(0x62, 0x40, 0, MPXH_BITS_17);
				}
				else if (comandos2.bits.mandar_625) {
					comandos2.bits.mandar_625 = 0;
					mpxh_ArmaMensaje(0x62, 0x50, 0, MPXH_BITS_17);
				}
				else if (comandos1.bits.mandar_626) {
					comandos1.bits.mandar_626 = 0;
					mpxh_ArmaMensaje(0x62, 0x60, 0, MPXH_BITS_17);
				}
				else if (comandos2.bits.mandar_0a8) {
					comandos2.bits.mandar_0a8 = 0;
					mpxh_ArmaMensaje(0x0a, 0x80, 0, MPXH_BITS_17);
				}
				else if (mandarPanico != 0) {
					layer = sacarLayer(mandarPanico);
					bit_clear(mandarPanico, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x01, 0x20, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x01, 0x20, layer, MPXH_BITS_16);
				}
				else if (mandarIncendioManual != 0) {
					layer = sacarLayer(mandarIncendioManual);
					bit_clear(mandarIncendioManual, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x01, 0x10, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x01, 0x10, layer, MPXH_BITS_16);
				}
				else if (mandarEmergenciaMedica != 0) {
					layer = sacarLayer(mandarEmergenciaMedica);
					bit_clear(mandarEmergenciaMedica, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x01, 0x00, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x01, 0x00, layer, MPXH_BITS_16);
				}
				else if (mandarEnProgramacion != 0) {
					layer = sacarLayer(mandarEnProgramacion);
					bit_clear(mandarEnProgramacion, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x0a, 0xb0, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x0a, 0xb0, layer, MPXH_BITS_16);
				}
				else if (mandarAgudoIncondicional != 0) {
					layer = sacarLayer(mandarAgudoIncondicional);
					bit_clear(mandarAgudoIncondicional, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x6f, 0x70, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x6f, 0x70, layer, MPXH_BITS_16);
				}
				else if (mandarPiru != 0) {
					layer = sacarLayer(mandarPiru);
					bit_clear(mandarPiru, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x6f, 0x60, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x6f, 0x60, layer, MPXH_BITS_16);
				}
				else if (mandarPiruPiru != 0) {
					layer = sacarLayer(mandarPiruPiru);
					bit_clear(mandarPiruPiru, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x6c, 0x40, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x6c, 0x40, layer, MPXH_BITS_16);
				}
				else if (mandarGracias != 0) {
					layer = sacarLayer(mandarGracias);
					bit_clear(mandarGracias, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x6d, 0x20, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x6d, 0x20, layer, MPXH_BITS_16);
				}
				else if (mandarGrave != 0) {
					layer = sacarLayer(mandarGrave);
					bit_clear(mandarGrave, layer);
					
					if (layer == 0)
					mpxh_ArmaMensaje(0x6f, 0xb0, 0, MPXH_BITS_17);
					else
					mpxh_ArmaMensaje(0x6f, 0xb0, layer, MPXH_BITS_16);
				}
				else if (mandarError != 0) {
					layer = sacarLayer(mandarError);
					bit_clear(mandarError, layer);
					
					if (layer == 0)
					mpxh_ArmaMensaje(0x6d, 0xc0, 0, MPXH_BITS_17);
					else
					mpxh_ArmaMensaje(0x6d, 0xc0, layer, MPXH_BITS_16);
				}
				else if (mandarAvanzada != 0) {
					layer = sacarLayer(mandarAvanzada);
					bit_clear(mandarAvanzada, layer);
					
					if (layer == 0)
					mpxh_ArmaMensaje(0x6b, 0x10, 0, MPXH_BITS_17);
					else
					mpxh_ArmaMensaje(0x6b, 0x10, layer, MPXH_BITS_16);
				}
				else if (mandarProgramacion != 0) {
					layer = sacarLayer(mandarProgramacion);
					bit_clear(mandarProgramacion, layer);
					
					if (layer == 0)
					mpxh_ArmaMensaje(0x6b, 0x00, 0, MPXH_BITS_17);
					else
					mpxh_ArmaMensaje(0x6b, 0x00, layer, MPXH_BITS_16);
				}
				else if (mandarApagarTodo != 0) {
					layer = sacarLayer(mandarApagarTodo);
					bit_clear(mandarApagarTodo, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(0x0a, 0xd0, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(0x0a, 0xd0, layer, MPXH_BITS_16);
				}
				else if (mandarMensajeMpxh != 0) {
					layer = sacarLayer(mandarMensajeMpxh);
					bit_clear(mandarMensajeMpxh, layer);
					
					if (layer == 0)
						mpxh_ArmaMensaje(mensajeMpxh_dataH, mensajeMpxh_dataL, 0, MPXH_BITS_17);
					else
						mpxh_ArmaMensaje(mensajeMpxh_dataH, mensajeMpxh_dataL, layer, MPXH_BITS_16);
				}
				else if (mpxh_tiempoIdle(24*MPXH_MSEG)) {
					pga_saveRAM();
					
					if (mandarBufferTlcd != 0) {
						layer = sacarLayer(mandarBufferTlcd);
						
						if (bufferTlcd_puntero < 34 && bufferTlcd[bufferTlcd_puntero] != 0xff) {
							if (layer == 0)
								mpxh_ArmaMensaje(bufferTlcd[bufferTlcd_puntero], 0, 0, MPXH_BITS_9);
							else
								mpxh_ArmaMensaje(bufferTlcd[bufferTlcd_puntero], 0, layer, MPXH_BITS_12);
							
							bufferTlcd_puntero ++;
						}
						else {
							bit_clear(mandarBufferTlcd, layer);
						}
					}
					else if (displayRAM_hayChar()) {
						displayRAM_sacarChar();
					}
					else if (mpxh_tiempoIdle(32*MPXH_MSEG)) {
						
						if (nodesManager_hayQueMandarPorMpxh()) {
							nodesManager_mandarPorMpxh();
						}
						if (mpxh_tiempoIdle(60*MPXH_MSEG)) {
							if (pga_hayqueDumpear()) {
								pga_dumpByte();
							}
						}
					}
				}
			}
			
		}
		
		if (mainTimer_expired(TIMER_1SEG)) {
			maintTimer_clearExpired(TIMER_1SEG);
			
			alarmMonitor_timers1s_handler();
			dateTime_timers1s_handler();
			fsmProg_timers1s_handler();
			nodesManager_timers1s_handler();
		}

		if (mainTimer_expired(TIMER_1MIN)) {
			maintTimer_clearExpired(TIMER_1MIN);
			
			alarmMonitor_timers1m_handler();
			nodesManager_timers1m_handler();
		}

		if (mainTimer_expired(TIMER_1HORA)) {
			maintTimer_clearExpired(TIMER_1HORA);
			
			alarmMonitor_timers1h_handler();
			dateTime_timers1h_handler();
		}

		
		wdt_reset_count();
		
		port_pin_set_output_level(DUTY_PAL, false);
	
	}

	return 0;
}


