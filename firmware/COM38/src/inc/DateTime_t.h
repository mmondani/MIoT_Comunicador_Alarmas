#ifndef DATETIME_T_H_
#define DATETIME_T_H_


typedef struct {
	uint8_t dia, mes, anio;					// Anio: años que pasaron desde el 2000
	uint8_t horas, minutos, segundos;
}dateTime_t;


#endif /* DATETIME_T_H_ */