
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import modifyDeviceSchema from '../../schemas/modifyDeviceSchema';
import {httpStatus} from '../../lib/httpStatus';


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

    const {comId, nombre, sincronizaHora, codigoRegion, icono, clavem, claveh, celularAsalto, usaApp, monitreada} = event.body;

    let params = {
        nombre,
        sincronizaHora,
        codigoRegion,
        icono,
        clavem,
        claveh,
        celularAsalto,
        usaApp,
        monitreada
    };

    for (let prop in params) {
        if (!params[prop])
            delete params[prop];
    }

    try {
        let response = await db.collection("devices")
            .updateOne(
                {comId: comId},
                {$set: params});

        if (response.modifiedCount === 0)
            return httpStatus(409, {error: "Error al modificar el comunicador " + comId})
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