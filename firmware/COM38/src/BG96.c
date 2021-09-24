#include <string.h>
#include "../inc/BG96.h"
#include "../inc/ringBuffer.h"
#include "../inc/uartRB.h"
#include "../inc/softTimers.h"
#include "../inc/basicDefinitions.h"
#include "../inc/BG96_commands.h"
#include "../inc/BG96_apn.h"

static struct usart_module* uart_module;
static bool catMEnabled;
static bool allBands4g;
static uint8_t simPin[4];

static uartRB_t uartRB;
static uint8_t rx_buffer[300];
static uint8_t tx_buffer[300];
static ringBuffer_t rbTx;
static ringBuffer_t rbRx;

static uint8_t serialBuffer[10];
static uint8_t serialCounter = 0;

static uint8_t imei[16];
static uint8_t publicIp[16];
static uint8_t signal = 0;

// Variables usadas para integrar la señal de status del BG96
static uint8_t bg96StatusSignal = 0;
static bool bg96Status = false;


typedef enum {
	BG96_PHONE_CARRIER_NONE = 0,
	BG96_PHONE_CARRIER_MOVISTAR,
	BG96_PHONE_CARRIER_CLARO,
	BG96_PHONE_CARRIER_PERSONAL
}bg96_phone_carrier_t;

static bg96_phone_carrier_t phoneCarrier = BG96_PHONE_CARRIER_NONE;

typedef enum {
	BG96_NETWORK_NONE = 0,
	BG96_NETWORK_GSM,
	BG96_NETWORK_EDGE,
	BG96_NETWORK_CATM1
}bg96_network_t;

static bg96_network_t network = BG96_NETWORK_NONE;


/**************************************************************/
// PDP Context
/**************************************************************/
typedef struct {
	bool free;
	uint8_t contextId;		// Va de 1 a 16
	uint8_t customApnServer[30];
	uint8_t customApnServerUser[20];
	uint8_t customApnServerPassword[20];
	uint8_t publicIp[16];
	bool opened;
	bool open;
	bool close;
	bool alreadyTryToOpen;
	bool alreadyTryToClose;
	uint8_t triesToOpen;
	uint8_t triesToClose;
}bg96PdpContext_t;

static bg96PdpContext_t pdpContexts[BG96_PDP_CONTEXTS];

// Determinar si hay que abrir o cerrar un PDP context
void checkPdpContextToOpenOrClose(void);
bg96PdpContext_t* pdpContextIdOpenClose;

uint8_t getPdpContextArrayIndex (uint8_t contextId);
/**************************************************************/

/**************************************************************/
// SSL Context
/**************************************************************/
typedef struct {
	bool free;
	uint8_t contextId;		// Va de 0 a 5
	uint8_t caCertName[20];
	bool opened;
	bool open;
	bool alreadyTryToOpen;
	uint8_t triesToOpen;
}bg96SslContext_t;

static bg96SslContext_t sslContexts[BG96_PDP_CONTEXTS];

// Determinar si hay que abrir un SSL context
void checkSslContextToOpen(void);
bg96SslContext_t* sslContextIdOpen;

uint8_t getSslContextArrayIndex (uint8_t contextId);
/**************************************************************/


/**************************************************************/
// Socket TCP
/**************************************************************/
typedef struct {
	bool free;
	uint32_t pdpContext;
	uint32_t sslContext;
	uint32_t connectionId;
	uint8_t remoteAddress[50];		// Tiene que estar temrinado con \0
	uint32_t remotePort;
	bool isSecure;
	bool connected;
	uint8_t dataReceived[100];
	uint8_t dataToSend[100];
	uint32_t dataReceivedLen;
	uint32_t dataToSendLen;
	bool connect;
	bool disconnect;
	bool sendData;
	bool readData;
	uint8_t triesToConnect;
	uint8_t triesToDisconnect;
	uint8_t triesToSendData;
	uint8_t triesToReceiveData;
	bool alreadyTriedToConnect;
	bool alreadyTriedToDisconnect;
	bool alreadyTriedToSendData;
	bool alreadyTriedToReceiveData;
}bg96TcpSocket_t;

static bg96TcpSocket_t tcpSockets[BG96_TCP_SOCKETS];

// Función a la que se llama cuando se producen eventos relacionados con los sockets
static void (*socketCallback)(uint32_t connectionId, bg96_socket_events evt, void* payload) = NULL;
static bg96_socketConnectedPayload socketConnectedPayload;
static bg96_socketDataReceivedPayload socketDataReceivedPayload;
static bg96_socketErrorPayload socketErrorPayload;

// Determinar si hay que abrir o cerrar un socket
void checkSocketToOpenOrClose(void);
bg96TcpSocket_t* connectionIdOpenClose;			// ConnectionID que se va a abrir o cerrar

// Determinar si hay que enviar data
void checkSocketToSendData(void);
bg96TcpSocket_t* connectionIdSendData;			// ConnectionID en el que se va a mandar data

// Determinar si hay que recibir data
void checkSocketToReceiveData (void);
bg96TcpSocket_t* connectionIdReceiveData;			// ConnectionID en el que se va a recibir data

uint8_t getSocketArrayIndex(uint8_t socketId);
/**************************************************************/


/**************************************************************/
// MQTT
/**************************************************************/
typedef struct {
	bg96_mqtt_configuration_t config;
	
	bool opened;
	bool connected;
	bool configured;
	
	bool connect;
	bool disconnect;
	bool subscribe;
	bool unsibscribe;
	bool publish;
	
	uint8_t topicToSend[20];
	uint8_t msgToSend[100];
	uint8_t msgToSendLen;
	uint8_t qos;
	bool retain;
	
	uint8_t topicReceived[20];
	uint8_t msgReceived[100];
	uint8_t msgReceivedLen;
	
	uint8_t topicToSubscribe[20];
	uint8_t qosToSubscribe;
}bg96Mqtt_t;

bg96Mqtt_t mqttClient;

// Función a la que se llama cuando se producen eventos relacionados con el cliente MQTT
static void (*mqttCallback)(bg96_mqtt_events evt, void* payload) = NULL;
static bg96_mqttErrorPayload mqttErrorPayload;
static bg96_mqttMessageReceivedPayload mqttMessagePayload;
/**************************************************************/

static union Flags{
	uint8_t byte;
	struct {
		uint8_t moduleOn:1;
		uint8_t imeiReceived:1;
		uint8_t simAvailable:1;
		uint8_t simWithPin:1;
		uint8_t registred:1;
		uint8_t roaming:1;
		uint8_t callReady:1;
		uint8_t bit7:1;
	} bits;
} flags;

static union Flags2{
	uint8_t byte;
	struct {
		uint8_t initModule:1;
		uint8_t disconnectInternet:1;
		uint8_t testModule:1;
		uint8_t openSocket:1;
		uint8_t closeSocket:1;
		uint8_t sendData:1;
		uint8_t receiveData:1;
		uint8_t connectInternet:1;
	} bits;
} flags2;

static union Flags3{
	uint8_t byte;
	struct {
		uint8_t openSslContext:1;
		uint8_t changeCellularNetworks:1;
		uint8_t changeBands:1;
		uint8_t mqttConfigure:1;
		uint8_t mqttConnect:1;
		uint8_t mqttDisconnect:1;
		uint8_t mqttSubscribe:1;
		uint8_t mqttPublish:1;
	} bits;
} flags3;

static union SerialFlags1{
	uint8_t byte;
	struct {
		uint8_t okReceived:1;
		uint8_t promptReceived:1;
		uint8_t errorReceived:1;
		uint8_t tcpSendOk:1;
		uint8_t sentCREG:1;
		uint8_t ipReceived:1;
		uint8_t closedReceived:1;
		uint8_t dataReceiveCompleted:1;
	} bits;
} serialFlags1;

static union SerialFlags2{
	uint8_t byte;
	struct {
		uint8_t keepAliveReceived:1;
		uint8_t plusReceived:1;
		uint8_t mqttPublishOk:1;
		uint8_t mqttSubscribeOk:1;
		uint8_t bit4:1;
		uint8_t bit5:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
	} bits;
} serialFlags2;


/**************************************************************/
// FSM el manejo del BG96
/**************************************************************/
typedef enum    {
	module_idle = 0,
	module_init,
	module_register,
	module_changeNetworks,
	module_changeBands,
	module_connectToInternet,
	module_openSslContext,
	module_testModule,
	module_openSocket,
	module_closeSocket,
	module_sendData,
	module_receiveData,
	module_mqttConfigure,
	module_mqttConnect,
	module_mqttDisconnect,
	module_mqttPublish,
	module_mqttSubscribe
}module_state_t;

typedef enum    {
	module_null = 0,
	module_init_turnoffModulePower,
	module_init_waitPowerOff,
	module_init_turnOnOffModule,
	module_init_powerKeyPulse,
	module_init_waitingPowerOn,
	module_init_sendCpin,
	module_init_waitingCpinOk,
	module_init_sendAte,
	module_init_waitingAteOk,
	module_init_sendQinistat,
	module_init_waitingQinistatOk,
	module_init_sendGsn,
	module_init_waitingGsnOk,
	module_init_sendQccid,
	module_init_waitingQccidOk,
	module_init_sendQcslk,
	module_init_waitingQsclkOk,
	module_init_sendCscs,
	module_init_waitingCscsOk,
	module_init_sendQurccfg,
	module_init_waitingQurccfgOk,
	module_init_sendNwsscanmode,
	module_init_waitingNwsscanmodeOk,
	module_init_sendIfc,
	module_init_waitingIfcOk,
	module_init_sendCfun,
	module_init_waitingCfunOk,
	module_init_sendBand,
	module_init_waitingBandOk,
	module_init_sendNwscanseq,
	module_init_waitingNwscanseqOk,
	module_init_sendIotopmode,
	module_init_waitingIotopmodeOk,
	module_init_sendCfun0,
	module_init_waitingCfun0Ok,
	module_init_sendCfun1,
	module_init_waitingCfun1Ok,
	module_init_sendCpin2,
	module_init_waitingCpinOk2,
	module_init_sendCpin1_2,
	module_init_waitingCpin1_2Ok,
	module_init_sendCpin3,
	module_init_waitingCpinOk3,
	module_init_sendSave,
	module_init_waitingSaveOk,
	module_init_sendCpin1,
	module_init_waitingCpin1Ok,
	module_init_sendCpinAgain,
	module_init_waitingCpinAgainOk,
	module_init_error,
	module_register_init,
	module_register_waitInitialDelay,
	module_register_sendCreg0,
	module_register_waitingCregOk,
	module_register_waitInitialDelay2,
	module_register_sendAtCreg,
	module_register_waitingAtCregOk,
	module_register_sendCreg2,
	module_register_waitingCreg2Ok,
	module_register_sendQnwinfo,
	module_register_waitingQnwinfoOk,
	module_register_turnOffModule,
	module_register_checkStatusSignal,
	module_internet_init,
	module_internet_waitInitialDelay,
	module_internet_sendQicsgp,
	module_internet_waitingQicsgpOk,
	module_internet_sendQiact,
	module_internet_waitingQiactOk,
	module_internet_sendAtQiact,
	module_internet_waitingAtQiactOk,
	module_internet_sendCipshut,
	module_internet_waitingCipshutOk,
	module_test_init,
	module_test_waitInitialDelay,
	module_test_sendCfun,
	module_test_waitingCfunOk,
	module_test_sendAtCreg,
	module_test_waitingAtCregOk,
	module_open_init,
	module_open_waitInitialDelay,
	module_open_sendQiopen,
	module_open_waitingQiopenOk,
	module_open_waitingSocketToBeOpen,
	module_close_init,
	module_close_waitInitialDelay,
	module_close_sendQiclose,
	module_open_waitingQicloseOk,
	module_send_init,
	module_send_waitInitialDelay,
	module_send_sendQisend,
	module_send_waitingPrompt,
	module_send_waitingSendOk,
	module_receive_init,
	module_receive_waitInitialDelay,
	module_receive_sendQird,
	module_receive_waitingReceiveOk,
	module_openSslContext_init,
	module_openSslContext_waitInitialDelay,
	module_openSslContext_sendQsslcfg,
	module_openSslContext_waitingQsslcfgOk,
	module_changeNetworks_sendNewscanmode,
	module_changeNetworks_waitingSendNewscanmodeOk,
	module_changeBands_sendQcfgBand,
	module_changeBands_waitingQcfgBandOk,
	module_mqttConfigure_init,
	module_mqttConfigure_waitInitialDelay,
	module_mqttConfigure_sendCfgPdp,
	module_mqttConfigure_waitingCfgPdpOk,
	module_mqttConfigure_sendCfgSsl,
	module_mqttConfigure_waitingCfgSslOk,
	module_mqttConfigure_sendCfgKeepalive,
	module_mqttConfigure_waitingCfgKeepaliveOk,
	module_mqttConfigure_sendCfgSession,
	module_mqttConfigure_waitingCfgSessionOk,
	module_mqttConfigure_sendCfgWill,
	module_mqttConfigure_waitingCfgWillOk,
	module_mqttConnect_init,
	module_mqttConnect_waitInitialDelay,
	module_mqttConnect_sendOpen,
	module_mqttConnect_waitingOpenOk,
	module_mqttConnect_waitingOpen,
	module_mqttConnect_sendConn,
	module_mqttConnect_waitingConnOk,
	module_mqttConnect_waitingConn,
	module_mqttConnect_sendClose,
	module_mqttConnect_waitingCloseOk,
	module_mqttConnect_waitingClose,
	module_mqttDisconnect_init,
	module_mqttDisconnect_waitInitialDelay,
	module_mqttDisconnect_sendDisc,
	module_mqttDisconnect_waitingDiscOk,
	module_mqttDisconnect_waitingDisc,
	module_mqttDisconnect_sendClose,
	module_mqttDisconnect_waitingCloseOk,
	module_mqttDisconnect_waitingClose,
	module_mqttPublish_init,
	module_mqttPublish_waitInitialDelay,
	module_send_sendQmtpub,
	module_mqttPublish_waitingPrompt,
	module_mqttPublish_waitingPublishOk,
	module_mqttSubscribe_init,
	module_mqttSubscribe_waitingInitialDelay,
	module_mqttSubscribe_sendQmtsub,
	module_mqttSubscribe_waitingQmtsubOk,
	module_mqttSubscribe_waitingQmtsub
}module_substate_t;

static module_state_t module_state;
static module_state_t module_previousState;

static module_substate_t module_substate;
static module_substate_t module_previousSubstate;

static void module_gotoState (module_state_t nextState, module_substate_t nextSubstate);
static void module_gotoSubstate (module_substate_t nextSubstate);
static void module_gotoPrevSubState (void);
static void module_gotoSubstateWithTimeOut (module_substate_t nextSubstate, uint32_t timeout);
static bool module_waitingOk (void);
static bool module_waitingPrompt (void);

SoftTimer_t timerModuleFsm;
SoftTimer_t timerModuleFsmTimeout;
SoftTimer_t softtimertimerRetryConnectInternet;
uint8_t module_aux1;
uint8_t module_auxBuffer1[10];

SoftTimer_t timerTestModule;

// Función a la que se llama cuando se producen eventos relacionados con el módulo celular
static void (*moduleCallback)(bg96_module_events evt, void* payload) = NULL;
static bg96_moduleStateChangePayload moduleStateChangePayload;
static bg96_contextStateChangePayload contextStateChangePayload;
static bg96_moduleErrorPayload moduleErrorPayload;
/**************************************************************/


/**************************************************************/
// FSM para la lectura de la UART del BG96
/**************************************************************/
typedef enum    {
	serialPort_idle = 0,
	serialPort_receiving_imei,
	serialPort_csq,
	serialPort_cops,
	serialPort_cpin,
	serialPort_creg,
	serialPort_cfun,
	serialPort_qnwinfo,
	serialPort_qinistat,
	serialPort_qiact,
	serialPort_qiopen,
	serialPort_qiurc,
	serialPort_qird,
	serialPort_qsslopen,
	serialPort_qsslurc,
	serialPort_qsslrecv,
	serialPort_mqttOpen,
	serialPort_mqttClose,
	serialPort_mqttConn,
	serialPort_mqttDisc,
	serialPort_mqttPub,
	serialPort_mqttSub,
	serialPort_mqttRec
}serialPort_state_t;

