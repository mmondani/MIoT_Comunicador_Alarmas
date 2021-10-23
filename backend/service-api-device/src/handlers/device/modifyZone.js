
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import modifyZoneSchema from '../../schemas/modifyZoneSchema';
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


async function modifyZone(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, particion, numero, nombre, icono} = event.body;

    let params = {
        "particiones.$[p].zonas.$[z].nombre": nombre,
        "particiones.$[p].zonas.$[z].icono": icono
    };

    for (let prop in params) {
        if (!params[prop])
            delete params[prop];
    }

    try {
        let response = await db.collection("devices")
            .updateOne(
                {comId: comId},
                {$set: params},
                {arrayFilters: [{"p.numero": particion}, {"z.numero": numero}]});

        if (response.modifiedCount === 0)
            return httpStatus(409, {error: `Error al modificar la zona ${numero} de la partición ${particion} del comunicador ${comId}`});
    }
    catch (error) {
        console.log("[Atlas] " + error);
        return httpStatus(500, error);
    }

    return httpStatus(200, {});
}

export const handler = commonMiddleware(modifyZone)
    .use(validator({
        inputSchema: modifyZoneSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))