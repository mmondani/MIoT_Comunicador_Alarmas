#ifndef MAINFSM_H_
#define MAINFSM_H_


void mainFsm_init (struct usart_module* uart);
void mainFsm_handler (void);
bool mainFsm_hayErrorCritico (void);
bool mainFsm_estaEnPruebaFabrica (void);
void mainFsm_analizarMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer, uint8_t nbits);


#endif /* MAINFSM_H_ */