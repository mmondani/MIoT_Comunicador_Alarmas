#include "inc/mpxhTimeDate.h"
#include "inc/basicDefinitions.h"
#include "inc/mpxh.h"



#define TIME_DATE_DIA2      0
#define TIME_DATE_DIA1      1
#define TIME_DATE_MES2      2
#define TIME_DATE_MES1      3
#define TIME_DATE_ANIO4     4
#define TIME_DATE_ANIO3     5
#define TIME_DATE_ANIO2     6
#define TIME_DATE_ANIO1     7
#define TIME_DATE_HORA2     8
#define TIME_DATE_HORA1     9
#define TIME_DATE_MIN2      10
#define TIME_DATE_MIN1      11
#define TIME_DATE_SEG2      12
#define TIME_DATE_SEG1      13


uint8_t TABLA_DIAS_MES[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
uint8_t timeAndDate[14];
uint8_t timeAndDate_mirror[14];
uint8_t ptrTimeDateIn  = 0;
uint8_t ptrTimeDateOut = 0;
uint8_t ptrSet = 0;
uint8_t timer_250;
bool newMpxhTimeDate;
bool mandarBeginHora;
bool mandarBeginFechaHora;



void mpxhTimeDate_init (void)
{
	timeAndDate[TIME_DATE_DIA2] = 2;
	timeAndDate[TIME_DATE_DIA1] = 5;
	timeAndDate[TIME_DATE_MES2] = 1;
	timeAndDate[TIME_DATE_MES1] = 0;
	timeAndDate[TIME_DATE_ANIO4] = 2;
	timeAndDate[TIME_DATE_ANIO3] = 0;
	timeAndDate[TIME_DATE_ANIO2] = 1;
	timeAndDate[TIME_DATE_ANIO1] = 9;
	timeAndDate[TIME_DATE_HORA2] = 0;
	timeAndDate[TIME_DATE_HORA1] = 0;
	timeAndDate[TIME_DATE_MIN2] = 0;
	timeAndDate[TIME_DATE_MIN1] = 0;
	timeAndDate[TIME_DATE_SEG2] = 0;
	timeAndDate[TIME_DATE_SEG1] = 1;
	
	newMpxhTimeDate = false;
	ptrTimeDateOut = TIME_DATE_SEG1+2;
}
	
	

void mpxhTimeDate_resetTimeDate (void) 
{
	ptrTimeDateIn = TIME_DATE_DIA2;
	ptrTimeDateOut = TIME_DATE_SEG1+2;
	ptrSet = TIME_DATE_DIA2;
}


void mpxhTimeDate_resetTime (void)
{
	ptrTimeDateIn = TIME_DATE_HORA2;
	ptrTimeDateOut = TIME_DATE_SEG1+1;
	ptrSet = TIME_DATE_HORA2;
}


void mpxhTimeDate_newNibble (uint8_t nibble)
{
	// 0Cx - Nibble fecha u hora
	timeAndDate_mirror[ptrTimeDateIn] = nibble_swap(nibble) & 0x0F;

	ptrTimeDateIn++;

	if (ptrTimeDateIn > TIME_DATE_SEG1)
	{
		// Termino de venir la hora y/o la fecha. Se pasan los datos
		for (int i = ptrSet ; i < TIME_DATE_SEG1; i++)
		{
			timeAndDate[i] = timeAndDate_mirror[i];
		}
		
		newMpxhTimeDate = true;
	}
}


void mpxhTimeDate_vinoMinutoPatron (void) 
{
	if (timeAndDate[TIME_DATE_SEG2] >= 3)
	{
		timeAndDate[TIME_DATE_SEG2] = 5;
		timeAndDate[TIME_DATE_SEG1] = 9;
		timer_250 = 250;
	}
	else
	{
		timeAndDate[TIME_DATE_SEG2] = 0;
		timeAndDate[TIME_DATE_SEG1] = 0;
	}
}


void mpxhTimeDate_setTime (mpxhTimeDate_t *newTime)
{
	uint8_t i, j;
	
	mpxhTimeDate_resetTime();
	
	for (i = ptrSet, j = 0; i <= TIME_DATE_SEG1; i++, j++) {
		timeAndDate[i] = newTime->t[j];
	}
}


void mpxhTimeDate_setDate (mpxhTimeDate_t *newDate)
{
	uint8_t i, j;
	
	mpxhTimeDate_resetTimeDate();
	
	for (i = ptrSet, j = 0; i <= TIME_DATE_ANIO1; i++, j++) {
		timeAndDate[i] = newDate->d[j];
	}
}


void mpxhTimeDate_setTimeDate (mpxhTimeDate_t *newTimeDate)
{
	uint8_t i, j;
	
	mpxhTimeDate_resetTimeDate();
	
	for (i = ptrSet, j = 0; i <= TIME_DATE_ANIO1; i++, j++) {
		timeAndDate[i] = newTimeDate->d[j];
		
	}
	
	for (j = 0; i <= TIME_DATE_SEG1; i++, j++) {
		timeAndDate[i] = newTimeDate->t[j];
	}
}


void mpxhTimeDate_getTimeDate (mpxhTimeDate_t *timeDate)
{
	uint8_t i, j;
	
	for (i = 0; i <= TIME_DATE_ANIO1; i++) {
		timeDate->d[i] = timeAndDate[i];
	}
	
	for (j = 0; i <= TIME_DATE_SEG1; i++, j++) {
		timeDate->t[j] = timeAndDate[i];
	}
}


bool mpxhTimeDate_newTimeDate (void)
{
	bool ret = newMpxhTimeDate;
	
	newMpxhTimeDate = false;
	
	return ret;
}


void mpxhTimeDate_empezarSacarHora (void)
{
	ptrTimeDateOut = TIME_DATE_HORA2;
	mandarBeginHora = true;
}


void mpxhTimeDate_empezarSacarFyh (void)
{
	ptrTimeDateOut = TIME_DATE_DIA2;
	mandarBeginFechaHora = true;
}

void mpxhTimeDate_detenerSacarFyh (void)
{
	ptrTimeDateOut = TIME_DATE_SEG1+1;
	
	mandarBeginFechaHora = false;
	mandarBeginHora = false;
}


bool mpxhTimeDate_hayQueSacarFyh (void)
{
	return (ptrTimeDateOut <= TIME_DATE_SEG1 );
}


void mpxhTimeDate_sendNextNibble (void)
{
	uint8_t ret = 0;
	
	if (mpxhTimeDate_hayQueSacarFyh()) {
		if (mpxh_Ocupado() == 0) {
			if (mandarBeginFechaHora) {
				mandarBeginFechaHora = false;
				
				mpxh_ArmaMensaje(0x0a, 0x90, 0, MPXH_BITS_17);
			}
			else if (mandarBeginHora) {
				mandarBeginHora = false;
				
				mpxh_ArmaMensaje(0x0a, 0xa0, 0, MPXH_BITS_17);
			}
			else {
				mpxh_ArmaMensaje(0x0C, nibble_swap(timeAndDate[ptrTimeDateOut]), 0, MPXH_BITS_17);
				ptrTimeDateOut ++;
			}
		}
	}
}


/*
 Debe ser llamada cada 4 msegs.
*/
void mpxhTimeDate_handler (void) 
{
    uint8_t diaAux, mesAux, diasMes, anioAux;


    timer_250++;        // Se incrementa cada 4 msegs


    if (timer_250 == 250)
    {
	    timer_250 = 0;
	    
	    timeAndDate[TIME_DATE_SEG1] ++;

	    if (timeAndDate[TIME_DATE_SEG1] > 9)
	    {
		    timeAndDate[TIME_DATE_SEG1] = 0;
		    timeAndDate[TIME_DATE_SEG2] ++;

		    if (timeAndDate[TIME_DATE_SEG2] > 5)
		    {
			    timeAndDate[TIME_DATE_SEG2] = 0;
			    timeAndDate[TIME_DATE_MIN1] ++;

			    if (timeAndDate[TIME_DATE_MIN1] > 9)
			    {
				    timeAndDate[TIME_DATE_MIN1] = 0;
				    timeAndDate[TIME_DATE_MIN2] ++;

				    if (timeAndDate[TIME_DATE_MIN2] > 5)
				    {
					    timeAndDate[TIME_DATE_MIN2] = 0;
					    timeAndDate[TIME_DATE_HORA1] ++;

					    if ((timeAndDate[TIME_DATE_HORA2] < 2 && timeAndDate[TIME_DATE_HORA1] > 9) ||
					    (timeAndDate[TIME_DATE_HORA2] == 2 && timeAndDate[TIME_DATE_HORA1] > 3))
					    {
						    timeAndDate[TIME_DATE_HORA1] = 0;
						    timeAndDate[TIME_DATE_HORA2] ++;

						    if (timeAndDate[TIME_DATE_HORA2] > 2)
						    {
							    timeAndDate[TIME_DATE_HORA2] = 0;
							    timeAndDate[TIME_DATE_DIA1] ++;

							    if (timeAndDate[TIME_DATE_DIA1] > 9)
							    {
								    timeAndDate[TIME_DATE_DIA1] = 0;
								    timeAndDate[TIME_DATE_DIA2] ++;

								    diaAux = timeAndDate[TIME_DATE_DIA1] | (nibble_swap(timeAndDate[TIME_DATE_DIA2]) & 0xF0);
								    mesAux = timeAndDate[TIME_DATE_MES1] | (nibble_swap(timeAndDate[TIME_DATE_MES2]) & 0xF0);

								    diasMes = TABLA_DIAS_MES[mesAux - 1];

								    if (diasMes == 28)
								    {
									    anioAux = (timeAndDate[TIME_DATE_ANIO1] >> 1) ^ timeAndDate[TIME_DATE_ANIO2];
									    anioAux = anioAux & 0x01;

									    if (anioAux == 0 && (bit_test(timeAndDate[TIME_DATE_ANIO1],0) == 0))
									    {
										    // Es bisiesto
										    diasMes ++;
									    }
								    }

								    if (diaAux > diasMes)
								    {
									    timeAndDate[TIME_DATE_DIA2] = 0;
									    timeAndDate[TIME_DATE_MES1] ++;

									    mesAux ++;

									    if (timeAndDate[TIME_DATE_MES1] > 9)
									    {
										    timeAndDate[TIME_DATE_MES1] = 0;
										    timeAndDate[TIME_DATE_MES2] ++;

										    if (mesAux > 12)
										    {
											    timeAndDate[TIME_DATE_MES2] = 0;
											    timeAndDate[TIME_DATE_ANIO1] ++;

											    if (timeAndDate[TIME_DATE_ANIO1] > 9)
											    {
												    timeAndDate[TIME_DATE_ANIO1] = 0;
												    timeAndDate[TIME_DATE_ANIO2]++;

												    if (timeAndDate[TIME_DATE_ANIO2] > 9)
												    {
													    timeAndDate[TIME_DATE_ANIO2] = 0;
												    }
											    }
										    }
									    }
								    }
							    }
						    }
					    }
				    }
			    }
		    }
	    }
    }	
}


uint8_t mpxhTimeDate_getDiasMesConBisiesto (uint8_t mes, uint8_t anio)
{
	uint8_t dias = 0;
	
	if (mes <= 12) {
		dias = TABLA_DIAS_MES[mes - 1];
		
		if (dias == 28 && (anio % 4) == 0)			// Es bisiesto
			dias ++;
	}	
	
	return dias;
}