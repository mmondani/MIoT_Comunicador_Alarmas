#ifndef MPXHTIMEDATE_H_
#define MPXHTIMEDATE_H_

#include <asf.h>

#define MPXH_DATE_DIA2      0
#define MPXH_DATE_DIA1      1
#define MPXH_DATE_MES2      2
#define MPXH_DATE_MES1      3
#define MPXH_DATE_ANIO4     4
#define MPXH_DATE_ANIO3     5
#define MPXH_DATE_ANIO2     6
#define MPXH_DATE_ANIO1     7
#define MPXH_TIME_HORA2     0
#define MPXH_TIME_HORA1     1
#define MPXH_TIME_MIN2      2
#define MPXH_TIME_MIN1      3
#define MPXH_TIME_SEG2      4
#define MPXH_TIME_SEG1      5

typedef struct {
	uint8_t d[8];		// Formato: d d m m a a a a
	uint8_t t[6];		// Formato: h h m m s s
}mpxhTimeDate_t;



void mpxhTimeDate_init (void);
void mpxhTimeDate_resetTimeDate (void);
void mpxhTimeDate_resetTime (void);
void mpxhTimeDate_newNibble (uint8_t nibble);
void mpxhTimeDate_vinoMinutoPatron (void) ;
void mpxhTimeDate_setTime (mpxhTimeDate_t *newTime);
void mpxhTimeDate_setDate (mpxhTimeDate_t *newDate);
void mpxhTimeDate_setTimeDate (mpxhTimeDate_t *newTimeDate);
void mpxhTimeDate_getTimeDate (mpxhTimeDate_t *timeDate);
void mpxhTimeDate_handler (void) ;
void mpxhTimeDate_empezarSacarFyh (void);
void mpxhTimeDate_detenerSacarFyh (void);
bool mpxhTimeDate_hayQueSacarFyh (void);
void mpxhTimeDate_empezarSacarHora (void);
void mpxhTimeDate_sendNextNibble (void);
bool mpxhTimeDate_newTimeDate (void);
uint8_t mpxhTimeDate_getDiasMesConBisiesto (uint8_t mes, uint8_t anio);


#endif /* MPXHTIMEDATE_H_ */