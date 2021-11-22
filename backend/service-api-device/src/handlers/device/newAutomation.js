
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import {IotData} from 'aws-sdk';
import newAutomationSchema from '../../schemas/newAutomationSchema';
import {httpStatus} from '../../lib/httpStatus';
import {BrokerMessage} from '../../lib/brokerMessage';
import {BrokerCommands} from '../../lib/brokerCommands';
import {BrokerRegisters} from '../../lib/brokerRegisters';


const iotdata = new IotData({endpoint: process.env.IOT_ENDPOINT});
const MongoClient = require("mongodb").MongoClient;

// Se define la conexión a la base de datos por fuera del handler para que pueda ser reusada
// por llamados sucesivos
let cachedDb = null;


async function connectToDatabase() {
    if (cachedDb) {
        return cachedDb;
    }

    const client = await MongoClient.connect(process.env.MONGODB_CONNECTION_STRING);
    const db = await client.db(process.env.MONGODB_DB_NAME);
    
    cachedDb = db;

    return db;
}


async function newAutomation(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, particion, numero, nombre, tipo, horaInicio, horaFin, horas, nodos} = event.body;

    let automation = {
        numero: numero,
        nombre: nombre,
        tipo: tipo,
        horaInicio: horaInicio,
        horaFin: horaFin,
        horas: horas,
        nodos: nodos
    }

    //console.log(JSON.stringify(automation));

    let msg = new BrokerMessage(
        BrokerCommands.SET,
        BrokerRegisters.CONFIGURACIONES_NODOS,
        parseInt(particion)-1
    );

    try {
        msg.addByte(numero);
        
        switch(tipo) {
            case "fototimer":
                msg.addByte(2);
                msg.addByte(particion);
                msg.addByte(horas);

                break;

            case "programacion_horaria":
                let horaInicioParsed = horaInicio.split(":");
                let horaFinParsed = horaFin.split(":");

                msg.addByte(1);
                msg.addByte(particion);
                msg.addByte(parseInt(horaInicioParsed[0]));
                msg.addByte(parseInt(horaInicioParsed[1]));
                msg.addByte(parseInt(horaFinParsed[0]));
                msg.addByte(parseInt(horaFinParsed[1]));

                break;

            case "noche":
                msg.addByte(3);
                msg.addByte(particion);

                break;

            case "simulador":
                msg.addByte(4);
                msg.addByte(particion);

                break;
        }

        for (let i = 0; i < 5; i ++) {
            if (nodos[i])
                msg.addByte(nodos[i]);
            else
                msg.addByte(0xff);
        }

        let msgBuffer = msg.getBufferToSend();

        let params = {
            topic: comId + "/cmd",
            payload: msgBuffer.toString("hex"),
            qos: 0
        };
        
    
        await iotdata.publish(params).promise();
        

        await db.collection("devices").updateOne(
            {comId: comId, "particiones.numero": particion},
            {$push:{"particiones.$.automatizaciones": {numero: automation.numero, nombre: automation.nombre, tipo: automation.tipo}}}
        );
    }
    catch (error) {
        console.log(error);
        return httpStatus(500, error);
    }

    return httpStatus(201, automation);
}

export const handler = commonMiddleware(newAutomation)
    .use(validator({
        inputSchema: newAutomationSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))