
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import {IotData} from 'aws-sdk';
import deleteAutomationSchema from '../../schemas/deleteAutomationSchema';
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


async function deleteAutomation(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, particion, numero, tipo} = event.body;

    try {
        let msg = new BrokerMessage(
            BrokerCommands.RESET,
            BrokerRegisters.CONFIGURACIONES_NODOS,
            parseInt(particion)-1
        );

        msg.addByte(numero);

        switch(tipo) {
            case "fototimer":
                msg.addByte(2);
                break;

            case "programacion_horaria":
                msg.addByte(1);
                break;

            case "noche":
                msg.addByte(3);
                break;

            case "simulador":
                msg.addByte(4);
                break;
        }

        let msgBuffer = msg.getBufferToSend();

        let params = {
            topic: comId + "/cmd",
            payload: msgBuffer.toString("hex"),
            qos: 0
        };
        
    
        await iotdata.publish(params).promise();
    }
    catch (error) {
        console.log("[Atlas] " + error);
        return httpStatus(500, error);
    }

    return httpStatus(200, {});
}

export const handler = commonMiddleware(deleteAutomation)
    .use(validator({
        inputSchema: deleteAutomationSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))