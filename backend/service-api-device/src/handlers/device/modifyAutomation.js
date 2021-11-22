
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import {IotData} from 'aws-sdk';
import modifyAutomationSchema from '../../schemas/modifyAutomationSchema';
import {httpStatus} from '../../lib/httpStatus';
import {BrokerMessage} from '../../lib/brokerMessage';
import {BrokerCommands} from '../../lib/brokerCommands';
import {BrokerRegisters} from '../../lib/brokerRegisters';
import {updateMqtt} from "../../lib/updateMqtt";


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


async function modifyAutomation(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, particion, numero, nombre, tipo, horaInicio, horaFin, horas, nodos} = event.body;

    try {
        if (tipo && nodos) {
            // Solo si viene información del tipo de automatización se va a mandar un mensaje
            // al COM38

            let msg = new BrokerMessage(
                BrokerCommands.SET,
                BrokerRegisters.CONFIGURACIONES_NODOS,
                parseInt(particion)-1
            );

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
        }
        
        if (nombre) {
            // Si viene el nombre, se lo actualiza en la base de datos directamente ya que 
            // no se le informa al COM38

            await db.collection("devices").updateOne(
                {comId: comId},
                {$set: {"particiones.$[p].automatizaciones.$[a].nombre": nombre}},
                {arrayFilters: [{"p.numero": particion}, {"a.numero": numero, "a.tipo": tipo}]}
            );

            // Se avisa por el broker que hay una novedad para este equipo
            await updateMqtt(iotdata, comId, particion - 1);
        }


    }
    catch (error) {
        console.log(error);
        return httpStatus(500, error);
    }   

    return httpStatus(200, {});
}

export const handler = commonMiddleware(modifyAutomation)
    .use(validator({
        inputSchema: modifyAutomationSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))