typedef enum    {
	serialPort_null = 0,
	serialPort_receiving_imei_waiting_eol,
	serialPort_receiving_imei_receiving,
	serialPort_qnwinfo_waitingNetwork,
	serialPort_qnwinfo_waitingPhoneCarrier,
	serialPort_qnwinfo_waitingQuote,
	serialPort_qnwinfo_waiting2Quotes,
	serialPort_qiact_waitingQuote,
	serialPort_qiact_receivingIp,
	serialPort_qiurc_waitingQuote,
	serialPort_qiurc_receivingEvent,
	serialPort_qiurc_receivingConnectionId,
	serialPort_qird_receivingReadLength,
	serialPort_qird_waitingCrLf,
	serialPort_qird_receivingData,
	serialPort_qsslurc_waitingQuote,
	serialPort_qsslurc_receivingEvent,
	serialPort_qsslurc_receivingConnectionId,
	serialPort_qsslrecv_receivingReadLength,
	serialPort_qsslrecv_waitingCrLf,
	serialPort_qsslrecv_receivingData,
	serialPort_mqttRec_waitingQuote,
	serialPort_mqttRec_receivingTopic,
	serialPort_mqttRec_waitingQuote2,
	serialPort_mqttRec_receivingMsg
}serialPort_substate_t;

static serialPort_state_t serialPort_state;
static serialPort_state_t serialPort_previousState;

static serialPort_substate_t serialPort_substate;
static serialPort_substate_t serialPort_previousSubstate;

static void serialPort_gotoState (serialPort_state_t nextState, serialPort_substate_t nextSubstate);
static void serialPort_gotoSubstate (serialPort_substate_t nextSubstate);
/**************************************************************/

/**************************************************************/
// Gestión de errores del BG96
/**************************************************************/
static void handleError (void);
static uint8_t numberOfModuleErrors = 0;
/**************************************************************/



static void usart_read_callback(struct usart_module *const usart_module);
static void usart_write_callback(struct usart_module *const usart_module);

static void readSerialPort (void);
static void closeAllSockets (void);
static void resetModuleData (void);


void bg96_init (struct usart_module* uart, bool catM, bool bands4g, uint8_t* pin) {
	struct port_config pin_conf;
	
	uart_module = uart;
	catMEnabled = catM;
	allBands4g = bands4g;
	
	if (pin != NULL) {
		for (uint32_t i = 0; i < 4 && pin[i] != '\0'; i++)
			simPin[i] = pin[i];
	}
	else
		simPin[0] = NULL;
	
	// Se configuran los pines que se usan con el BG96
	pin_conf.direction  = PORT_PIN_DIR_INPUT;
	pin_conf.input_pull = SYSTEM_PINMUX_PIN_PULL_NONE;
	port_pin_set_config(BG96_STATUS, &pin_conf);
	
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	pin_conf.input_pull = SYSTEM_PINMUX_PIN_PULL_NONE;
	port_pin_set_config(BG96_POWER_KEY, &pin_conf);
	port_pin_set_output_level(BG96_POWER_KEY, false);
	nop();
	nop();
	nop();
	nop();
	port_pin_set_config(BG96_POWER, &pin_conf);
	port_pin_set_output_level(BG96_POWER, false);
	nop();
	nop();
	nop();
	nop();
	port_pin_set_config(BG96_RESET, &pin_conf);
	port_pin_set_output_level(BG96_RESET, false);
	nop();
	nop();
	nop();
	nop();
	port_pin_set_config(BG96_DTR, &pin_conf);
	port_pin_set_output_level(BG96_DTR, false);
	nop();
	nop();
	nop();
	nop();
	port_pin_set_config(BG96_RTS, &pin_conf);
	port_pin_set_output_level(BG96_RTS, false);
	
	
	// Se configuran los ringbuffers de la UART que comunica con el BG96
	ringBuffer_init(&rbTx, tx_buffer, 300);
	ringBuffer_init(&rbRx, rx_buffer, 300);
	uartRB_init(&uartRB, uart_module, &rbTx, &rbRx, usart_write_callback, usart_read_callback);
	
	
	// Se configura el timer que va a disparar un test del módulo BG96 (cada 10 minutos)
	softTimer_init(&timerTestModule, 600000);
	
	
	// Inicialización de los contextos
	for (uint32_t i = 0; i < BG96_PDP_CONTEXTS; i++)
		pdpContexts[i].free = true;
		
	for (uint32_t i = 0; i < BG96_SSL_CONTEXTS; i++)
		sslContexts[i].free = true;
	
	// Inicialización de los sockets
	for (uint32_t i = 0; i < BG96_TCP_SOCKETS; i++)
		tcpSockets[i].free = true;
	
	
	// Inicialización de las FSM
	module_state = module_idle;
	module_previousState = module_idle;

	module_substate = module_null;
	module_previousSubstate = module_null;
	
	serialPort_state = serialPort_idle;
	serialPort_previousState = serialPort_idle;

	serialPort_substate = serialPort_null;
	serialPort_previousSubstate = serialPort_null;
}


