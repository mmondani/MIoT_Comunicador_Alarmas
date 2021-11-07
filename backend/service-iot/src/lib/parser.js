export const parseHeader = (message) => {
    let parsedMessage;

    /**
     * Formato del header
     *      comando: 1 byte
     *      registro: 2 bytes
     *      layer: 1 byte
     *      fecha: 3 bytes
     *      hora: 3 bytes
     *      random: 1 byte
     *      largo: 2 bytes
     *      payload: n bytes
     *      checksum: 1 byte
     */
    let sum = 0;
    for (let i = 0; i < message.length; i ++) {
        sum += message[i];
    }

    if (sum & 0xff === 0xff) {
        // Checksum correcto
        parsedMessage = {}
        parsedMessage.comando = message[0];
        parsedMessage.registro = ((message[2] << 8) & 0xff00) | message[1];
        parsedMessage.layer = message[3];
        parsedMessage.timestamp = new Date(message[6] + 2000, message[5], message[4], message[7], message[8], message[9]).getTime();
        parsedMessage.random = message[10];
        parsedMessage.largo = ((message[12] << 8) & 0xff) | message[11];
        parsedMessage.payload = [];
        for (let i = 0; i < parsedMessage.largo; i++) {
            parsedMessage.payload.push(message[13 + i]);
        }

        return parsedMessage;
    }
    else {
        // Error en el checksum
        return parsedMessage;
    }
}


let EstadosAlarma = ["null", "desactivada", "activada", "activada_estoy", "activada_me_voy", "activacion_parcial", "programacion"];
let EstadosBateria = ["bien", "dudosa", "baja"];

export const parsePedirFyH = (message) => {
    let payloadParsed = {};

    payloadParsed.cod_region = message.payload[0];
    
    return payloadParsed;
}

export const parseRegisterEstado = (message) => {
    let payloadParsed = {};

    if (message.comando == 0x0a || message.comando == 0x0b) {
        payloadParsed.estado = EstadosAlarma[message.payload[0]];
        payloadParsed.estadoRedElectrica = (message.payload[1] === 0)? false : true;
        payloadParsed.estadoBateria = EstadosBateria[message.payload[2]];
        payloadParsed.estadoMpxh = (message.payload[3] === 0)? false : true;
        payloadParsed.particion = (message.payload[4] === 0)? false : true;
        payloadParsed.sonando = (message.payload[5] === 0)? false : true;
        payloadParsed.ready = (message.payload[6] === 0)? false : true;
        payloadParsed.cantidadZonas = message.payload[7];
        payloadParsed.versionFirmware = message.payload[8].toString() + "." + message.payload[9].toString();

        payloadParsed.zonasAnormales = 
            message.payload[10].toString(2).padStart(8, "0") + 
            message.payload[11].toString(2).padStart(8, "0") + 
            message.payload[12].toString(2).padStart(8, "0") + 
            message.payload[13].toString(2).padStart(8, "0");
        

        payloadParsed.zonasMemorizadas =
            message.payload[14].toString(2).padStart(8, "0") + 
            message.payload[15].toString(2).padStart(8, "0") + 
            message.payload[16].toString(2).padStart(8, "0") + 
            message.payload[17].toString(2).padStart(8, "0");

        payloadParsed.modo = 'ninguno';
        if (message.payload[18] != 0)
            payloadParsed.modo = 'estoy';
        else if (message.payload[19] != 0)
            payloadParsed.modo = 'me_voy';
        
        payloadParsed.zonasIncluidas = 
            message.payload[20].toString(2).padStart(8, "0") + 
            message.payload[21].toString(2).padStart(8, "0") + 
            message.payload[22].toString(2).padStart(8, "0") + 
            message.payload[23].toString(2).padStart(8, "0");

        payloadParsed.zonasCondicionales = message.payload[24].toString(2).padStart(4, "0");
    }
    
    return payloadParsed;
}


export const parseRegisterOpenClose = (message) => {
    let payloadParsed = {};

    if (message.comando == 0x0b) {
        payloadParsed.estado_alarma = EstadosAlarma[message.payload[0]];
        payloadParsed.usuario = message.payload[1];
    }
    
    return payloadParsed;
}


export const parseRegisterRed = (message) => {
    let payloadParsed = {};

    if (message.comando == 0x0b) {
        payloadParsed.estadoRedElectrica = (message.payload[0] === 0)? false : true;
    }
    
    return payloadParsed;
}


export const parseRegisterBateria = (message) => {
    let payloadParsed = {};

    if (message.comando == 0x0b) {
        payloadParsed.estadoBateria = EstadosBateria[message.payload[0]];
    }
    
    return payloadParsed;
}


export const parseRegisterSonandoReady = (message) => {
    let payloadParsed = {};

    if (message.comando == 0x0a || message.comando == 0x0b) {
        payloadParsed.particiones = [];
        for (let i = 0; i < 8; i++) {
            let particion = {};
        
            if ((message.payload[0] & (1 << i)) != 0)
                particion.sonando = true;
            else
                particion.sonando = false;

            if ((message.payload[1] & (1 << i)) != 0)
                particion.lista = true;
            else
                particion.lista = false;

            if ((message.payload[2] & (1 << i)) != 0)
                particion.modo = "estoy";
            else if ((message.payload[3] & (1 << i)) != 0)
                particion.modo = "me_voy";  
            else
                particion.modo = "ninguno";  


            payloadParsed.particiones.push(particion);
        }

        payloadParsed.cantidadZonas = message.payload[4];
    }
    
    return payloadParsed;
}

