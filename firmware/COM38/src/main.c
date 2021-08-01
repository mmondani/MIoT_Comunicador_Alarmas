#include "asf.h"
#include "common/include/nm_common.h"
#include "config/conf_board.h"
#include "inc/project.h"
#include "inc/main.h"
#include "inc/debounce.h"
#include "inc/mainTimer.h"
#include "inc/softTimers.h"
#include "inc/mpxh.h"
#include "inc/pga.h"
#include "inc/BlinkingLed.h"
#include "inc/BlinkingPatterns.h"
#include "inc/displayRAM.h"

#define STRING_EOL    "\r\n"
#define STRING_HEADER "-- WINC1500 TCP client example --"STRING_EOL \
	"-- "BOARD_NAME " --"STRING_EOL	\
	"-- Compiled: "__DATE__ " "__TIME__ " --"STRING_EOL

/** UART module for debug. */
static struct usart_module probador_uart_module;
static struct usart_module console_uart_module;


uint8_t dataH, dataL, dataLayer;

uint8_t sacarLayer (uint8_t dato) ;



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
}



int main(void)
{

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
	

	while (1) {
		mainLoop();
	
	}

	return 0;
}

void mainLoop (void) {
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
		}
			
		/*****************************************************************************/
		//  RECEPCIÓN MPXH : 16 bits
		/*****************************************************************************/
		if ( mpxh_recibio ( MPXH_BITS_16 ) )
		{
			mpxh_clearRecFlag( MPXH_BITS_16 );
			mpxh_getRecibido( &dataH, &dataL, &dataLayer);
		}
			
		/*****************************************************************************/
		//  RECEPCIÓN MPXH : 15 bits
		/*****************************************************************************/
		if ( mpxh_recibio ( MPXH_BITS_15 ) )
		{
			mpxh_clearRecFlag( MPXH_BITS_15 );
			mpxh_getRecibido( &dataH, &dataL, &dataLayer);
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
		//  HANDLERS
		/*****************************************************************************/



		/*****************************************************************************/
		//  ACCIONES
		/*****************************************************************************/


			
		/*****************************************************************************/
		//  SACARBUF MPXH
		/*****************************************************************************/
		if (!mpxh_Ocupado()) {
				
			if (mpxh_tiempoIdle(24*MPXH_MSEG)) {
				if (displayRAM_hayChar()) {
					displayRAM_sacarChar();
				}
			}
			else if (mpxh_tiempoIdle(32*MPXH_MSEG)) {
				if (mpxh_tiempoIdle(60*MPXH_MSEG)) {
					if (pga_hayqueDumpear()) {
						pga_dumpByte();
					}
				}
			}
		}
	}
		
	if (mainTimer_expired(TIMER_1SEG)) {
		maintTimer_clearExpired(TIMER_1SEG);
		
		wdt_reset_count();
	}

	if (mainTimer_expired(TIMER_1MIN)) {
		maintTimer_clearExpired(TIMER_1MIN);
			
	}

	if (mainTimer_expired(TIMER_1HORA)) {
		maintTimer_clearExpired(TIMER_1HORA);
	}


	port_pin_set_output_level(DUTY_PAL, false);
}
