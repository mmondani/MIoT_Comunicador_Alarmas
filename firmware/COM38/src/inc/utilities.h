#ifndef UTILITIES_H_
#define UTILITIES_H_


#include "asf.h"
#include "basicDefinitions.h"


uint8_t sacarLayer (uint8_t dato);
uint8_t toUpperCase (uint8_t ascii);
uint8_t traducirAsciiATlcd (uint8_t ascii);
uint8_t traducirTlcdAAscii (uint8_t tlcd);
uint8_t traductirHexaATeclado (uint8_t letra);
uint8_t traducirHexaACaracter (uint8_t hexa);
uint8_t traducirCaracterAHexa (uint8_t c);
void convertNumberToHexString (uint8_t* string, uint8_t number);
uint8_t convertHexStringToNumber (uint8_t* string);
void enableWDT (void);
void disableWDT (void);


#endif