void bg96_handler (void) {
	
	readSerialPort();
	
	checkPdpContextToOpenOrClose();
	checkSslContextToOpen();
	checkSocketToOpenOrClose();
	checkSocketToSendData();
	checkSocketToReceiveData();
	
	if (softTimer_expired(&timerTestModule)) {
		softTimer_restart(&timerTestModule);
		flags2.bits.testModule = 1;
	}
	
	switch(module_state) {
		case module_idle:
		
			numberOfModuleErrors = 0;
		
			if (flags2.bits.initModule == 1)
				module_gotoState(module_init, module_init_turnoffModulePower);
			else if (flags.bits.registred == 0)
				module_gotoState(module_register, module_register_init);
			else if (flags3.bits.changeCellularNetworks)
				module_gotoState(module_changeNetworks, module_changeNetworks_sendNewscanmode);
			else if (flags3.bits.changeBands)
				module_gotoState(module_changeBands, module_changeBands_sendQcfgBand);
			else if (flags2.bits.testModule)
				module_gotoState(module_testModule, module_test_init);
			else if ((flags2.bits.connectInternet && softTimer_expired(&softtimertimerRetryConnectInternet)) || flags2.bits.disconnectInternet)
				module_gotoState(module_connectToInternet, module_internet_init);
			else if ((flags3.bits.openSslContext && softTimer_expired(&softtimertimerRetryConnectInternet)))
				module_gotoState(module_openSslContext, module_openSslContext_init);
			else if (flags3.bits.mqttConfigure)
				module_gotoState(module_mqttConfigure, module_mqttConfigure_init);
			else if (flags3.bits.mqttConnect && mqttClient.configured)
				module_gotoState(module_mqttConnect, module_mqttConnect_init);
			else if (flags3.bits.mqttSubscribe && mqttClient.connected)
				module_gotoState(module_mqttSubscribe, module_mqttSubscribe_init);
			else if (flags3.bits.mqttPublish && mqttClient.connected)
				module_gotoState(module_mqttPublish, module_mqttPublish_init);
			else if (flags3.bits.mqttDisconnect && mqttClient.connected)
				module_gotoState(module_mqttDisconnect, module_mqttDisconnect_init);
			else if (flags2.bits.openSocket)
				module_gotoState(module_openSocket, module_open_init);
			else if (flags2.bits.closeSocket)
				module_gotoState(module_closeSocket, module_close_init);
			else if (flags2.bits.sendData) {
				if (connectionIdSendData->connected)
					module_gotoState(module_sendData, module_send_init);
			}
			else if (flags2.bits.receiveData) {
				if (connectionIdReceiveData->connected)
					module_gotoState(module_receiveData, module_receive_init);
			}
			break;
			
		case module_init:
			switch (module_substate) {
				case module_init_turnoffModulePower:
					// Se desalimenta el módulo
					port_pin_set_output_level(BG96_POWER, true);				
					softTimer_init(&timerModuleFsm, 6000);
					
					module_gotoSubstate(module_init_waitPowerOff);
					break;
					
				case module_init_waitPowerOff:
					if (softTimer_expired(&timerModuleFsm)) {
						// Se alimenta el módulo
						port_pin_set_output_level(BG96_POWER, false);	
						nop();
						nop();
						nop();
						nop();		
						port_pin_set_output_level(BG96_POWER_KEY, false);
						nop();
						nop();
						nop();
						nop();
						port_pin_set_output_level(BG96_DTR, false);
						
						flags.bits.simAvailable = 0;
						flags.bits.simWithPin = 0;
						
						softTimer_init(&timerModuleFsm, 4000);
						module_gotoSubstate(module_init_turnOnOffModule);
					}
					break;
					
				case module_init_turnOnOffModule:
					if (softTimer_expired(&timerModuleFsm)) {
						// Inicio un pulso en POWER KEY para encender  o apagar el modulo
						port_pin_set_output_level(BG96_POWER_KEY, true);
					
						// La duración del pulso depende de si el módulo ya estab encedido o no
						if (bg96Status) {
							// El módulo estaba encendido. Se lo apaga
							softTimer_init(&timerModuleFsm, 800);
						}
						else {
							// El módulo estaba apagado. Se lo enciende
							softTimer_init(&timerModuleFsm, 2400);
						}
					
						module_gotoSubstate(module_init_powerKeyPulse);
					}
					break;
					
				case module_init_powerKeyPulse:
					if (softTimer_expired(&timerModuleFsm)) {
						port_pin_set_output_level(BG96_POWER_KEY, false);
						
						//Se esperan 8 segundos para empezar a mandar comandos
						softTimer_init(&timerModuleFsm, 8000);
						module_gotoSubstate(module_init_waitingPowerOn);
					}
					break;
					
				case module_init_waitingPowerOn:
					if (softTimer_expired(&timerModuleFsm)) {
						if (bg96Status) {
							// Inició el módulo
							resetModuleData();
							
							module_gotoSubstate(module_init_sendCpin);
						}
						else
							module_gotoSubstate(module_init_waitPowerOff);
					}
					break;
					
				case module_init_sendCpin:
					flags.bits.callReady = 0;
					uartRB_writeString(&uartRB , bg96_cpin);
					
					module_gotoSubstateWithTimeOut(module_init_waitingCpinOk, 16000);
					break;
					
				case module_init_waitingCpinOk:
					if (module_waitingOk()) {
						if (flags.bits.simWithPin) {
							
							if (simPin[0] != NULL) {
								module_gotoSubstate(module_init_sendCpin1);
							}
							else {
								// La SIM tiene pin pero el producto no tiene
								if (moduleCallback != NULL) {
									moduleErrorPayload.error = module_error_sim_with_pin_but_device_without_pin;
									moduleCallback(bg96_module_error, (void*)&moduleErrorPayload);
								}
								
								module_gotoSubstate(module_init_error);
							}
							
						}
						else if (simPin[0] != 0){
							// La SIM no tiene pin pero el producto sí tiene
							if (moduleCallback != NULL) {
								moduleErrorPayload.error = module_error_sim_without_pin_but_device_with_pin;
								moduleCallback(bg96_module_error, (void*)&moduleErrorPayload);
							}
							
							module_gotoSubstate(module_init_error);
						}
						else {
							if (flags.bits.simAvailable)
								module_gotoSubstate(module_init_sendAte);
							else {
								// O no hay SIM o tiene algún problema
								if (moduleCallback != NULL) {
									moduleErrorPayload.error = module_error_sim_error;
									moduleCallback(bg96_module_error, (void*)&moduleErrorPayload);
								}
								
								module_gotoSubstate(module_init_error);
							}
						}
					}
					break;
					
				case module_init_sendAte:
					uartRB_writeString(&uartRB , bg96_ate1);
					module_gotoSubstateWithTimeOut(module_init_waitingAteOk, 16000);
					break;
					
				case module_init_waitingAteOk:
					if (module_waitingOk())
						module_gotoSubstate(module_init_sendQinistat);
						
					break;
					
				case module_init_sendQinistat:
					// Cuenta intentos de preguntas sobre el estado de la SIM
					module_aux1 = 0;
					uartRB_writeString(&uartRB , bg96_qinistat);
					module_gotoSubstateWithTimeOut(module_init_waitingQinistatOk, 16000);
					break;
					
				case module_init_waitingQinistatOk:
					if (module_waitingOk()) {
						if (flags.bits.callReady)
							module_gotoSubstate(module_init_sendGsn);
						else {
							module_aux1 ++;
							if (module_aux1 >= 32)
								module_gotoSubstate(module_init_waitPowerOff);
							else
								module_gotoSubstate(module_init_sendQinistat);
						}
						
					}
					break;
					
				case module_init_sendGsn:
					flags.bits.imeiReceived = 0;
					uartRB_writeString(&uartRB , bg96_gsn);
					module_gotoSubstateWithTimeOut(module_init_waitingGsnOk, 16000);
					
					break;
				
				case module_init_waitingGsnOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_init_sendQccid);
						
					break;
					
				case module_init_sendQccid:
					uartRB_writeString(&uartRB , bg96_qccid);
					module_gotoSubstateWithTimeOut(module_init_waitingQccidOk, 16000);
					
					break;
					
				case module_init_waitingQccidOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_init_sendQcslk);
						
					break;
					
				case module_init_sendQcslk:
					uartRB_writeString(&uartRB , bg96_qsclk);
					module_gotoSubstateWithTimeOut(module_init_waitingQsclkOk, 16000);
					
					break;
					
				case module_init_waitingQsclkOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_init_sendCscs);
					break;
					
				case module_init_sendCscs:
					uartRB_writeString(&uartRB , bg96_cscs);
					module_gotoSubstateWithTimeOut(module_init_waitingCscsOk, 16000);
					
					break;
					
				case module_init_waitingCscsOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_init_sendQurccfg);
						
					break;
					
				case module_init_sendQurccfg:
					uartRB_writeString(&uartRB , bg96_qurccfg);
					module_gotoSubstateWithTimeOut(module_init_waitingQurccfgOk, 16000);
					
					break;
					
				case module_init_waitingQurccfgOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_init_sendNwsscanmode);
						
					break;
					
				case module_init_sendNwsscanmode:
					uartRB_writeString(&uartRB , bg96_qcfg_nwscanmode);
					
					if (catMEnabled)
						uartRB_writeByte(&uartRB, '0');		// Escaneo automático (CatM1 y 2G)
					else
						uartRB_writeByte(&uartRB, '1');		// Solo 2G
					
					uartRB_writeByte(&uartRB, '\r');
					
					module_gotoSubstateWithTimeOut(module_init_waitingNwsscanmodeOk, 16000);
					
					break;
					
				case module_init_waitingNwsscanmodeOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_init_sendIfc);
						
					break;
					
				case module_init_sendIfc:
					uartRB_writeString(&uartRB , bg96_ifc);
					module_gotoSubstateWithTimeOut(module_init_waitingIfcOk, 16000);
					
					break;
					
				case module_init_waitingIfcOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_init_sendCfun);
						
					break;
					
				case module_init_sendCfun:
					uartRB_writeString(&uartRB , bg96_cfun);
					module_gotoSubstateWithTimeOut(module_init_waitingCfunOk, 16000);
					
					break;
					
				case module_init_waitingCfunOk:
					if (module_waitingOk())
						module_gotoSubstate(module_init_sendBand);
						
					break;
					
				case module_init_sendBand:
					if (allBands4g)
						uartRB_writeString(&uartRB , bg96_qcfg_band_todas_bg96);
					else
						uartRB_writeString(&uartRB , bg96_qcfg_band_reducido_bg96);
					
					module_gotoSubstateWithTimeOut(module_init_waitingBandOk, 16000);
					
					break;
					
				case module_init_waitingBandOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_init_sendNwscanseq);
					break;
					
				case module_init_sendNwscanseq:
					uartRB_writeString(&uartRB , bg96_qcfg_nwscanseq);
					module_gotoSubstateWithTimeOut(module_init_waitingNwscanseqOk, 16000);
					
					break;
					
				case module_init_waitingNwscanseqOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_init_sendIotopmode);
					break;
					
				case module_init_sendIotopmode:
					uartRB_writeString(&uartRB , bg96_qcfg_iotopmode);
					module_gotoSubstateWithTimeOut(module_init_waitingIotopmodeOk, 16000);
					
					break;
					
				case module_init_waitingIotopmodeOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_init_sendCfun0);
						
					break;
					
				case module_init_sendCfun0:
					uartRB_writeString(&uartRB , bg96_cfun0);
					module_gotoSubstateWithTimeOut(module_init_waitingCfun0Ok, 16000);
					
					break;
					
				case module_init_waitingCfun0Ok:
					if (module_waitingOk()) 
						module_gotoSubstate(module_init_sendCfun1);
						
					break;
					
				case module_init_sendCfun1:
					uartRB_writeString(&uartRB , bg96_cfun1);
					
					softTimer_init(&timerModuleFsm, 2000);
					module_gotoSubstateWithTimeOut(module_init_waitingCfun1Ok, 16000);
					
					break;
					
				case module_init_waitingCfun1Ok:
					if (module_waitingOk())
						module_gotoSubstate(module_init_sendCpin2);
						
					break;
					
				case module_init_sendCpin2:
					// Se vuelve a desbloquear la SIM, pero esta vez ya se que es la correcta
					if (softTimer_expired(&timerModuleFsm)) {
						uartRB_writeString(&uartRB , bg96_cpin);
						module_gotoSubstateWithTimeOut(module_init_waitingCpinOk2, 16000);
					}
					break;
					
				case module_init_waitingCpinOk2:
					if (module_waitingOk()) {
						if (flags.bits.simWithPin)
							module_gotoSubstate(module_init_sendCpin1_2);
						else
							module_gotoSubstate(module_init_sendSave);
					}
					break;
					
				case module_init_sendCpin1_2:
					uartRB_writeString(&uartRB , bg96_cpin1);
					uartRB_writeBytes(&uartRB, simPin, 4);
					uartRB_writeByte(&uartRB, '\r');
							
					module_gotoSubstateWithTimeOut(module_init_waitingCpin1_2Ok, 16000);
					break;
					
				case module_init_waitingCpin1_2Ok:
					if (module_waitingOk()) {
						module_gotoSubstate(module_init_sendCpin3);
					}
					break;
					
				case module_init_sendCpin3:
					uartRB_writeString(&uartRB , bg96_cpin);
					module_gotoSubstateWithTimeOut(module_init_waitingCpinOk3, 16000);
					break;
					
				case module_init_waitingCpinOk3:
					if (module_waitingOk())
						module_gotoSubstate(module_init_sendSave);
					break;
					
				case module_init_sendSave:
					uartRB_writeString(&uartRB , bg96_save);
					module_gotoSubstateWithTimeOut(module_init_waitingSaveOk, 16000);
					
					break;
					
				case module_init_waitingSaveOk:
					if (module_waitingOk()) {
						if (flags.bits.moduleOn) {
							// El módulo celular está encendido y configurado
							flags2.bits.initModule = 0;

							module_gotoState(module_idle, module_null);
						}
						else
							module_gotoSubstateWithTimeOut(module_init_waitPowerOff, 16000);
					}
					break;
					
				case module_init_sendCpin1:
					uartRB_writeString(&uartRB , bg96_cpin1);
					uartRB_writeBytes(&uartRB, simPin, 4);
					uartRB_writeByte(&uartRB, '\r');
					
					module_gotoSubstateWithTimeOut(module_init_waitingCpin1Ok, 2000);
					break;
					
				case module_init_waitingCpin1Ok:
					if (module_waitingOk()) {
						module_gotoSubstate(module_init_sendCpinAgain);
					}
					break;
					
				case module_init_sendCpinAgain:
					uartRB_writeString(&uartRB , bg96_cpin);				
					module_gotoSubstateWithTimeOut(module_init_waitingCpinAgainOk, 2000);
					break;
					
				case module_init_waitingCpinAgainOk:
					if (module_waitingOk()) {
						if (flags.bits.simAvailable)
							module_gotoSubstate(module_init_sendAte);
						else {
							// SIM y equipo tienen pin distintos
							if (moduleCallback != NULL) {
								moduleErrorPayload.error = module_error_sim_and_device_different_pin;
								moduleCallback(bg96_module_error, (void*)&moduleErrorPayload);
							}
							
							module_gotoSubstate(module_init_error);
						}
					}
					break;
					
				case module_init_error:
					// Se queda en este estado sin hacer nada. Una capa superior del firmware debe tomar
					// alguna acción.
					break;
				
			}
			break;
			
		case module_register:
			switch (module_substate) {
				case module_register_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);
					
					// Cuenta la cantidad de intentos de registro
					module_aux1 = 0;
					
					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_register_waitInitialDelay);
					break;
					
				case module_register_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm)) 
						module_gotoSubstate(module_register_sendCreg0);
						
					break;
					
				case module_register_sendCreg0:
					uartRB_writeString(&uartRB , bg96_creg0);
					module_gotoSubstateWithTimeOut(module_register_waitingCregOk, 16000);
					
					break;
					
				case module_register_waitingCregOk:
					if (module_waitingOk()) {
						softTimer_init(&timerModuleFsm, 2000);
						module_gotoSubstate(module_register_waitInitialDelay2);
					}
					break;
					
				case module_register_waitInitialDelay2:
					if (softTimer_expired(&timerModuleFsm)) 
						module_gotoSubstate(module_register_sendAtCreg);
						
					break;
					
				case module_register_sendAtCreg:
					uartRB_writeString(&uartRB , bg96_atcreg);
					module_gotoSubstateWithTimeOut(module_register_waitingAtCregOk, 16000);
					serialFlags1.bits.sentCREG = 1;
					
					break;
					
				case module_register_waitingAtCregOk:
					if (module_waitingOk()) {
						module_aux1 ++;
						
						if (module_aux1 < 32) {
							if (flags.bits.registred == 1) 
								module_gotoSubstate(module_register_sendCreg2);
							else {
								softTimer_init(&timerModuleFsm, 2000);
								module_gotoSubstate(module_register_waitInitialDelay2);
							}
						}
						else {
							module_gotoSubstate(module_register_turnOffModule);
						}
					}
					break;
					
				case module_register_sendCreg2:
					uartRB_writeString(&uartRB , bg96_creg2);
					module_gotoSubstateWithTimeOut(module_register_waitingCreg2Ok, 16000);
					
					break;
					
				case module_register_waitingCreg2Ok:
					if (module_waitingOk()) 
						module_gotoSubstate(module_register_sendQnwinfo);
						
					break;
					
				case module_register_sendQnwinfo:
					uartRB_writeString(&uartRB , bg96_qnwinfo);
					module_gotoSubstateWithTimeOut(module_register_waitingQnwinfoOk, 16000);
					
					break;
					
				case module_register_waitingQnwinfoOk:
					if (module_waitingOk()) {
						// Se registró el módulo en la red celular
						flags.bits.registred = 1;
						
						for (uint32_t i = 0; i < BG96_PDP_CONTEXTS; i++)
							pdpContexts[i].opened = false;
						
						if (moduleCallback != NULL) {
							moduleStateChangePayload.state = module_states_change_registred;
							moduleCallback(bg96_module_stateChange, (void*)&moduleStateChangePayload);
						}
						
						module_gotoState(module_idle, module_null);
					}
					break;
					
				case module_register_turnOffModule:
					uartRB_writeString(&uartRB , bg96_qpowd);
					
					softTimer_init(&timerModuleFsm, 16000);
					module_gotoSubstate(module_register_checkStatusSignal);
					break;
					
				case module_register_checkStatusSignal:
					if (!bg96Status) {
						flags.bits.moduleOn = 0;
						flags2.bits.initModule = 1;
						module_gotoState(module_idle, module_null);
					}
					else if (softTimer_expired(&timerModuleFsm)) {
						handleError();
					}
					break;
			}
			break;
		
		case module_connectToInternet:
			switch (module_substate) {
				case module_internet_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					// Cuenta la cantidad de intentos de conexión a internet
					module_aux1 = 0;

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_internet_waitInitialDelay);
					break;
					
				case module_internet_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm)) {
						if (pdpContextIdOpenClose->opened && flags2.bits.disconnectInternet == 1)
							module_gotoSubstate(module_internet_sendCipshut);
						else {
							module_aux1 ++;
							if (module_aux1 > 4) {
								pdpContextIdOpenClose->opened = false;
								flags2.bits.disconnectInternet = 0;
								flags.bits.moduleOn = 0;
								flags2.bits.initModule = 1;
								
								module_gotoState(module_idle, module_null);
							}
							else {
								module_gotoSubstate(module_internet_sendQicsgp);
							}
						}
					}
					break;
					
				case module_internet_sendQicsgp:
					uartRB_writeString(&uartRB , bg96_qicsgp);
					uartRB_writeByte(&uartRB, pdpContextIdOpenClose->contextId + '0');
					uartRB_writeString(&uartRB , ",1,\"");
					
					if (pdpContextIdOpenClose->customApnServer != NULL && pdpContextIdOpenClose->customApnServerUser != NULL && pdpContextIdOpenClose->customApnServerPassword != NULL) {
						uartRB_writeString(&uartRB , pdpContextIdOpenClose->customApnServer);
						uartRB_writeString(&uartRB , bg96_separador);
						uartRB_writeString(&uartRB , pdpContextIdOpenClose->customApnServerUser);
						uartRB_writeString(&uartRB , bg96_separador);
						uartRB_writeString(&uartRB , pdpContextIdOpenClose->customApnServerPassword);
					}
					else if (phoneCarrier == BG96_PHONE_CARRIER_MOVISTAR) {
						uartRB_writeString(&uartRB , bg96_apn_movistar);
						uartRB_writeString(&uartRB , bg96_separador);
						uartRB_writeString(&uartRB , bg96_apn_usr_movistar);
						uartRB_writeString(&uartRB , bg96_separador);
						uartRB_writeString(&uartRB , bg96_apn_pass_movistar);
					}
					else if (phoneCarrier == BG96_PHONE_CARRIER_CLARO) {
						uartRB_writeString(&uartRB , bg96_apn_claro);
						uartRB_writeString(&uartRB , bg96_separador);
						uartRB_writeString(&uartRB , bg96_apn_usr_claro);
						uartRB_writeString(&uartRB , bg96_separador);
						uartRB_writeString(&uartRB , bg96_apn_pass_claro);
					}
					else {
						uartRB_writeString(&uartRB , bg96_apn_personal);
						uartRB_writeString(&uartRB , bg96_separador);
						uartRB_writeString(&uartRB , bg96_apn_usr_personal);
						uartRB_writeString(&uartRB , bg96_separador);
						uartRB_writeString(&uartRB , bg96_apn_pass_personal);
					}
					
					uartRB_writeByte(&uartRB , '"');
					uartRB_writeByte(&uartRB , '\r');
					
					module_gotoSubstateWithTimeOut(module_internet_waitingQicsgpOk, 16000);
					
					break;
						
				case module_internet_waitingQicsgpOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_internet_sendQiact);
						
					break;
					
				case module_internet_sendQiact:
					uartRB_writeString(&uartRB , bg96_qiact);
					uartRB_writeByte(&uartRB, pdpContextIdOpenClose->contextId + '0');
					uartRB_writeByte(&uartRB , '\r');
					
					module_gotoSubstateWithTimeOut(module_internet_waitingQiactOk, 150000);
					
					break;
					
				case module_internet_waitingQiactOk:
					if (module_waitingOk()) 
						module_gotoSubstate(module_internet_sendAtQiact);
						
					break;
					
				case module_internet_sendAtQiact:
					uartRB_writeString(&uartRB , bg96_atqiact);
					module_gotoSubstateWithTimeOut(module_internet_waitingAtQiactOk, 16000);
					
					break;
					
				case module_internet_waitingAtQiactOk:
					if (module_waitingOk()) {
						// TODO cambiar la forma en la que se parsea la respuesta a este comando si hay más de un contexto
						
						if (serialFlags1.bits.ipReceived) {
							// Se conectó el módulo a internet
							pdpContextIdOpenClose->opened = true;
							pdpContextIdOpenClose->open = false;
							pdpContextIdOpenClose->alreadyTryToOpen = false;
							pdpContextIdOpenClose->triesToOpen = 0;
							flags2.bits.disconnectInternet = 0;
							flags2.bits.connectInternet = 0;
							
							// Se copia la IP informada por el módulo
							for (uint32_t i = 0; i < 16; i++)
								pdpContextIdOpenClose->publicIp[i] = publicIp[i];
							
							if (moduleCallback != NULL) {
								contextStateChangePayload.state = module_states_context_opened;
								contextStateChangePayload.contextId = getPdpContextArrayIndex(pdpContextIdOpenClose->contextId);
								contextStateChangePayload.isSsl = false;
								
								moduleCallback(bg96_module_contextStateChange, (void*)&contextStateChangePayload);
							}
							
							module_gotoState(module_idle, module_null);
						}
						else {
							handleError();
						}
					}
					break;
					
				case module_internet_sendCipshut:
					uartRB_writeString(&uartRB , bg96_cipshut);
					uartRB_writeByte(&uartRB, pdpContextIdOpenClose->contextId + '0');
					uartRB_writeByte(&uartRB , '\r');
					
					module_gotoSubstateWithTimeOut(module_internet_waitingCipshutOk, 90000);
					
					break;
					
				case module_internet_waitingCipshutOk:
					if (module_waitingOk()) {
						pdpContextIdOpenClose->opened = false;
						pdpContextIdOpenClose->close = false;
						pdpContextIdOpenClose->alreadyTryToClose = false;
						pdpContextIdOpenClose->triesToClose = 0;
						flags2.bits.disconnectInternet = 0;
						
						softTimer_init(&softtimertimerRetryConnectInternet, 4000);
						
						module_gotoState(module_idle, module_null);
					}
					break;
			}
			break;
			
		case module_testModule:
			switch(module_substate) {
				case module_test_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_test_waitInitialDelay);
					break;
					
				case module_test_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm)) 
						module_gotoSubstate(module_test_sendCfun);
						
					break;
					
				case module_test_sendCfun:
					uartRB_writeString(&uartRB , bg96_cfun);
					module_gotoSubstateWithTimeOut(module_test_waitingCfunOk, 16000);
					
					break;
					
				case module_test_waitingCfunOk:
					if (module_waitingOk()) {
						if (flags.bits.moduleOn == 1)
							module_gotoSubstate(module_test_sendAtCreg);
						else {
							flags2.bits.testModule = 0;
							module_gotoState(module_idle, module_null);
						}
					}
					break;
					
				case module_test_sendAtCreg:
					uartRB_writeString(&uartRB , bg96_atcreg);
					module_gotoSubstateWithTimeOut(module_test_waitingAtCregOk, 16000);
					serialFlags1.bits.sentCREG = 1;
					
					break;
					
				case module_test_waitingAtCregOk:
					if (module_waitingOk()) {
						flags2.bits.testModule = 0;
						module_gotoState(module_idle, module_null);
					}
					break;
			}
			break;
			
		case module_openSslContext:
			switch(module_substate) {
				case module_openSslContext_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					// Cuenta la cantidad de intentos de abrir un contexto SSL
					module_aux1 = 0;

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_openSslContext_waitInitialDelay);
					break;
				
				
				case module_openSslContext_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm)) {
						module_aux1 ++;
						if (module_aux1 > 4) {
							flags3.bits.openSslContext = 0;
							flags.bits.moduleOn = 0;
							flags2.bits.initModule = 1;
								
							module_gotoState(module_idle, module_null);
						}
						else {
							module_gotoSubstate(module_openSslContext_sendQsslcfg);
						}
					}
					break;
					
				case module_openSslContext_sendQsslcfg:
					uartRB_writeString(&uartRB , bg96_qsslcfg);
					uartRB_writeByte(&uartRB, sslContextIdOpen->contextId + '0');
					uartRB_writeString(&uartRB, bg96_separador3);
					uartRB_writeString(&uartRB, sslContextIdOpen->caCertName);
					uartRB_writeByte(&uartRB, '"');
					uartRB_writeByte(&uartRB, '\r');
					
					module_gotoSubstateWithTimeOut(module_openSslContext_waitingQsslcfgOk, 16000);
					break;
					
				case module_openSslContext_waitingQsslcfgOk:
					if (module_waitingOk()) {
						// Se pudo abrir el contexto de SSL
						sslContextIdOpen->alreadyTryToOpen = false;
						sslContextIdOpen->triesToOpen = 0;
						sslContextIdOpen->open = false;
						sslContextIdOpen->opened = true;
						
						if (moduleCallback != NULL) {
							contextStateChangePayload.contextId = getSslContextArrayIndex(sslContextIdOpen->contextId);
							contextStateChangePayload.state = module_states_context_opened;
							contextStateChangePayload.isSsl = true;
							moduleCallback(bg96_module_contextStateChange, (void*)&contextStateChangePayload);
						}
						
						flags3.bits.openSslContext = 0;
						module_gotoState(module_idle, module_null);
					}
					break;
					
			}
			break;
			
		case module_openSocket:
			switch(module_substate) {
				case module_open_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_open_waitInitialDelay);
					break;
					
				case module_open_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm)) 
						module_gotoSubstate(module_open_sendQiopen);
						
					break;
					
				case module_open_sendQiopen: 
					if (!connectionIdOpenClose->isSecure) {
						uartRB_writeString(&uartRB , bg96_cipstart);
						uartRB_writeByte(&uartRB , connectionIdOpenClose->pdpContext + '0');
						uartRB_writeByte(&uartRB , ',');
						uartRB_writeByte(&uartRB , connectionIdOpenClose->connectionId + '0');
						uartRB_writeString(&uartRB , bg96_tcp);
						uartRB_writeString(&uartRB , connectionIdOpenClose->remoteAddress);
						uartRB_writeString(&uartRB , bg96_separador2);
						sprintf(module_auxBuffer1, "%d", connectionIdOpenClose->remotePort);
						uartRB_writeString(&uartRB , module_auxBuffer1);
						uartRB_writeByte(&uartRB , '\r');
					}
					else {
						uartRB_writeString(&uartRB , bg96_qsslopen);
						uartRB_writeByte(&uartRB , connectionIdOpenClose->pdpContext + '0');
						uartRB_writeByte(&uartRB , ',');
						uartRB_writeByte(&uartRB , connectionIdOpenClose->sslContext + '0');
						uartRB_writeByte(&uartRB , ',');
						uartRB_writeByte(&uartRB , connectionIdOpenClose->connectionId + '0');
						uartRB_writeString(&uartRB , bg96_separador3);
						uartRB_writeString(&uartRB , connectionIdOpenClose->remoteAddress);
						uartRB_writeString(&uartRB , bg96_separador2);
						sprintf(module_auxBuffer1, "%d", connectionIdOpenClose->remotePort);
						uartRB_writeString(&uartRB , module_auxBuffer1);
						uartRB_writeByte(&uartRB , '\r');
					}
					
					module_gotoSubstateWithTimeOut(module_open_waitingQiopenOk, 16000);

					break;
					
				case module_open_waitingQiopenOk:
					if (module_waitingOk()) {
						softTimer_init(&timerModuleFsm, 150000);
						module_gotoSubstate(module_open_waitingSocketToBeOpen);
					}
					break;
					
				case module_open_waitingSocketToBeOpen:
					if (softTimer_expired(&timerModuleFsm) || serialFlags1.bits.errorReceived || serialFlags1.bits.closedReceived) {
						// No se abrió el socket
						
						if (socketCallback != NULL) {
							socketConnectedPayload.result = false;
							socketCallback(getSocketArrayIndex(connectionIdOpenClose->connectionId), bg96_socket_connected, (void*)&socketConnectedPayload);
						}
						
						// TODO revisar que sea necesario cerrar el socket al fallar la apertura
						//if (serialFlags1.bits.closedReceived == 0)
						//	tcpSockets[connectionIdOpenClose].disconnect = true;
						
						flags2.bits.openSocket = 0;
						module_gotoState(module_idle, module_null);
					}
					else if (connectionIdOpenClose->connected) {
						// Se pudo abrir el socket
						connectionIdOpenClose->alreadyTriedToConnect = false;
						connectionIdOpenClose->triesToConnect = 0;
						connectionIdOpenClose->connect = false;
						
						if (socketCallback != NULL) {
							socketConnectedPayload.result = true;
							socketCallback(getSocketArrayIndex(connectionIdOpenClose->connectionId), bg96_socket_connected, (void*)&socketConnectedPayload);
						}
						
						flags2.bits.openSocket = 0;
						module_gotoState(module_idle, module_null);
					}
					break;
			}
			break;
			
		case module_closeSocket:
			switch(module_substate) {
				case module_close_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_close_waitInitialDelay);
					break;
				
				case module_close_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm))
						module_gotoSubstate(module_close_sendQiclose);
				
					break;
					
				case module_close_sendQiclose:
					if (!connectionIdOpenClose->isSecure) {
						uartRB_writeString(&uartRB , bg96_qiclose);
						uartRB_writeByte(&uartRB , connectionIdOpenClose->connectionId + '0');
						uartRB_writeByte(&uartRB , '\r');
					}
					else {
						uartRB_writeString(&uartRB , bg96_qsslclose);
						uartRB_writeByte(&uartRB , connectionIdOpenClose->connectionId + '0');
						uartRB_writeByte(&uartRB , '\r');
					}
					
					
					module_gotoSubstateWithTimeOut(module_open_waitingQicloseOk, 16000);
					break;
					
				case module_open_waitingQicloseOk:
					if (module_waitingOk()) {
						// Se cerró el socket
						// TODO llega un URC también indicando que cerró?
						connectionIdOpenClose->alreadyTriedToDisconnect = false;
						connectionIdOpenClose->triesToDisconnect = 0;
						connectionIdOpenClose->disconnect = false;
						connectionIdOpenClose->connected = false;
						
						if (socketCallback != NULL) {
							socketCallback(getSocketArrayIndex(connectionIdOpenClose->connectionId), bg96_socket_closed, NULL);
						}
						
						flags2.bits.closeSocket = 0;
						module_gotoState(module_idle, module_null);
					}
					break;
			}
			break;
			
		case module_sendData:
			switch(module_substate) {
				case module_send_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_send_waitInitialDelay);
					break;
					
				case module_send_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm))
						module_gotoSubstate(module_send_sendQisend);
						
					break;
					
				case module_send_sendQisend:
					if (!connectionIdSendData->isSecure) {
						uartRB_writeString(&uartRB , bg96_qisend);
						uartRB_writeByte(&uartRB , connectionIdSendData->connectionId + '0');
						uartRB_writeByte(&uartRB , '\r');
					}
					else {
						uartRB_writeString(&uartRB , bg96_qsslsend);
						uartRB_writeByte(&uartRB , connectionIdSendData->connectionId + '0');
						uartRB_writeByte(&uartRB , '\r');
					}
					
					module_gotoSubstateWithTimeOut(module_send_waitingPrompt, 16000);
					break;
					
				case module_send_waitingPrompt:
					if(module_waitingPrompt()) {
						uartRB_writeBytes(&uartRB , connectionIdSendData->dataToSend, connectionIdSendData->dataToSendLen);
						uartRB_writeByte(&uartRB , 0x1a);
						
						softTimer_init(&timerModuleFsm, 20000);
						module_gotoSubstate(module_send_waitingSendOk);
					}
					break;
					
				case module_send_waitingSendOk:
					if (softTimer_expired(&timerModuleFsm) || serialFlags1.bits.errorReceived || serialFlags1.bits.closedReceived) {
						// Error al enviar la información
						module_gotoState(module_idle, module_null);
					}
					else if (serialFlags1.bits.tcpSendOk) {
						// Se enviaron los datos
						connectionIdSendData->alreadyTriedToSendData = false;
						connectionIdSendData->triesToSendData = 0;
						connectionIdSendData->sendData = false;

						if (socketCallback != NULL) {
							socketCallback(getSocketArrayIndex(connectionIdSendData->connectionId), bg96_socket_dataSent, NULL);
						}

						flags2.bits.sendData = 0;
						module_gotoState(module_idle, module_null);
					}
					break;
					
				
			}
			break;
			
			
		case module_receiveData:
			switch(module_substate) {
				case module_receive_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_receive_waitInitialDelay);
					
					break;
					
				case module_receive_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm))
						module_gotoSubstate(module_receive_sendQird);
						
					break;
					
				case module_receive_sendQird:
					if (!connectionIdReceiveData->isSecure) {
						uartRB_writeString(&uartRB , bg96_qird);
						uartRB_writeByte(&uartRB , connectionIdReceiveData->connectionId + '0');
						uartRB_writeString(&uartRB , ",100");
						uartRB_writeByte(&uartRB , '\r');
					}else {
						uartRB_writeString(&uartRB , bg96_qsslrecv);
						uartRB_writeByte(&uartRB , connectionIdReceiveData->connectionId + '0');
						uartRB_writeString(&uartRB , ",100");
						uartRB_writeByte(&uartRB , '\r');
					}
					
					softTimer_init(&timerModuleFsm, 16000);
					module_gotoSubstate(module_receive_waitingReceiveOk);
					break;
					
				case module_receive_waitingReceiveOk:
					if (softTimer_expired(&timerModuleFsm) || serialFlags1.bits.errorReceived || serialFlags1.bits.closedReceived) {
						// Error al recibir la información
						module_gotoState(module_idle, module_null);
					}
					else if (serialFlags1.bits.dataReceiveCompleted) {
						// Se terminaron de recibir los datos
						connectionIdReceiveData->alreadyTriedToReceiveData = false;
						connectionIdReceiveData->triesToReceiveData = 0;
						connectionIdReceiveData->readData = false;

						if (socketCallback != NULL) {
							socketDataReceivedPayload.data = connectionIdReceiveData->dataReceived;
							socketDataReceivedPayload.length =  connectionIdReceiveData->dataReceivedLen;
							socketCallback(getSocketArrayIndex(connectionIdReceiveData->connectionId), bg96_socket_dataReceived, (void*)&socketDataReceivedPayload);
						}

						flags2.bits.receiveData = 0;
						module_gotoState(module_idle, module_null);
					}
					break;
			}
			break;
			
		case module_changeNetworks:
			switch(module_substate) {
				case module_changeNetworks_sendNewscanmode:
					uartRB_writeString(&uartRB, bg96_qcfg_nwscanmode);
					
					if (catMEnabled)
						uartRB_writeByte(&uartRB, '0');
					else
						uartRB_writeByte(&uartRB, '1');
						
					uartRB_writeByte(&uartRB, '\r');
					
					module_gotoSubstateWithTimeOut(module_changeNetworks_waitingSendNewscanmodeOk, 16000);
					break;
					
				case module_changeNetworks_waitingSendNewscanmodeOk:
					if (module_waitingOk()) 
						module_gotoState(module_idle, module_null);
						
					break;
			}
			break;
			
		case module_changeBands:
			switch(module_substate) {
				case module_changeBands_sendQcfgBand:
					// TODO este comando es disitnto para BG96 y BG95
					if (allBands4g)
						uartRB_writeString(&uartRB, bg96_qcfg_band_all_bg96);
					else
						uartRB_writeString(&uartRB, bg96_qcfg_band_reduced_bg96);
						
					module_gotoSubstateWithTimeOut(module_changeBands_waitingQcfgBandOk, 16000);
					break;
					
				case module_changeBands_waitingQcfgBandOk:
					if (module_waitingOk()) 
						module_gotoState(module_idle, module_null);
						
					break;
			}
			break;
			
		case module_mqttConfigure:
			switch(module_substate) {
				case module_mqttConfigure_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_mqttConfigure_waitInitialDelay);
					break;
					
				case module_mqttConfigure_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm))
						module_gotoSubstate(module_mqttConfigure_sendCfgPdp);
					
					break;
					
				case module_mqttConfigure_sendCfgPdp:
					uartRB_writeString(&uartRB, bg96_mqttCfg_pdpcid);
					uartRB_writeByte(&uartRB, pdpContexts[mqttClient.config.pdpContextId].contextId + '0');
					uartRB_writeByte(&uartRB, '\r');
					
					module_gotoSubstateWithTimeOut(module_mqttConfigure_waitingCfgPdpOk, 1000);
					
					break;
					
				case module_mqttConfigure_waitingCfgPdpOk:
					if (module_waitingOk())
						module_gotoSubstate(module_mqttConfigure_sendCfgSsl);
						
					break;
					
				case module_mqttConfigure_sendCfgSsl:
					uartRB_writeString(&uartRB, bg96_mqttCfg_ssl);
					
					if (mqttClient.config.ssl)
						uartRB_writeByte(&uartRB, '1');
					else
						uartRB_writeByte(&uartRB, '0');
					
					uartRB_writeByte(&uartRB, ',');
					uartRB_writeByte(&uartRB, sslContexts[mqttClient.config.sslContextId].contextId + '0');
					uartRB_writeByte(&uartRB, '\r');
					
					module_gotoSubstateWithTimeOut(module_mqttConfigure_waitingCfgSslOk, 1000);
					
					break;
					
				case module_mqttConfigure_waitingCfgSslOk:
					if (module_waitingOk())
						module_gotoSubstate(module_mqttConfigure_sendCfgKeepalive);
						
					break;
					
				case module_mqttConfigure_sendCfgKeepalive:
					uartRB_writeString(&uartRB, bg96_mqttCfg_keepalive);
					
					sprintf(module_auxBuffer1, "%d", mqttClient.config.keepaliveTime);
					uartRB_writeString(&uartRB , module_auxBuffer1);
					uartRB_writeByte(&uartRB, '\r');
					
					module_gotoSubstateWithTimeOut(module_mqttConfigure_waitingCfgKeepaliveOk, 1000);
					
					break;
					
				case module_mqttConfigure_waitingCfgKeepaliveOk:
					if (module_waitingOk())
						module_gotoSubstate(module_mqttConfigure_sendCfgSession);
						
					break;
					
				case module_mqttConfigure_sendCfgSession:
					uartRB_writeString(&uartRB, bg96_mqttCfg_session);
					
					if (mqttClient.config.cleanSession)
						uartRB_writeByte(&uartRB, '1');
					else
						uartRB_writeByte(&uartRB, '0');
						
					uartRB_writeByte(&uartRB, '\r');
					
					module_gotoSubstateWithTimeOut(module_mqttConfigure_waitingCfgSessionOk, 1000);
					
					break;
					
				case module_mqttConfigure_waitingCfgSessionOk:
					if (module_waitingOk()) {
						if (mqttClient.config.will_topic[0] != '\0' && mqttClient.config.will_msg[0] != '\0')
							module_gotoSubstate(module_mqttConfigure_sendCfgWill);
						else {
							mqttClient.configured = true;
							flags3.bits.mqttConfigure = 0;
							
							module_gotoState(module_idle, module_null);
						}
						
					}
					break;
					
				case module_mqttConfigure_sendCfgWill:
					uartRB_writeString(&uartRB, bg96_mqttCfg_will);
					uartRB_writeByte(&uartRB, mqttClient.config.will_qos + '0');
					uartRB_writeByte(&uartRB, ',');
					
					if (mqttClient.config.will_retain)
						uartRB_writeByte(&uartRB, '1');
					else
						uartRB_writeByte(&uartRB, '0');
					
					uartRB_writeString(&uartRB, bg96_separador3);
					uartRB_writeString(&uartRB, mqttClient.config.will_topic);
					uartRB_writeString(&uartRB, bg96_separador);
					uartRB_writeString(&uartRB, mqttClient.config.will_msg);
					uartRB_writeByte(&uartRB, '\"');
					uartRB_writeByte(&uartRB, '\r');
					
					module_gotoSubstateWithTimeOut(module_mqttConfigure_waitingCfgWillOk, 1000);
					
					break;
					
				case module_mqttConfigure_waitingCfgWillOk:
					if (module_waitingOk()) {
						mqttClient.configured = true;
						flags3.bits.mqttConfigure = 0;
						
						module_gotoState(module_idle, module_null);
					}
					break;
			}
			break;
			
		case module_mqttConnect:
			switch (module_substate) {
				case module_mqttConnect_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_mqttConnect_waitInitialDelay);
					
					break;
					
				case module_mqttConnect_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm))
						module_gotoSubstate(module_mqttConnect_sendOpen);
						
					break;
					
				case module_mqttConnect_sendOpen:
					uartRB_writeString(&uartRB, bg96_mqttOpen);
					uartRB_writeString(&uartRB, mqttClient.config.brokerUrl);
					uartRB_writeString(&uartRB, bg96_separador2);
					
					sprintf(module_auxBuffer1, "%d", mqttClient.config.brokerPort);
					uartRB_writeString(&uartRB , module_auxBuffer1);
					uartRB_writeByte(&uartRB, '\r');
					
					module_gotoSubstateWithTimeOut(module_mqttConnect_waitingOpenOk, 1500);
					break;
					
				case module_mqttConnect_waitingOpenOk:
					if (module_waitingOk()) {				
						softTimer_init(&timerModuleFsm, 75000);
						module_gotoSubstate(module_mqttConnect_waitingOpen);
					}
					break;
					
				case module_mqttConnect_waitingOpen:			
					if (softTimer_expired(&timerModuleFsm) || serialFlags1.bits.errorReceived) {
						// No pudo abrir la conexión con el broker
						mqttClient.opened = false;
						flags3.bits.mqttConnect = 0;
						
						if (mqttCallback != NULL) {
							mqttErrorPayload.error = mqtt_error_cant_connect;
							mqttCallback(bg96_mqtt_error, (void*)&mqttErrorPayload);
						}

						module_gotoState(module_idle, module_null);
					}
					else if (mqttClient.opened) {
						// Pudo abrir la conexión con el broker						
						module_gotoSubstate(module_mqttConnect_sendConn);
					}
					break;
					
				case module_mqttConnect_sendConn:
					uartRB_writeString(&uartRB, bg96_mqttConn);
					uartRB_writeString(&uartRB, mqttClient.config.clientId);
					uartRB_writeString(&uartRB, bg96_separador);
					uartRB_writeString(&uartRB, mqttClient.config.user);
					uartRB_writeString(&uartRB, bg96_separador);
					uartRB_writeString(&uartRB, mqttClient.config.password);
					uartRB_writeByte(&uartRB, '\"');
					uartRB_writeByte(&uartRB, '\r');
					
					module_gotoSubstateWithTimeOut(module_mqttConnect_waitingConnOk, 1500);
					break;
					
				case module_mqttConnect_waitingConnOk:
					if (module_waitingOk()) {
						softTimer_init(&timerModuleFsm, 10000);
						module_gotoSubstate(module_mqttConnect_waitingConn);
					}
					break;
					
				case module_mqttConnect_waitingConn:
					if (softTimer_expired(&timerModuleFsm) || serialFlags1.bits.errorReceived) {
						// No pudo conectarse al broker
						mqttClient.connected = false;
						flags3.bits.mqttConnect = 0;
						
						
						// TODO revisar por que al mandar el comando QMTCLOSE da error
						//module_gotoSubstate(module_mqttConnect_sendClose);
						
						
						if (mqttCallback != NULL) {
							mqttErrorPayload.error = mqtt_error_cant_connect;
							mqttCallback(bg96_mqtt_error, (void*)&mqttErrorPayload);
						}
						
						module_gotoState(module_idle, module_null);
					}
					else if (mqttClient.connected) {
						// Pudo conectarse al broker
						flags3.bits.mqttConnect = 0;
						
						if (mqttCallback != NULL)
							mqttCallback(bg96_mqtt_connected, NULL);
						
						module_gotoState(module_idle, module_null);
					}
					break;
					
				case module_mqttConnect_sendClose:
					uartRB_writeString(&uartRB, bg96_mqttClose);
					
					module_gotoSubstateWithTimeOut(module_mqttConnect_waitingCloseOk, 1500);
					break;
					
				case module_mqttConnect_waitingCloseOk:
					if (module_waitingOk()) {
						softTimer_init(&timerModuleFsm, 50000);
						module_gotoSubstate(module_mqttConnect_waitingClose);
					}
					break;
					
				case module_mqttConnect_waitingClose:
					if (softTimer_expired(&timerModuleFsm) || serialFlags1.bits.errorReceived) {
						// No pudo cerrar la conexión
						mqttClient.connected = false;
						flags3.bits.mqttConnect = 0;
						
						if (mqttCallback != NULL) {
							mqttErrorPayload.error = mqtt_error_cant_connect;
							mqttCallback(bg96_mqtt_error, (void*)&mqttErrorPayload);
						}
						
						module_gotoState(module_idle, module_null);
					}
					else if (!mqttClient.opened) {
						// Pudo cerrar la conexión
						flags3.bits.mqttConnect = 0;
						
						if (mqttCallback != NULL) {
							mqttErrorPayload.error = mqtt_error_cant_connect;
							mqttCallback(bg96_mqtt_error, (void*)&mqttErrorPayload);
						}
						
						module_gotoState(module_idle, module_null);
					}
					break;
			}
			break;
			
		case module_mqttDisconnect:
			switch (module_substate) {
				case module_mqttDisconnect_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_mqttDisconnect_waitInitialDelay);
					
					break;
					
				case module_mqttDisconnect_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm)) {
						if (mqttClient.connected)
							module_gotoSubstate(module_mqttDisconnect_sendDisc);
						else
							module_gotoSubstate(module_mqttDisconnect_sendClose);
					}
					break;
					
				case module_mqttDisconnect_sendDisc:
					uartRB_writeString(&uartRB, bg96_mqttDisc);
					
					module_gotoSubstateWithTimeOut(module_mqttDisconnect_waitingDiscOk, 1500);
					break;
					
				case module_mqttDisconnect_waitingDiscOk:
					if (module_waitingOk()) {
						softTimer_init(&timerModuleFsm, 50000);
						module_gotoSubstate(module_mqttDisconnect_waitingDisc);
					}
					
					break;
				
				case module_mqttDisconnect_waitingDisc:
					if (softTimer_expired(&timerModuleFsm) || serialFlags1.bits.errorReceived) {
						// No pudo disconectarse del broker
						flags3.bits.mqttDisconnect = 0;
						
						if (mqttCallback != NULL) {
							mqttErrorPayload.error = mqtt_error_cant_disconnect;
							mqttCallback(bg96_mqtt_error, (void*)&mqttErrorPayload);
						}
						
						module_gotoState(module_idle, module_null);
					}
					else if (!mqttClient.connected) {
						// Pudo disconectarse del broker
						
						flags3.bits.mqttDisconnect = 0;
						mqttClient.opened = false;
						mqttClient.connected = false;
						module_gotoState(module_idle, module_null);
						
						// TODO revisar por que al mandar el comando QMTCLOSE da error
						//module_gotoSubstate(module_mqttDisconnect_sendClose);
					}
					break;
					
				case module_mqttDisconnect_sendClose:
					uartRB_writeString(&uartRB, bg96_mqttClose);
					
					module_gotoSubstateWithTimeOut(module_mqttDisconnect_waitingCloseOk, 1500);
					break;
					
				case module_mqttDisconnect_waitingCloseOk:
					if (module_waitingOk()) {
						softTimer_init(&timerModuleFsm, 50000);
						module_gotoSubstate(module_mqttDisconnect_waitingClose);
					}
					break;
				
				case module_mqttDisconnect_waitingClose:
					if (softTimer_expired(&timerModuleFsm) || serialFlags1.bits.errorReceived) {
						// No pudo cerrar la conexión
						flags3.bits.mqttDisconnect = 0;
						
						if (mqttCallback != NULL) {
							mqttErrorPayload.error = mqtt_error_cant_connect;
							mqttCallback(bg96_mqtt_error, (void*)&mqttErrorPayload);
						}
						
						module_gotoState(module_idle, module_null);
					}
					else if (!mqttClient.opened) {
						// Pudo cerrar la conexión
						flags3.bits.mqttDisconnect = 0;
						
						if (mqttCallback != NULL) {;
							mqttCallback(bg96_mqtt_disconnected, NULL);
						}
						
						module_gotoState(module_idle, module_null);
					}
					break;
			}
			break;
			
		case module_mqttPublish:
			switch(module_substate) {
				case module_mqttPublish_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_mqttPublish_waitInitialDelay);
					break;
					
				case module_mqttPublish_waitInitialDelay:
					if (softTimer_expired(&timerModuleFsm))
						module_gotoSubstate(module_send_sendQmtpub);
						
					break;
					
				case module_send_sendQmtpub:
					uartRB_writeString(&uartRB , bg96_mqttPub);
					
					if (mqttClient.qos == 0)
						uartRB_writeByte(&uartRB , '0');
					else
						uartRB_writeByte(&uartRB , '1');
					uartRB_writeByte(&uartRB , ',');
					
					uartRB_writeByte(&uartRB , mqttClient.qos + '0');
					uartRB_writeByte(&uartRB , ',');
						
					if (mqttClient.retain)
						uartRB_writeByte(&uartRB , '1');
					else
						uartRB_writeByte(&uartRB , '0');
							
					uartRB_writeString(&uartRB , bg96_separador3);
					uartRB_writeString(&uartRB , mqttClient.topicToSend);	
							
					uartRB_writeByte(&uartRB , '\"');
					uartRB_writeByte(&uartRB , '\r');
					
					module_gotoSubstateWithTimeOut(module_mqttPublish_waitingPrompt, 16000);
					break;
					
				case module_mqttPublish_waitingPrompt:
					if(module_waitingPrompt()) {
						uartRB_writeBytes(&uartRB , mqttClient.msgToSend, mqttClient.msgToSendLen);
						uartRB_writeByte(&uartRB , 0x1a);
						
						softTimer_init(&timerModuleFsm, 20000);
						module_gotoSubstate(module_mqttPublish_waitingPublishOk);
					}
					break;
					
				case module_mqttPublish_waitingPublishOk:
					if (softTimer_expired(&timerModuleFsm) || serialFlags1.bits.errorReceived || serialFlags1.bits.closedReceived) {
						// Error al enviar la información
						module_gotoState(module_idle, module_null);
					}
					else if (serialFlags2.bits.mqttPublishOk) {
						// Se publícó el mensaje
						flags3.bits.mqttPublish = 0;

						if (mqttCallback != NULL) {
							mqttCallback(bg96_mqtt_msgPublished, NULL);
						}

						module_gotoState(module_idle, module_null);
					}
					break;
					
				
			}
			break;
			
		case module_mqttSubscribe:
			switch(module_substate) {
				case module_mqttSubscribe_init:
					port_pin_set_output_level(BG96_POWER, false);
					port_pin_set_output_level(BG96_DTR, false);

					softTimer_init(&timerModuleFsm, 40);
					module_gotoSubstate(module_mqttSubscribe_waitingInitialDelay);
					
					break;
					
				case module_mqttSubscribe_waitingInitialDelay:
					if (softTimer_expired(&timerModuleFsm))
						module_gotoSubstate(module_mqttSubscribe_sendQmtsub);
						
					break;
					
				case module_mqttSubscribe_sendQmtsub:
					uartRB_writeString(&uartRB , bg96_mqttSub);
					uartRB_writeString(&uartRB , mqttClient.topicToSubscribe);
					uartRB_writeString(&uartRB , bg96_separador2);
					uartRB_writeByte(&uartRB , mqttClient.qosToSubscribe +'0');
					uartRB_writeByte(&uartRB , '\r');
					
					module_gotoSubstateWithTimeOut(module_mqttSubscribe_waitingQmtsubOk, 1500);
					
					break;
					
				case module_mqttSubscribe_waitingQmtsubOk:
					if(module_waitingOk()) {
						softTimer_init(&timerModuleFsm, 20000);
						module_gotoSubstate(module_mqttSubscribe_waitingQmtsub);
					}
					break;
					
				case module_mqttSubscribe_waitingQmtsub:
					if (softTimer_expired(&timerModuleFsm) || serialFlags1.bits.errorReceived || serialFlags1.bits.closedReceived) {
						// No se pudo suscribir
						module_gotoState(module_idle, module_null);
					}
					else if (serialFlags2.bits.mqttSubscribeOk) {
						// Se suscribió
						flags3.bits.mqttSubscribe = 0;

						if (mqttCallback != NULL) {
							mqttCallback(bg96_mqtt_subscribed, NULL);
						}

						module_gotoState(module_idle, module_null);
					}
					break;
			}
			break;
	}
}


