
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import {IotData} from 'aws-sdk';
import modifyDeviceSchema from '../../schemas/modifyDeviceSchema';
import {httpStatus} from '../../lib/httpStatus';
import {BrokerMessage} from '../../lib/brokerMessage';
import {BrokerCommands} from '../../lib/brokerCommands';
import {BrokerRegisters} from '../../lib/brokerRegisters';
const passwordUtilities = require("../../lib/passwordUtilities");


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


async function modifyDevice(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, nombre, icono, clavem, claveh, celularAsalto, usaApp, monitreada, sincronizaHora, codigoRegion} = event.body;

    let params = {
        nombre,
        icono,
        clavem,
        claveh,
        celularAsalto,
        usaApp,
        monitreada,
        sincronizaHora,
        codigoRegion
    };

    for (let prop in params) {
        if (!params[prop])
            delete params[prop];
    }

    // Se hashean las claves si están presentes
    if (params.clavem)
        params.clavem = passwordUtilities.hashPassword(params.clavem);

    if (params.claveh)
        params.claveh = passwordUtilities.hashPassword(params.claveh);


    try {
        let response = await db.collection("devices")
            .updateOne(
                {comId: comId},
                {$set: params});

        // Si vienen los parámetros sincronizaHora y codigoRegion,
        // se envía el comando al broker para cambiar el timezone del comunicador
        if (sincronizaHora && codigoRegion) {
            let msg = new BrokerMessage(
                BrokerCommands.SET,
                BrokerRegisters.CONFIGURACION_TIEMPO,
                0
            );

            let sincroniza = (sincronizaHora)? 1 : 0;
            msg.addByte(sincroniza);
            msg.addByte(codigoRegion);
            

            let msgBuffer = msg.getBufferToSend();

            let params = {
                topic: comId + "/cmd",
                payload: msgBuffer.toString("hex"),
                qos: 0
            };

            await iotdata.publish(params).promise();
        }
    }
    catch (error) {
        console.log("[Atlas] " + error);
        return httpStatus(500, error);
    }

    return httpStatus(200, {});
}

export const handler = commonMiddleware(modifyDevice)
    .use(validator({
        inputSchema: modifyDeviceSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))