#ifndef PGA_H_
#define PGA_H_


#include "PGA_Map.h"


/*****************************************************************************/
//  DEFINICIONES
/*****************************************************************************/
#define ID_DEVICE                 0x90
#define ID_DEVICE_H               0x09
#define ID_DEVICE_L               0x00
#define ID_PGA_15BITS_MPXH        0b00001001

#define PGA_SAVE_QUEUE_LENGTH     50



void pga_init (void);
void pga_checkEeprom (void);
uint8_t pga_dumpByte (void);
void pga_dumpPga (void);
bool pga_hayqueDumpear (void);
void pga_enqueueSave (uint32_t arrayIndex, uint32_t cant, uint32_t addr);
void pga_saveRAM (void);
void pga_resetAll (void);
bool pga_isQueueEmpty (void);

#endif /* PGA_H_ */