void bg96_statusSignal (void) {
	bg96StatusSignal = bg96StatusSignal >> 1;
	
	if (port_pin_get_input_level(BG96_STATUS))
		bit_clear(bg96StatusSignal, 7);
	else
		bit_set(bg96StatusSignal, 7);
		
	// Si 8 mediciones seguidas dan 0, significa que el módulo está apagado
	if (bg96StatusSignal == 0) {
		
		if (bg96Status) {
			flags.bits.moduleOn = 0;
			flags2.bits.initModule = 1;
		}
		
		bg96Status = false;
	}
	else if (bg96StatusSignal == 0xff)
		bg96Status = true;
}


int32_t bg96_getSocket (bool isSecure, uint32_t pdpConectextNumber, uint32_t sslConectextNumber, uint8_t* address, uint32_t port) {
	uint32_t i;
	int32_t ret = -1;
	
	for (i = 0; i < BG96_TCP_SOCKETS; i++) {
		if (tcpSockets[i].free) {
			tcpSockets[i].free = false;
			tcpSockets[i].triesToConnect = 0;
			tcpSockets[i].triesToDisconnect = 0;
			tcpSockets[i].triesToSendData = 0;
			tcpSockets[i].triesToReceiveData = 0;
			tcpSockets[i].alreadyTriedToConnect = false;
			tcpSockets[i].alreadyTriedToDisconnect = false;
			tcpSockets[i].alreadyTriedToSendData = false;
			tcpSockets[i].alreadyTriedToReceiveData = false;
			tcpSockets[i].connected = false;
			tcpSockets[i].connect = false;
			tcpSockets[i].disconnect = false;
			tcpSockets[i].sendData = false;
			tcpSockets[i].readData = false;
			tcpSockets[i].connectionId = i;
			
			if (BG96_PDP_CONTEXTS > 0 && pdpConectextNumber < BG96_PDP_CONTEXTS)
				tcpSockets[i].pdpContext = pdpContexts[pdpConectextNumber].contextId;
				
			if (BG96_SSL_CONTEXTS > 0 && sslConectextNumber < BG96_SSL_CONTEXTS)
				tcpSockets[i].sslContext = sslContexts[sslConectextNumber].contextId;
			
			for (uint32_t j = 0; j < 50 && address[j] != '\0'; j++)
				tcpSockets[i].remoteAddress[j] = address[j];
				
			tcpSockets[i].remotePort = port;
			
			if (isSecure)
				tcpSockets[i].isSecure = true;
			else
				tcpSockets[i].isSecure = false;
				
			ret = i;
			break;
		}
	}
	
	return ret;
}

