#ifndef PGA_MAP_H_
#define PGA_MAP_H_


/*****************************************************************************/
//  PGA
/*****************************************************************************/
#define PGA_LENGTH				700
#define PGA_LENGTH_MPXH			14
uint8_t pgaData[PGA_LENGTH];


// Campos
#define PGA_NOMBRE_DISP						0
#define PGA_ID_DISPOSITIVO					17
#define PGA_SINCRO_INTERNET					21
#define PGA_CODIGO_REGION					22
#define PGA_RETARDO_P1						23
#define PGA_RETARDO_P2						24
#define PGA_RETARDO_P3						25
#define PGA_RETARDO_P4						26
#define PGA_RETARDO_P5						27
#define PGA_RETARDO_P6						28
#define PGA_RETARDO_P7						29
#define PGA_RETARDO_P8						30

#define PGA_PROG_HORARIA_1_HORA_ENCENDIDO	31
#define PGA_PROG_HORARIA_1_HORA_APAGADO		33
#define PGA_PROG_HORARIA_1_PARTICION		35
#define PGA_PROG_HORARIA_1_NODOS			36
//
#define PGA_PROG_HORARIA_16_HORA_ENCENDIDO	181
#define PGA_PROG_HORARIA_16_HORA_APAGADO	183
#define PGA_PROG_HORARIA_16_PARTICION		185
#define PGA_PROG_HORARIA_16_NODOS			186
//
#define PGA_PROG_HORARIA_LARGO				10

#define PGA_FOTOTIMER_1_HORAS				191
#define PGA_FOTOTIMER_1_PARTICION			192
#define PGA_FOTOTIMER_1_NODOS				193
//
#define PGA_FOTOTIMER_16_HORAS				296
#define PGA_FOTOTIMER_16_PARTICION			297
#define PGA_FOTOTIMER_16_NODOS				298
//
#define PGA_FOTOTIMER_LARGO					7

#define PGA_NOCHE_1_PARTICION				303
#define PGA_NOCHE_1_NODOS					304
//
#define PGA_NOCHE_16_PARTICION				393
#define PGA_NOCHE_16_NODOS					394
//
#define PGA_NOCHE_LARGO						6

#define PGA_SIMULADOR_1_PARTICION			399
#define PGA_SIMULADOR_1_NODOS				400
//
#define PGA_SIMULADOR_16_PARTICION			489
#define PGA_SIMULADOR_16_NODOS				490
//
#define PGA_SIMULADOR_LARGO					6

#define PGA_FLAG							495

#define PGA_SERVER_KEY						496
#define PGA_SERVER_KEY_LARGO				16

#define PGA_MONITOREADA						512
#define PGA_APP								513

#define PGA_BROKER_PORT						514
#define PGA_BROKER_URL						516
#define PGA_BROKER_URL_LARGO				70

#endif