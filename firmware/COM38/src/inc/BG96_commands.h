#ifndef BG96_COMMANDS_H_
#define BG96_COMMANDS_H_


const char bg96_cpin[] = "AT+CPIN?\r";
const char bg96_cpin1[] = "AT+CPIN=";
const char bg96_qinistat[]="AT+QINISTAT\r";
const char bg96_gsn[] = "AT+GSN\r";
const char bg96_qccid[] = "AT+QCCID\r";
const char bg96_qsclk[] = "AT+QSCLK=1\r";
const char bg96_cscs[] = "AT+CSCS=\"GSM\"\r";
const char bg96_qurccfg[]="AT+QURCCFG=\"URCPORT\",\"uart1\"\r";
const char bg96_qcfg_nwscanmode[] = "AT+QCFG=\"nwscanmode\",";
const char bg96_qcfg_band_all_bg96[] = "AT+QCFG=\"band\",f,400a0e189f,8000008\r";        // TODAS las bandas para 2G y CATM1; B4 y B28 para NB2
const char bg96_qcfg_band_reduced_bg96[] = "AT+QCFG=\"band\",f,8000008,8000008\r";       // TODAS las bandas 2G; sólo banda B4 y B28 para CATM1 y NB2
const char bg96_ifc[] = "AT+IFC=2,2\r";
const char bg96_cfun[] = "AT+CFUN?\r";
const char bg96_cfun0[] = "AT+CFUN=0\r";
const char bg96_cfun1[] = "AT+CFUN=1\r";
const char bg96_qcfg_band_reducido_bg96[] = "AT+QCFG=\"band\",f,8000008,8000008\r";			// TODAS las bandas 2G; sólo banda B4 y B28 para CATM1 y NB2
const char bg96_qcfg_band_todas_bg96[] = "AT+QCFG=\"band\",f,400a0e189f,8000008\r";			// TODAS las bandas para 2G y CATM1; B4 y B28 para NB2
const char bg96_qcfg_nwscanseq[] = "AT+QCFG=\"nwscanseq\",020103\r";						// Preferencia CatM1 -> GSM -> NB
const char bg96_qcfg_iotopmode[] = "AT+QCFG=\"iotopmode\",0\r";								// Usar solo CatM1 y no NB
const char bg96_save[] = "AT&W\r";
const char bg96_ate1[] = "ATE1\r";
const char bg96_creg0[] = "AT+CREG=0\r";
const char bg96_atcreg[] = "AT+CREG?\r";
const char bg96_creg2[] = "AT+CREG=2\r";
const char bg96_qnwinfo[] = "AT+QNWINFO\r";
const char bg96_qpowd[] = "AT+QPOWD=1\r";
const char bg96_cipshut[]="AT+QIDEACT=";
const char bg96_qicsgp[] = "AT+QICSGP=";
const char bg96_qiact[]="AT+QIACT=";
const char bg96_atqiact[]="AT+QIACT?\r";
const char bg96_cipstart[]="AT+QIOPEN=";
const char bg96_tcp[]=",\"TCP\",\"";
const char bg96_qiclose[]="AT+QICLOSE=";
const char bg96_qisend[]="AT+QISEND=";
const char bg96_qird[]="AT+QIRD=";
const char bg96_qsslcfg[]="AT+QSSLCFG=\"cacert\",";
const char bg96_qsslopen[]="AT+QSSLOPEN=";
const char bg96_qsslclose[]="AT+QSSLCLOSE=";
const char bg96_qsslsend[]="AT+QSSLSEND=";
const char bg96_qsslrecv[]="AT+QSSLRECV=";
const char bg96_mqttCfg_pdpcid[]="AT+QMTCFG=\"pdpcid\",0,";
const char bg96_mqttCfg_ssl[]="AT+QMTCFG=\"ssl\",0,";
const char bg96_mqttCfg_keepalive[]="AT+QMTCFG=\"keepalive\",0,";
const char bg96_mqttCfg_session[]="AT+QMTCFG=\"session\",0,";
const char bg96_mqttCfg_will[]="AT+QMTCFG=\"will\",0,1,";
const char bg96_mqttOpen[]="AT+QMTOPEN=0,\"";
const char bg96_mqttConn[]="AT+QMTCONN=0,\"";
const char bg96_mqttClose[]="AT+QMTCLOSE=0\r";
const char bg96_mqttDisc[]="AT+QMTDISC=0\r";
const char bg96_mqttPub[]="AT+QMTPUB=0,";
const char bg96_mqttSub[]="AT+QMTSUB=0,1,\"";
const char bg96_separador[]="\",\"";
const char bg96_separador2[]="\",";
const char bg96_separador3[]=",\"";

#endif