void bg96_initModule (void) {
	flags2.bits.initModule = 1;
}

void bg96_resetModule (void) {
	flags.bits.moduleOn = 0;
	flags2.bits.initModule = 1;
	flags.bits.registred = 0;
	
	module_gotoState(module_idle, module_null);
}

void bg96_setEnableCatM (bool catm) {
	catMEnabled = catm;
	
	flags3.bits.changeCellularNetworks = 1;
}

void bg96_setAll4gBands (bool bands) {
	allBands4g = bands;
	
	flags3.bits.changeBands = 1;
}


int8_t bg96_getPdpContext (uint8_t* customApn, uint8_t* customApnUser, uint8_t* customApnPassword) {
	uint32_t i;
	int32_t ret = -1;
	
	for (i = 0; i < BG96_PDP_CONTEXTS; i++) {
		if (pdpContexts[i].free) {
			pdpContexts[i].free = false;
			pdpContexts[i].contextId = i + 1;
			pdpContexts[i].opened = false;
			pdpContexts[i].open = false;
			pdpContexts[i].close = false;
			pdpContexts[i].alreadyTryToOpen = false;
			pdpContexts[i].alreadyTryToClose = false;
			pdpContexts[i].triesToOpen = 0;
			pdpContexts[i].triesToClose = 0;
			pdpContexts[i].publicIp[0] = 0xff;
			
			if (customApn != NULL) {
				for (uint32_t j = 0; j < 30 && customApn[j] != '\0'; j++)
				pdpContexts[i].customApnServer[j] = customApn[j];
			}
			else
				customApn = NULL;
			
			if (customApnUser != NULL) {
				for (uint32_t j = 0; j < 20 && customApnUser[j] != '\0'; j++)
				pdpContexts[i].customApnServerUser[j] = customApnUser[j];
			}
			else
				customApnUser = NULL;
			
			if (customApnPassword != NULL) {
				for (uint32_t j = 0; j < 20 && customApnPassword[j] != '\0'; j++)
					pdpContexts[i].customApnServerPassword[j] = customApnPassword[j];
			}
			else
				customApnPassword = NULL;


			ret = i;
			break;
		}
	}
	
	return ret;
}


int8_t bg96_getSslContext (uint8_t* caCert) {
	uint32_t i;
	int32_t ret = -1;
	
	for (i = 0; i < BG96_SSL_CONTEXTS; i++) {
		if (sslContexts[i].free) {
			sslContexts[i].free = false;
			sslContexts[i].contextId = i;
			sslContexts[i].opened = false;
			sslContexts[i].open = false;
			sslContexts[i].alreadyTryToOpen = false;
			sslContexts[i].triesToOpen = 0;
			
			for (uint32_t j = 0; j < 20 && caCert[j] != '\0'; j++)
				sslContexts[i].caCertName[j] = caCert[j];

			ret = i;
			break;
		}
	}
	
	return ret;
}


int32_t bg96_openPdpContext (uint8_t contextId) {
	if (contextId >= BG96_PDP_CONTEXTS)
		return -1;
		
	pdpContexts[contextId].open = true;
	
	return 0;
}


int32_t bg96_closePdpContext (uint8_t contextId) {
	if (contextId >= BG96_PDP_CONTEXTS)
		return -1;
		
	pdpContexts[contextId].close = true;
	return 0;
}




int32_t bg96_openSslContext (uint8_t contextId) {
	if (contextId >= BG96_SSL_CONTEXTS)
		return -1;
		
	sslContexts[contextId].open = true;
	return 0;
}


bool bg96_isBusy (void) {
	return (module_state != module_idle ||
		flags2.bits.initModule ||
		flags2.bits.testModule ||
		flags2.bits.disconnectInternet ||
		flags2.bits.openSocket ||
		flags2.bits.closeSocket ||
		flags2.bits.sendData ||
		flags2.bits.receiveData ||
		flags2.bits.connectInternet ||
		flags2.bits.disconnectInternet ||
		flags3.bits.openSslContext);
}


bool bg96_isRegistred (void) {
	return flags.bits.registred == 1;
}


bool bg96_isPdpContextOpened (uint8_t contextId) {
	return pdpContexts[contextId].opened;
}

bool bg96_isSslContextOpened (uint8_t contextId) {
	return sslContexts[contextId].opened;
}


int32_t bg96_openSocket (uint32_t id) {	
	
	if (id >= BG96_TCP_SOCKETS)
		return -1;
	
	tcpSockets[id].connect = 1;
	tcpSockets[id].triesToConnect = 0;
	tcpSockets[id].alreadyTriedToConnect = false;
	
	return 0;
}


