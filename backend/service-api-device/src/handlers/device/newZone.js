
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import newZoneSchema from '../../schemas/newZoneSchema';
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


async function newZone(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, particion, numero, nombre, icono} = event.body;

    let zone = {
        numero: numero,
        nombre: nombre,
        icono: icono
    }

    try {
        let response = await db.collection("devices").updateOne(
            {comId: comId, "particiones.numero": particion},
            {$push:{"particiones.$.zonas": zone}}
        );

        if (response.modifiedCount === 0)
            return httpStatus(409, {error: `Error al crear la zona ${numero} en el comunicador ${comId}`})
    }
    catch (error) {
        console.log("[Atlas] " + error);
        return httpStatus(500, error);
    }

    return httpStatus(201, zone);
}

export const handler = commonMiddleware(newZone)
    .use(validator({
        inputSchema: newZoneSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))