int32_t bg96_closeSocket (uint32_t id) {

	if (id >= BG96_TCP_SOCKETS)
		return -1;
	
	tcpSockets[id].disconnect = 1;
	tcpSockets[id].triesToDisconnect = 0;
	tcpSockets[id].alreadyTriedToDisconnect = false;
	
	return 0;
}


void bg96_socketSendData (uint32_t id, uint8_t* data, uint32_t len) {
	if (id < BG96_TCP_SOCKETS) {
		for (uint32_t i = 0; i < len; i++)
			tcpSockets[id].dataToSend[i] = data[i];
			
		tcpSockets[id].dataToSendLen = len;
		tcpSockets[id].sendData = 1;
	}
}


void bg96_registerModuleCallback (void (*cb)(bg96_module_events, void*)) {
	moduleCallback = cb;
}

void bg96_registerSocketCallback (void (*cb)(uint32_t, bg96_socket_events, void*)) {
	socketCallback = cb;
}


bool bg96_isSocketConnected (uint32_t id) {
	return (!tcpSockets[id].free && tcpSockets[id].connected);
}



void bg96_mqtt_loadConfigurationDefault (bg96_mqtt_configuration_t* config) {
	config->ssl = false;
	config->sslContextId = 0;
	config->pdpContextId = 1;
	config->cleanSession = true;
	config->keepaliveTime = 120;
	config->will_topic[0] = '\0';
	config->will_msg[0] = '\0';
	config->will_qos = 1;
	config->will_retain = true;
	config->brokerUrl[0] = '\0';
	config->brokerPort = 1883;
	config->clientId[0] = '\0';
	config->user[0] = '\0';
	config->password[0] = '\0';
}

void bg96_mqtt_init (bg96_mqtt_configuration_t* config) {
	uint32_t i;
	
	
	mqttClient.config.ssl = config->ssl;
	mqttClient.config.sslContextId = config->sslContextId;
	mqttClient.config.pdpContextId = config->pdpContextId;
	mqttClient.config.cleanSession = config->cleanSession;
	mqttClient.config.keepaliveTime = config->keepaliveTime;
	
	for (i = 0; i < 20 && config->will_topic[i] != '\0'; i++)
		mqttClient.config.will_topic[i] = config->will_topic[i];
	mqttClient.config.will_topic[i] = '\0';
		
	for (i = 0; i < 20 && config->will_msg[i] != '\0'; i++)
		mqttClient.config.will_msg[i] = config->will_msg[i];
	mqttClient.config.will_msg[i] = '\0';
		
	mqttClient.config.will_qos = config->will_qos;
	mqttClient.config.will_retain = config->will_retain;
	
	for (i = 0; i < 50 && config->brokerUrl[i] != '\0'; i++)
		mqttClient.config.brokerUrl[i] = config->brokerUrl[i];
	mqttClient.config.brokerUrl[i] = '\0';
		
	mqttClient.config.brokerPort = config->brokerPort;
	
	for (i = 0; i < 10 && config->clientId[i] != '\0'; i++)
		mqttClient.config.clientId[i] = config->clientId[i];
	mqttClient.config.clientId[i] = '\0';
	
	for (i = 0; i < 10 && config->user[i] != '\0'; i++)
		mqttClient.config.user[i] = config->user[i];
	mqttClient.config.user[i] = '\0';
	
	for (i = 0; i < 10 && config->password[i] != '\0'; i++)
		mqttClient.config.password[i] = config->password[i];
	mqttClient.config.password[i] = '\0';
		
	mqttClient.opened = false;
	mqttClient.connected = false;
	mqttClient.configured = false;
	
	flags3.bits.mqttConfigure = 1;
}


void bg96_mqtt_registerMqttCallback (void (*cb)(bg96_mqtt_events, void*)) {
	mqttCallback = cb;
}


void bg96_mqtt_connect (void) {
	flags3.bits.mqttConnect = 1;
}


void bg96_mqtt_disconnect (void) {
	flags3.bits.mqttDisconnect = 1;
}

bool bg96_mqtt_isConnected (void) {
	return (mqttClient.connected);
}


void bg96_mqtt_subscribe (uint8_t* topic, uint8_t qos) {
	uint32_t i;
	
	for (i = 0; topic[i] != '\0'; i++)
		mqttClient.topicToSubscribe[i] = topic[i];
	mqttClient.topicToSubscribe[i] = '\0';
	
	mqttClient.qosToSubscribe = qos;
	
	flags3.bits.mqttSubscribe = 1;
}


void bg96_mqtt_unsubscribe (uint8_t* topic) {
	
}


void bg96_mqtt_publish (uint8_t* topic, uint8_t* data, uint32_t len, uint8_t qos, bool retain) {
	uint32_t i;
	
	for (i = 0; topic[i] != '\0'; i++)
		mqttClient.topicToSend[i] = topic[i];
	mqttClient.topicToSend[i] = '\0';
	
	for (i = 0; i < len; i++)
		mqttClient.msgToSend[i] = data[i];
	mqttClient.msgToSendLen = len;
	
	mqttClient.qos = qos;
	mqttClient.retain = retain;
			
	flags3.bits.mqttPublish = 1;
}






void handleError (void) {
	
	numberOfModuleErrors ++;
	
	if (numberOfModuleErrors >= 8) {
		switch(module_state) {
			case module_init:
				flags.bits.moduleOn = 0;
				flags2.bits.initModule = 1;
				
				module_gotoState(module_idle, module_null);
				break;
			
			case module_register:
				flags.bits.registred = 0;
			
				module_gotoState(module_idle, module_null);
				break;
			
			case module_connectToInternet:
				pdpContextIdOpenClose->opened = false;
			
				module_gotoState(module_idle, module_null);
				break;
			
			case module_testModule:
			
				module_gotoState(module_idle, module_null);
				break;
				
			case module_openSocket:
			
				module_gotoState(module_idle, module_null);
				break;
				
			case module_closeSocket:
			
				module_gotoState(module_idle, module_null);
				break;
				
			case module_sendData:
			
				module_gotoState(module_idle, module_null);
				break;
				
			case module_receiveData:
			
				break;
				
			case module_openSslContext:
			
				module_gotoState(module_idle, module_null);
				break;
				
			case module_changeNetworks:
				flags.bits.moduleOn = 0;
				flags2.bits.initModule = 1;
				
				module_gotoState(module_idle, module_null);
				break;
				
			case module_changeBands:
				flags.bits.moduleOn = 0;
				flags2.bits.initModule = 1;
				
				module_gotoState(module_idle, module_null);
				break;
				
			case module_mqttConfigure:
				flags.bits.moduleOn = 0;
				flags2.bits.initModule = 1;
				
				module_gotoState(module_idle, module_null);
				break;
				
			case module_mqttConnect:
				flags.bits.moduleOn = 0;
				flags2.bits.initModule = 1;
				
				module_gotoState(module_idle, module_null);
				break;
				
			case module_mqttDisconnect:
				flags.bits.moduleOn = 0;
				flags2.bits.initModule = 1;
				flags3.bits.mqttDisconnect = 0;
				
				module_gotoState(module_idle, module_null);
				break;
				
			case module_mqttPublish:
				flags.bits.moduleOn = 0;
				flags2.bits.initModule = 1;
				
				module_gotoState(module_idle, module_null);
				break;
			
		}
	}
	else {
		switch(module_state) {
			case module_init:
				module_gotoPrevSubState();
				
				break;
			
			case module_register:
				module_gotoPrevSubState();
				
				break;
			
			case module_connectToInternet:
				module_gotoPrevSubState();
				
				break;
			
			case module_testModule:
				module_gotoPrevSubState();
				
				break;
				
			case module_openSocket:
				module_gotoPrevSubState();
				
				break;
				
			case module_closeSocket:
				if (module_substate == module_open_waitingQicloseOk) {
					// Vuelvo a idle para volver a intentar cerrarlo
					module_gotoState(module_idle, module_null);
				}
				else
					module_gotoPrevSubState();
					
				break;
				
			case module_sendData:
				module_gotoPrevSubState();
				
				break;
				
			case module_receiveData:
			
				break;
				
			case module_openSslContext:
				module_gotoPrevSubState();
				
				break;
				
			case module_changeNetworks:
				module_gotoPrevSubState();
				
				break;
				
			case module_changeBands:
				module_gotoPrevSubState();
				
				break;
			
			case module_mqttConfigure:
				module_gotoPrevSubState();
				
				break;
				
			case module_mqttConnect:
				module_gotoPrevSubState();
				
				break;
				
			case module_mqttDisconnect:
				module_gotoPrevSubState();
				
				break;
				
			case module_mqttPublish:
				module_gotoPrevSubState();
			
				break;
		}
	}
}


void checkPdpContextToOpenOrClose(void) {
	uint32_t i;
	
	if (bg96_isBusy())
		return;
		
	// Recorro el array de contextos PDP para determinar si alguno necesita ser abierto o cerrado
	for (i = 0; i < BG96_PDP_CONTEXTS; i++) {
		if (pdpContexts[i].open && !pdpContexts[i].alreadyTryToOpen) {
			if (pdpContexts[i].triesToOpen < 4) {
				flags2.bits.connectInternet = 1;
				pdpContextIdOpenClose = &pdpContexts[i];
				pdpContexts[i].triesToOpen ++;
				pdpContexts[i].alreadyTryToOpen = true;
				break;
			}
			else {
				// Se superó la cantidad de intentos
				if (moduleCallback != NULL) {
					moduleErrorPayload.error = module_error_cant_open_pdp_context;
					moduleErrorPayload.id = pdpContexts[i].contextId;
					
					moduleCallback(bg96_module_error, (void*)&moduleErrorPayload);
				}
			}

		}
		else if (pdpContexts[i].close && !pdpContexts[i].alreadyTryToClose) {
			if (pdpContexts[i].triesToClose < 2) {
				flags2.bits.disconnectInternet = 1;
				pdpContextIdOpenClose = &pdpContexts[i];
				pdpContexts[i].triesToClose ++;
				pdpContexts[i].alreadyTryToClose = true;
				break;
			}
			else {
				// Se superó la cantidad de intentos
				if (moduleCallback != NULL) {
					moduleErrorPayload.error = module_error_cant_close_pdp_context;
					moduleErrorPayload.id = pdpContexts[i].contextId;
					
					moduleCallback(bg96_module_error, (void*)&moduleErrorPayload);
				}
			}
		}
	}
	
	// Si logré recorrer todo el array de contextos, significa que ninguno necesita abrirse o cerrarse
	// o que ya todos intentaron una vez aunque sea. Clereo los flags para que vuelvan a intentarlo, si le quedan intentos
	if (i == BG96_PDP_CONTEXTS) {
		for (i = 0; i < BG96_PDP_CONTEXTS; i++) {
			pdpContexts[i].alreadyTryToOpen = false;
			pdpContexts[i].alreadyTryToClose = false;
		}
	}
}


void checkSslContextToOpen(void) {
	uint32_t i;
	
	if (bg96_isBusy())
	return;
	
	// Recorro el array de contextos PDP para determinar si alguno necesita ser abierto o cerrado
	for (i = 0; i < BG96_SSL_CONTEXTS; i++) {
		if (sslContexts[i].open && !sslContexts[i].alreadyTryToOpen) {
			if (sslContexts[i].triesToOpen < 4) {
				flags3.bits.openSslContext = 1;
				sslContextIdOpen = &sslContexts[i];
				sslContexts[i].triesToOpen ++;
				sslContexts[i].alreadyTryToOpen = true;
				break;
			}
			else {
				// Se superó la cantidad de intentos
				if (moduleCallback != NULL) {
					moduleErrorPayload.error = module_error_cant_open_ssl_context;
					moduleErrorPayload.id = sslContexts[i].contextId;
					
					moduleCallback(bg96_module_error, (void*)&moduleErrorPayload);
				}
			}

		}
	}
	
	// Si logré recorrer todo el array de contextos, significa que ninguno necesita abrirse
	// o que ya todos intentaron una vez aunque sea. Clereo los flags para que vuelvan a intentarlo, si le quedan intentos
	if (i == BG96_SSL_CONTEXTS) {
		for (i = 0; i < BG96_SSL_CONTEXTS; i++) {
			sslContexts[i].alreadyTryToOpen = false;
		}
	}
}


void checkSocketToOpenOrClose(void) {
	uint32_t i;
	
	if (bg96_isBusy())
		return;
	
	// Recorro el array de sockets para determinar si algún socket necesita ser abierto o cerrado
	for (i = 0; i < BG96_TCP_SOCKETS; i++) {
		if (tcpSockets[i].connect && !tcpSockets[i].alreadyTriedToConnect) {
			if (tcpSockets[i].triesToConnect < 2) {
				flags2.bits.openSocket = 1;
				connectionIdOpenClose = &tcpSockets[i];
				tcpSockets[i].triesToConnect ++;
				tcpSockets[i].alreadyTriedToConnect = true;
				break;
			}
			else {
				// Se superó la cantidad de intentos
				if (socketCallback != NULL) {
					socketErrorPayload.error = socket_error_cant_open;
					
					socketCallback(tcpSockets[i].connectionId, bg96_socket_error, (void*)&moduleErrorPayload);
				}
			}

		}
		else if (tcpSockets[i].disconnect && !tcpSockets[i].alreadyTriedToDisconnect) {
			if (tcpSockets[i].triesToDisconnect < 2) {
				flags2.bits.closeSocket = 1;
				connectionIdOpenClose = &tcpSockets[i];
				tcpSockets[i].triesToDisconnect ++;
				tcpSockets[i].alreadyTriedToDisconnect = true;
				break;
			}
			else {
				// Se superó la cantidad de intentos
				if (socketCallback != NULL) {
					socketErrorPayload.error = socket_error_cant_close;
					
					socketCallback(tcpSockets[i].connectionId, bg96_socket_error, (void*)&moduleErrorPayload);
				}
			}
		}	
	}
	
	// Si logré recorrer todo el array de sockets, significa que ninguno necesita abrirse o cerrarse
	// o que ya todos intentaron una vez aunque sea. Clereo los flags para que vuelvan a intentarlo, si le quedan intentos
	if (i == BG96_TCP_SOCKETS) {
		for (i = 0; i < BG96_TCP_SOCKETS; i++) {
			tcpSockets[i].alreadyTriedToConnect = false;
			tcpSockets[i].alreadyTriedToDisconnect = false;
		}
	}
}


void checkSocketToSendData(void) {
	uint32_t i;
	
	if (bg96_isBusy())
		return;
		
	// Recorro el array de sockets para determinar si algún socket necesita ser abierto o cerrado
	for (i = 0; i < BG96_TCP_SOCKETS; i++) {
		if (tcpSockets[i].sendData && !tcpSockets[i].alreadyTriedToSendData) {
			if (tcpSockets[i].triesToSendData < 4) {
				flags2.bits.sendData = 1;
				connectionIdSendData = &tcpSockets[i];
				tcpSockets[i].triesToSendData ++;
				tcpSockets[i].alreadyTriedToSendData = true;
				break;
			}
			else {
				// Se superó la cantidad de intentos
				if (socketCallback != NULL) {
					socketErrorPayload.error = socket_error_cant_send;
					
					socketCallback(tcpSockets[i].connectionId, bg96_socket_error, (void*)&moduleErrorPayload);
				}
			}

		}
	}
	
	// Si logré recorrer todo el array de sockets, significa que ninguno necesita enviar data
	// o que ya todos intentaron una vez aunque sea. Clereo los flags para que vuelvan a intentarlo, si le quedan intentos
	if (i == BG96_TCP_SOCKETS) {
		for (i = 0; i < BG96_TCP_SOCKETS; i++) {
			tcpSockets[i].alreadyTriedToSendData = false;
			tcpSockets[i].alreadyTriedToSendData = false;
		}
	}
}


void checkSocketToReceiveData (void) {
	uint32_t i;
	
	if (bg96_isBusy())
		return;
		
	// Recorro el array de sockets para determinar si algún socket necesita ser abierto o cerrado
	for (i = 0; i < BG96_TCP_SOCKETS; i++) {
		if (tcpSockets[i].readData && !tcpSockets[i].alreadyTriedToReceiveData) {
			if (tcpSockets[i].triesToSendData < 4) {
				flags2.bits.receiveData = 1;
				connectionIdReceiveData = &tcpSockets[i];
				tcpSockets[i].triesToReceiveData ++;
				tcpSockets[i].alreadyTriedToReceiveData = true;
				break;
			}
			else {
				// Se superó la cantidad de intentos
				if (socketCallback != NULL) {
					socketErrorPayload.error = socket_error_cant_read;
					
					socketCallback(tcpSockets[i].connectionId, bg96_socket_error, (void*)&moduleErrorPayload);
				}
			}

		}
	}
	
	// Si logré recorrer todo el array de sockets, significa que ninguno necesita enviar data
	// o que ya todos intentaron una vez aunque sea. Clereo los flags para que vuelvan a intentarlo, si le quedan intentos
	if (i == BG96_TCP_SOCKETS) {
		for (i = 0; i < BG96_TCP_SOCKETS; i++) {
			tcpSockets[i].alreadyTriedToSendData = false;
			tcpSockets[i].alreadyTriedToSendData = false;
		}
	}
}



void usart_read_callback(struct usart_module *const usart_module) {
	uartRB_rxHandler(&uartRB);
}

void usart_write_callback(struct usart_module *const usart_module) {
	uartRB_txHandler(&uartRB);
}


uint8_t getPdpContextArrayIndex (uint8_t contextId) {
	uint8_t ret = 0xff;
	
	for (uint8_t i = 0; i < BG96_PDP_CONTEXTS; i++) {
		if (pdpContexts[i].contextId == contextId) {
			ret = i;
		}
	}
	
	return ret;
}

uint8_t getSslContextArrayIndex (uint8_t contextId) {
	uint8_t ret = 0xff;
	
	for (uint8_t i = 0; i < BG96_SSL_CONTEXTS; i++) {
		if (sslContexts[i].contextId == contextId) {
			ret = i;
		}
	}
	
	return ret;
}

uint8_t getSocketArrayIndex (uint8_t socketId) {
	uint8_t ret = 0xff;
	
	for (uint8_t i = 0; i < BG96_TCP_SOCKETS; i++) {
		if (tcpSockets[i].connectionId == socketId) {
			ret = i;
		}
	}
	
	return ret;
}

void readSerialPort (void) {
	static uint8_t aux;
	static uint8_t buffAux[20];
	static uint32_t socketDataReadLength;	
	uint8_t data;
	

	if(uartRB_getPendingBytes(&uartRB) > 0) {
		
		data = uartRB_readByte(&uartRB);
		serialCounter ++;
		
		switch (serialPort_state) {
			case serialPort_idle:
				if (data == 13 || data == 10) {
					
					// OK
					if (serialCounter == 3 && serialBuffer[0] == 'O' && serialBuffer[1] == 'K') {
						serialFlags1.bits.okReceived = 1;
					}
					
					serialCounter  = 0;
				}
				else  {
					if (serialCounter <= 10)
						serialBuffer[serialCounter - 1] = data;
					
					if (serialCounter == 1) {
						if (data == '@')
							serialFlags2.bits.keepAliveReceived = 1;
						else if (data == '+')
							serialFlags2.bits.plusReceived = 1;
						else if (data == '>')
							serialFlags1.bits.promptReceived = 1;
					}
					
					if (serialCounter == 5) {
						// ERROR
						if (serialBuffer[0] == 'E' && serialBuffer[1] == 'R' && serialBuffer[2] == 'R')
							serialFlags1.bits.errorReceived = 1;
							
						// AT+GSN (IMEI)
						if (serialBuffer[0] == 'A' && serialBuffer[1] == 'T' && serialBuffer[2] == '+' && serialBuffer[3] == 'G' && serialBuffer[4] == 'S')
							serialPort_gotoState(serialPort_receiving_imei, serialPort_receiving_imei_waiting_eol);
							
						//NORMAL POWER DOWN
						if(serialBuffer[0] == 'N' && serialBuffer[1] == 'O' && serialBuffer[2] == 'R' && serialBuffer[3] == 'M') {
							flags.bits.moduleOn = 0;
							flags2.bits.initModule = 1;
						}	
						
						if (serialBuffer[0] == '+' && serialBuffer[1] == 'C') {
							
							// +CME o +CMS
							if (serialBuffer[2] == 'M' && (serialBuffer[3] == 'E' || serialBuffer[3] == 'S'))
								serialFlags1.bits.errorReceived = 1;
								
							// +CSQ
							if (serialBuffer[2] == 'S' && serialBuffer[3] == 'Q')
								serialPort_gotoState(serialPort_csq, serialPort_null);
								
							// +COPS
							if (serialBuffer[2] == 'O' && serialBuffer[3] == 'P')
								serialPort_gotoState(serialPort_cops, serialPort_null);
								
							// +CPIN
							if (serialBuffer[2] == 'P' && serialBuffer[3] == 'I' && serialBuffer[4] == 'N')
								serialPort_gotoState(serialPort_cpin, serialPort_null);
								
							// +CREG
							if (serialBuffer[2] == 'R' && serialBuffer[3] == 'E' && serialBuffer[4] == 'G')
								serialPort_gotoState(serialPort_creg, serialPort_null);
								
							// +CFUN
							if (serialBuffer[2] == 'F' && serialBuffer[3] == 'U' && serialBuffer[4] == 'N')
								serialPort_gotoState(serialPort_cfun, serialPort_null);
						}
							
						if (serialBuffer[0] == '+' && serialBuffer[1] == 'Q') {
							
							// +QNWINFO
							if (serialBuffer[2] == 'N' && serialBuffer[3] == 'W')
								serialPort_gotoState(serialPort_qnwinfo, serialPort_qnwinfo_waitingQuote);
								
							// +QINISTAT
							if (serialBuffer[2] == 'I' && serialBuffer[3] == 'N')
								serialPort_gotoState(serialPort_qinistat, serialPort_null);
								
							// +QIACT
							if (serialBuffer[2] == 'I' && serialBuffer[3] == 'A' && serialBuffer[4] == 'C')
								serialPort_gotoState(serialPort_qiact, serialPort_qiact_waitingQuote);
								
							// +QIACT
							if (serialBuffer[2] == 'I' && serialBuffer[3] == 'A' && serialBuffer[4] == 'C')
								serialPort_gotoState(serialPort_qiact, serialPort_qiact_waitingQuote);
							
							// +QIOPEN	
							if(serialBuffer[2] == 'I' && serialBuffer[3] == 'O' && serialBuffer[4]== 'P')
								serialPort_gotoState(serialPort_qiopen, serialPort_null);
								
							// +QIURC
							if(serialBuffer[2] == 'I' && serialBuffer[3] == 'U' && serialBuffer[4]== 'R')
								serialPort_gotoState(serialPort_qiurc, serialPort_qiurc_waitingQuote);
								
							// +QIRD
							if(serialBuffer[2] == 'I' && serialBuffer[3] == 'R' && serialBuffer[4]== 'D') {
								socketDataReadLength = 0;
								serialPort_gotoState(serialPort_qird, serialPort_qird_receivingReadLength);
							}
							
						}	
						
					}
					else if (serialCounter == 6) {
						//SEND OK
						if(serialBuffer[0] == 'S' && serialBuffer[1] == 'E' && serialBuffer[2] == 'N' && serialBuffer[3] == 'D' && serialBuffer[4] == ' ' && serialBuffer[5] == 'O')
							serialFlags1.bits.tcpSendOk = 1;
							
						if (serialBuffer[0] == '+' && serialBuffer[1] == 'Q' && serialBuffer[2] == 'S' && serialBuffer[3] == 'S' && serialBuffer[4] == 'L') {
							// +QSSLOPEN
							if(serialBuffer[5]== 'O')
								serialPort_gotoState(serialPort_qsslopen, serialPort_null);
								
							// +QSSLURC
							if(serialBuffer[5]== 'U')
								serialPort_gotoState(serialPort_qsslurc, serialPort_qsslurc_waitingQuote);
								
							// +QSSLRECV
							if(serialBuffer[5]== 'R') {
								socketDataReadLength = 0;
								serialPort_gotoState(serialPort_qsslrecv, serialPort_qsslrecv_receivingReadLength);
							}
						}
						
						if (serialBuffer[0] == '+' && serialBuffer[1] == 'Q' && serialBuffer[2] == 'M' && serialBuffer[3] == 'T') {
							// +QMTOPEN
							if (serialBuffer[4] == 'O' && serialBuffer[5] == 'P')
								serialPort_gotoState(serialPort_mqttOpen, serialPort_null);
								
							// +QMTCLOSE
							if (serialBuffer[4] == 'C' && serialBuffer[5] == 'L')
								serialPort_gotoState(serialPort_mqttClose, serialPort_null);
								
							// +QMTCONN
							if (serialBuffer[4] == 'C' && serialBuffer[5] == 'O')
								serialPort_gotoState(serialPort_mqttConn, serialPort_null);
								
							// +QMTDISC
							if (serialBuffer[4] == 'D' && serialBuffer[5] == 'I')
								serialPort_gotoState(serialPort_mqttDisc, serialPort_null);
								
							// +QMTSTAT
							if (serialBuffer[4] == 'S' && serialBuffer[5] == 'T') {
								// TODO tener un comportamiento diferente dependiendo de qué código de error venga en el mensaje
								if (mqttCallback != NULL) {
									mqttErrorPayload.error = mqtt_error_connection_error;
									mqttCallback(bg96_mqtt_error, (void*)&mqttErrorPayload);
								}
							}
							
							// +QMTPUB
							if (serialBuffer[4] == 'P' && serialBuffer[5] == 'U')
								serialPort_gotoState(serialPort_mqttPub, serialPort_null);
								
							// +QMTSUB
							if (serialBuffer[4] == 'S' && serialBuffer[5] == 'U')
								serialPort_gotoState(serialPort_mqttSub, serialPort_null);
								
							// +QMTRECV
							if (serialBuffer[4] == 'R' && serialBuffer[5] == 'E')
								serialPort_gotoState(serialPort_mqttRec, serialPort_mqttRec_waitingQuote);
						}
						
					}
				}
				

				
				break;
				
			case serialPort_receiving_imei:
			
				switch (serialPort_substate) {
					case serialPort_receiving_imei_waiting_eol:
						if (data == '\n') {
							serialCounter = 0;
							serialPort_gotoSubstate(serialPort_receiving_imei_receiving);
						}
						break;
						
					case serialPort_receiving_imei_receiving:
						if (data != 10 && data != 13) {
							imei[serialCounter - 1] = data;
							
							if (serialCounter >= 15) {
								imei[15] = 0xFF;
								flags.bits.imeiReceived = 1;
								
								serialPort_gotoState(serialPort_idle, serialPort_null);
							}
						}
						else
							serialPort_gotoState(serialPort_idle, serialPort_null);
						break;
				}
				break;
				
				
			case serialPort_csq:
				/*   
					+CSQ: 13,99                
                    ^     ^^   
                    12345678 
				*/ 
				if (serialCounter == 7) {
					signal = data - '0';
				}
				else if (serialCounter == 8) {
					if (data != ',') {
						signal *= 10;
						signal = signal + (data - '0');
					}
					
					serialPort_gotoState(serialPort_idle, serialPort_null);
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				
				break;
				
			case serialPort_cops:
				/*
					+COPS: 0,0,“CHN-UNICOM”,0
					^           ^
					1           13
				*/
				if (serialCounter >= 13 && serialCounter <= 28) {
					if (data == '"') {
						
						if (buffAux[0] == 'M' && buffAux[1] == 'O')
							phoneCarrier = BG96_PHONE_CARRIER_MOVISTAR;
						else if (buffAux[0] == 'C' && buffAux[1] == 'L')
							phoneCarrier = BG96_PHONE_CARRIER_CLARO;
						else if (buffAux[0] == 'P' && buffAux[1] == 'E')
							phoneCarrier = BG96_PHONE_CARRIER_PERSONAL;
						else
							phoneCarrier = BG96_PHONE_CARRIER_NONE;
						
						serialPort_gotoState(serialPort_idle, serialPort_null);
					}
					else
						buffAux[serialCounter - 13] = data;
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				
				break;
				
			case serialPort_cpin:
				/*
					+CPIN: READY
					+CPIN: SIM PIN
					^      ^
					1      8
				*/
				if (serialCounter == 8) {
					if (data == 'R')
						flags.bits.simAvailable = 1;
					else if (data == 'S')
						flags.bits.simWithPin = 1;
					else {
						if(flags.bits.simAvailable) {
							flags.bits.moduleOn = 0;
							flags2.bits.initModule = 1;
							//estado = 0
						}
						
						flags.bits.simAvailable = 0;
					}
					
					serialPort_gotoState(serialPort_idle, serialPort_null);
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
					
				break;
				
			case serialPort_creg:
				/*
					Si yo mandé CREG (serialFlags1.bits.sentCREG == 1)
					
				    +CREG: 1,1
                    ^        ^   
                    12345678 10 
					
                    +CREG: 2,1,"119D","62B2D03",8                
                    ^        ^   
                    12345678 10  
					  
               // Si yo no mandé CREG (serialFlags1.bits.sentCREG == 0)
			   
					+CREG: 1
                    ^      ^   
                    12345678 
					
				    +CREG: 1,"119D","62B2D03",8                
                    ^      ^   
                    12345678 
				*/
				if ((serialCounter == 8 && serialFlags1.bits.sentCREG == 0) || (serialCounter == 10 && serialFlags1.bits.sentCREG) ) {
					
					if (data == '1') {
						if (flags.bits.registred == 0) {
							// Se registró el módulo de la red celular
							if (moduleCallback != NULL) {
								moduleStateChangePayload.state = module_states_change_registred;
								moduleCallback(bg96_module_stateChange, (void*)&moduleStateChangePayload);
							}
						}
						
						flags.bits.registred = 1;
						flags.bits.roaming = 0;
					}
					else if (data == '5') {
						if (flags.bits.registred == 0) {
							// Se registró el módulo de la red celular
							if (moduleCallback != NULL) {
								moduleStateChangePayload.state = module_states_change_registred;
								moduleCallback(bg96_module_stateChange, (void*)&moduleStateChangePayload);
							}
						}
						
						flags.bits.registred = 1;
						flags.bits.roaming = 1;
					}
					else {
						if (flags.bits.registred == 1) {
							// Se desregistró el módulo de la red celular
							if (moduleCallback != NULL) {
								moduleStateChangePayload.state = module_states_change_unregistred;
								moduleCallback(bg96_module_stateChange, (void*)&moduleStateChangePayload);
							}
						}
						
						flags.bits.registred = 0;
						flags.bits.roaming = 0;
					}
					
					serialPort_gotoState(serialPort_idle, serialPort_null);
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				
				break;
				
			case serialPort_cfun:
				/*
					+CFUN: 1
					^      ^
					1      8
				*/
				if (serialCounter == 8) {
					if (data == '1')
						flags.bits.moduleOn = 1;
					else {
						flags.bits.moduleOn = 0;
						flags2.bits.initModule = 1;
					}
						
					serialPort_gotoState(serialPort_idle, serialPort_null);
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				
				break;
				
			case serialPort_qnwinfo:
				/*
					+QNWINFO: <Act>, <oper>, <band>,   <channel>
					+QNWINFO: "GSM","72234","GSM 850",154
					+QNWINFO: "GSM","72234","GSM 850",130
					+QNWINFO: "EDGE","72234","GSM 850",154
					+QNWINFO: "EDGE","72207","GSM 850",245
					+QNWINFO: "EDGE","722310","GSM 1900",524
					+QNWINFO: "CAT-M1","72207","LTE BAND 4",2000
					                  ^     ^
					                  19    25
				*/
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				else {
					switch(serialPort_substate) {
						case serialPort_qnwinfo_waitingQuote:
							if (data == '"')
								serialPort_gotoSubstate(serialPort_qnwinfo_waitingNetwork);
						
							break;
						
						case serialPort_qnwinfo_waitingNetwork:
							if (data == 'G')
								network = BG96_NETWORK_GSM;
							else if (data == 'E')
								network = BG96_NETWORK_EDGE;
							else if (data == 'C' || data == 'e')
								network = BG96_NETWORK_CATM1;
							else
								network = BG96_NETWORK_NONE;
						
							aux = 0;
							serialPort_gotoSubstate(serialPort_qnwinfo_waiting2Quotes);
						
							break;
						
							case serialPort_qnwinfo_waiting2Quotes:
							if (data == '"') {
								aux ++;
							
								if (aux == 2) {
									aux = 0;
									serialPort_gotoSubstate(serialPort_qnwinfo_waitingPhoneCarrier);
								}
							}
						
							break;
						
						case serialPort_qnwinfo_waitingPhoneCarrier:
							if (data != '"') {
								buffAux[aux] = data;
								aux++;
							}
							else {
								if (buffAux[0] == '7' && buffAux[1] == '2' && buffAux[2] == '2') {
									if (buffAux [3] == '0' && buffAux[4] == '7')
										phoneCarrier = BG96_PHONE_CARRIER_MOVISTAR;
									else if (buffAux [3] == '3' && buffAux[4] == '1' && buffAux[5] == '0')
										phoneCarrier = BG96_PHONE_CARRIER_CLARO;
									else if (buffAux [3] == '3' && buffAux[4] == '4')
										phoneCarrier = BG96_PHONE_CARRIER_PERSONAL;
									else
										phoneCarrier = BG96_PHONE_CARRIER_NONE;
									
								}
								
								serialPort_gotoState(serialPort_idle, serialPort_null);
							}
							break;
					}
				}
				
				break;
				
			case serialPort_qinistat:
				/*
					+QINISTAT: 3
					^          ^
					1          12
				*/
				if (serialCounter == 12) {
					if (data == '3')
						flags.bits.callReady = 1;
						
					serialPort_gotoState(serialPort_idle, serialPort_null);
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);

				break;
				
			case serialPort_qiact:
				/*
					+QIACT: 1,1,1,"10.44.234.14"
					+QIACT: 1,1,1,"255.255.255.255"
					^			   ^             ^
					1              16            30
				*/
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				else {
					switch (serialPort_substate) {
						case serialPort_qiact_waitingQuote:
							if (data == '"') {
								aux = 0;
								serialPort_gotoSubstate(serialPort_qiact_receivingIp);
							}
							
							break;
						
						case serialPort_qiact_receivingIp:
						if (data != '"') {
							publicIp[aux] = data;
							aux ++;
						}
						else {
							if ((publicIp[0] >= '0' && publicIp[0] <= '9') && (publicIp[1] >= '0' && publicIp[1] <= '9')) {
								if (publicIp[2] == '.' || ((publicIp[2] >= '0' && publicIp[2] <= '9') && publicIp[3] == '.'))
								serialFlags1.bits.ipReceived = 1;
							}
							
							serialPort_gotoState(serialPort_idle, serialPort_null);
						}
						
						break;
					}
				}
				
				break;
				
			case serialPort_qiopen:
				/*
					+QIOPEN: 0,0
					+QIOPEN: 0,567
					^     ^  ^ ^
					1     7  1012
				*/
				
				if (serialCounter == 10) {
					// Se guarda el número de connection ID
					aux = data - '0';
				}
				if (serialCounter == 12) {
					if (data == '0') {
						// Conexión exitosa
						tcpSockets[aux].connected = true;
					}
					else {
						// Hubo un error en la conexión
						serialFlags1.bits.errorReceived = 1;
					}
						
					serialPort_gotoState(serialPort_idle, serialPort_null);	
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				break;
				
			case serialPort_qiurc:
				/*
					+QIURC: "recv",0
					+QIURC: "closed",0
					+QIURC: "pdpdeact",1
					^        ^ 
					1        10 
				*/
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				else {
					switch(serialPort_substate){
						case serialPort_qiurc_waitingQuote:
							if (data == '"') {
								aux = 0;
								serialPort_gotoSubstate(serialPort_qiurc_receivingEvent);
							}
							
							break;
						
						case serialPort_qiurc_receivingEvent:
							if (data != '"') {
								buffAux[aux] = data;
								aux++;
							}
							else 
								serialPort_gotoSubstate(serialPort_qiurc_receivingConnectionId);
							
							break;
						
						case serialPort_qiurc_receivingConnectionId:
							if (data >= '0' && data <= '9') {
								// Número de connection ID
								aux = data - '0';
							
								if (aux >= 0 && aux < BG96_TCP_SOCKETS) {
							
									// recv
									if (buffAux[0] == 'r')
										tcpSockets[aux].readData = true;
								
									// pdpdeact
									else if (buffAux[0] == 'p' && buffAux[1] == 'd' && buffAux[2] == 'p') {
										if (!pdpContexts[aux - 1].free) {
											pdpContexts[aux - 1].opened = false;
											
											if (moduleCallback != NULL) {
												contextStateChangePayload.state = module_states_context_closed;
												contextStateChangePayload.contextId = getPdpContextArrayIndex(aux);
												contextStateChangePayload.isSsl = false;
												moduleCallback(bg96_module_contextStateChange, (void*)&contextStateChangePayload);
											}
											
											closeAllSockets();
										}
									}
							
									// closed
									else if (buffAux[0] == 'c' && buffAux[1] == 'l' && buffAux[2] == 'o') {
										serialFlags1.bits.closedReceived = 1;
								
										tcpSockets[aux].connected = false;
									
										bg96_closeSocket(aux);
								
										if (socketCallback != NULL) {
											socketCallback(getSocketArrayIndex(aux), bg96_socket_closed, NULL);
										}
									}
								}
							
								serialPort_gotoState(serialPort_idle, serialPort_null);
							}	
						
							break;
					}
				}
				break;
				
			case serialPort_qird: 
				/*
					+QIRD: <read_actual_length><CR><LF><data>
				
					OK
				*/
				switch(serialPort_substate) {
					case serialPort_qird_receivingReadLength:
						if (data == 10 || data == 13)
							serialPort_gotoSubstate(serialPort_qird_waitingCrLf);
						else if (data >= '0' && data <= '9') {
							socketDataReadLength *= 10;
							socketDataReadLength += (data - '0');
						}
							
						break;
						
					case serialPort_qird_waitingCrLf:
						if (data == 10 || data == 13) {
							aux = 0;
							serialPort_gotoSubstate(serialPort_qird_receivingData);
						}
							
						break;
						
					case serialPort_qird_receivingData:
						if (aux < socketDataReadLength) {
							connectionIdReceiveData->dataReceived[aux] = data;
							aux ++;
						}
						else {
							connectionIdReceiveData->dataReceivedLen = socketDataReadLength;
							
							serialFlags1.bits.dataReceiveCompleted = 1;
							
							serialPort_gotoState(serialPort_idle, serialPort_null);
						}
						break;
				}
				break;
				
				
			case serialPort_qsslopen:
				/*
					+QSSLOPEN: 0,0
					           ^ ^
					1          1214
				*/
				
				if (serialCounter == 12) {
					// Se guarda el número de connection ID
					aux = data - '0';
				}
				if (serialCounter == 14) {
					if (data == '0') {
						// Conexión exitosa
						tcpSockets[aux].connected = true;
					}
					else {
						// Hubo un error en la conexión
						serialFlags1.bits.errorReceived = 1;
					}
						
					serialPort_gotoState(serialPort_idle, serialPort_null);	
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
					
				break;


			case serialPort_qsslurc:
				/*
					+QSSLURC: "recv",<connectID>
					+QSSLURC: "closed",<connectID>
					           ^
					1          12
				*/
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				else {
					switch(serialPort_substate){
						case serialPort_qsslurc_waitingQuote:
							if (data == '"') {
								aux = 0;
								serialPort_gotoSubstate(serialPort_qsslurc_receivingEvent);
							}
							
							break;
						
						case serialPort_qsslurc_receivingEvent:
							if (data != '"') {
								buffAux[aux] = data;
								aux++;
							}
							else 
								serialPort_gotoSubstate(serialPort_qsslurc_receivingConnectionId);
							
							break;
						
						case serialPort_qsslurc_receivingConnectionId:
							if (data >= '0' && data <= '9') {
								// Número de connection ID
								aux = data - '0';
							
								if (aux >= 0 && aux < BG96_TCP_SOCKETS) {
							
									// recv
									if (buffAux[0] == 'r')
										tcpSockets[aux].readData = true;
							
									// closed
									else if (buffAux[0] == 'c' && buffAux[1] == 'l' && buffAux[2] == 'o') {
										serialFlags1.bits.closedReceived = 1;
								
										tcpSockets[aux].connected = false;
									
										bg96_closeSocket(aux);
								
										if (socketCallback != NULL) {
											socketCallback(getSocketArrayIndex(aux), bg96_socket_closed, NULL);
										}
									}
								}
							
								serialPort_gotoState(serialPort_idle, serialPort_null);
							}	
						
							break;
					}
				}
				break;
				
			case serialPort_qsslrecv: 
				/*
					+QSSLRECV: <read_actual_length><CR><LF><data>
				
					OK
				*/
				switch(serialPort_substate) {
					case serialPort_qsslrecv_receivingReadLength:
						if (data == 10 || data == 13)
							serialPort_gotoSubstate(serialPort_qsslrecv_waitingCrLf);
						else if (data >= '0' && data <= '9') {
							socketDataReadLength *= 10;
							socketDataReadLength += (data - '0');
						}
							
						break;
						
					case serialPort_qsslrecv_waitingCrLf:
						if (data == 10 || data == 13) {
							aux = 0;
							serialPort_gotoSubstate(serialPort_qsslrecv_receivingData);
						}
							
						break;
						
					case serialPort_qsslrecv_receivingData:
						if (aux < socketDataReadLength && aux < 100) {
							connectionIdReceiveData->dataReceived[aux] = data;
							aux ++;
						}
						else {
							connectionIdReceiveData->dataReceivedLen = socketDataReadLength;
							
							serialFlags1.bits.dataReceiveCompleted = 1;
							
							serialPort_gotoState(serialPort_idle, serialPort_null);
						}
						break;
				}
				break;
				
			case serialPort_mqttOpen:
				/*
					+QMTOPEN: 0,0
					+QMTOPEN: 0,-1
					+QMTOPEN: 0,1
					^           ^
					1           13
				*/
				
				if (serialCounter == 13) {
					if (data == '0') {
						// Conexión exitosa
						mqttClient.opened = true;
					}
					else {
						// Hubo un error en la conexión
						serialFlags1.bits.errorReceived = 1;
					}
						
					serialPort_gotoState(serialPort_idle, serialPort_null);	
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
					
				break;
				
			case serialPort_mqttClose:
				/*
					+QMTCLOSE: 0,0
					+QMTCLOSE: 0,-1
					^            ^
					1            14
				*/
				
				if (serialCounter == 14) {
					if (data == '0') {
						// Cierre exitoso
						mqttClient.opened = false;
					}
					else {
						// Hubo un error en el cierre
						serialFlags1.bits.errorReceived = 1;
					}
						
					serialPort_gotoState(serialPort_idle, serialPort_null);	
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
					
				break;
				
			case serialPort_mqttConn:
				/*
					+QMTCONN: 0,0,0
					+QMTCONN: 0,1,0
					+QMTCONN: 0,0,1
					+QMTCONN: 0,0,3
					^           ^ ^
					1           1315
				*/
				if (serialCounter == 13) {
					aux = data;
					
					if (data == 2) {
						// No se pudo enviar el mensaje
						serialFlags1.bits.errorReceived = 1;
						serialPort_gotoState(serialPort_idle, serialPort_null);	
					}
				}
				
				if (serialCounter == 15) {
					if ((aux == '0' || aux == '1') && data == '0') {
						// Conexión exitosa
						mqttClient.connected = true;
					}
					else {
						// Hubo un error en la conexión
						serialFlags1.bits.errorReceived = 1;
					}
						
					serialPort_gotoState(serialPort_idle, serialPort_null);	
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
					
				break;
				
			case serialPort_mqttDisc:
				/*
					+QMTDISC: 0,0
					+QMTDISC: 0,-1
					^           ^
					1           13
				*/
				
				if (serialCounter == 13) {
					if (data == '0') {
						// Desconexión del broker exitoso
						mqttClient.connected = false;
					}
					else {
						// Hubo un error en la desconexión
						serialFlags1.bits.errorReceived = 1;
					}
						
					serialPort_gotoState(serialPort_idle, serialPort_null);	
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
					
				break;
				
			case serialPort_mqttPub:
				/*
					+QMTPUB: 0,1,0
					+QMTPUB: 0,1,1,2
					+QMTPUB: 0,1,2
					^            ^
					1            14
				*/
				if (serialCounter == 14) {
					if (data == '0' || data == '1') {
						// Publicación exitosa
						serialFlags2.bits.mqttPublishOk = 1;
					}
					else {
						// Hubo un error en la publicación
						serialFlags1.bits.errorReceived = 1;
					}
					
					serialPort_gotoState(serialPort_idle, serialPort_null);
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				
				break;
				
			case serialPort_mqttSub:
				/*
					+QMTSUB: 0,1,0,2
					+QMTPUB: 0,1,1,2
					+QMTPUB: 0,1,2
					^            ^
					1            14
				*/
				if (serialCounter == 14) {
					if (data == '0' || data == '1') {
						// Suscripción exitosa
						serialFlags2.bits.mqttSubscribeOk = 1;
					}
					else {
						// Hubo un error en la suscripción
						serialFlags1.bits.errorReceived = 1;
					}
					
					serialPort_gotoState(serialPort_idle, serialPort_null);
				}
				
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				break;
				
			case serialPort_mqttRec:
				if (data == 10 || data == 13)
					serialPort_gotoState(serialPort_idle, serialPort_null);
				else {
					switch(serialPort_substate){
						case serialPort_mqttRec_waitingQuote:
							if (data == '\"') {
								aux = 0;
								serialPort_gotoSubstate(serialPort_mqttRec_receivingTopic);
							}
								
							break;
							
						case serialPort_mqttRec_receivingTopic:
							if (data != '\"') {
								if (aux < 20) {
									mqttClient.topicReceived[aux] = data;
									aux ++;
								}
							}
							else {
								if (aux < 20)
									mqttClient.topicReceived[aux] = '\0';
									
								serialPort_gotoSubstate(serialPort_mqttRec_waitingQuote2);
							}
							break;
							
						case serialPort_mqttRec_waitingQuote2:
							if (data == '\"') {
								aux = 0;
								serialPort_gotoSubstate(serialPort_mqttRec_receivingMsg);
							}
							
							break;
							
						case serialPort_mqttRec_receivingMsg:
							if (data != '\"') {
								if (aux < 100) {
									mqttClient.msgReceived[aux] = data;
									aux ++;
								}
							}
							else {
								mqttClient.msgReceivedLen = aux;
								
								if (mqttCallback) {
									mqttMessagePayload.topic = mqttClient.topicReceived;
									mqttMessagePayload.data = mqttClient.msgReceived;
									mqttMessagePayload.length = mqttClient.msgReceivedLen;
									
									mqttCallback(bg96_mqtt_msgReceived, (void*)&mqttMessagePayload);
								}
								
								serialPort_gotoState(serialPort_idle, serialPort_null);
							}
							break;
					}
				}
				break;
		}
	}
}


void serialPort_gotoState (serialPort_state_t nextState, serialPort_substate_t nextSubstate)
{
	serialPort_previousState = serialPort_state;
	serialPort_state = nextState;
	
	serialPort_previousSubstate = nextSubstate;
	serialPort_substate = nextSubstate;
}


void serialPort_gotoSubstate (serialPort_substate_t nextSubstate)
{
	serialPort_previousSubstate = serialPort_substate;
	serialPort_substate = nextSubstate;
}


void closeAllSockets (void) {
	for (int i = 0; i < BG96_TCP_SOCKETS; i++) {
		if (tcpSockets[i].connected) {
			if (socketCallback != NULL) {
				socketCallback(i, bg96_socket_closed, NULL);
			}
		}
		
		tcpSockets[i].connected = false;
	}
}


void resetModuleData (void) {
	flags.bits.registred = 0;
			
	for (uint32_t i = 0; i < BG96_PDP_CONTEXTS; i++)
		pdpContexts[i].opened = 0;
		
	for (uint32_t i = 0; i < BG96_SSL_CONTEXTS; i++)
		sslContexts[i].opened = 0;	
		
	for (uint32_t i = 0; i < BG96_TCP_SOCKETS; i++)
		tcpSockets[i].connected = 0;
		
	mqttClient.opened = false;
	mqttClient.connected = false;
	mqttClient.configured = false;
}


void module_gotoState (module_state_t nextState, module_substate_t nextSubstate)
{
	module_previousState = module_state;
	module_state = nextState;
	
	module_previousSubstate = nextSubstate;
	module_substate = nextSubstate;
}


void module_gotoSubstate (module_substate_t nextSubstate)
{
	module_previousSubstate = module_substate;
	module_substate = nextSubstate;
	
	serialFlags1.byte = 0;
	serialFlags2.byte = 0;
}


void module_gotoPrevSubState (void) {
	module_substate_t aux;
	
	aux = module_substate;
	module_substate = module_previousSubstate;
	module_previousSubstate = aux;
}


void module_gotoSubstateWithTimeOut (module_substate_t nextSubstate, uint32_t timeout) {
	module_gotoSubstate(nextSubstate);
	
	softTimer_init(&timerModuleFsmTimeout, timeout);
}


bool module_waitingOk (void) {
	bool ret = false;
	
	if (softTimer_expired(&timerModuleFsmTimeout) || serialFlags1.bits.errorReceived == 1) {
		handleError();
	}
	else {
		if (serialFlags1.bits.okReceived)
			ret = true;
	}
	
	return ret;
}


bool module_waitingPrompt (void) {
	bool ret = false;
	
	if (softTimer_expired(&timerModuleFsmTimeout) || serialFlags1.bits.errorReceived == 1) {
		handleError();
	}
	else {
		if (serialFlags1.bits.promptReceived)
			ret = true;
	}
	
	return ret;
}