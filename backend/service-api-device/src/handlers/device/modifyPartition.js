
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import modifyPartitionSchema from '../../schemas/modifyPartitionSchema';
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


async function modifyPartition(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, numero, nombre, retardoDisparo} = event.body;

    let params = {
        "particiones.$.nombre": nombre,
        "particiones.$.retardoDisparo": retardoDisparo
    };

    for (let prop in params) {
        if (!params[prop])
            delete params[prop];
    }

    try {
        let response = await db.collection("devices")
            .updateOne(
                {comId: comId, "particiones.numero": numero},
                {$set: params});

        if (response.modifiedCount === 0)
            return httpStatus(409, {error: "Error al modificar la partición"})
    }
    catch (error) {
        console.log("[Atlas] " + error);
        return httpStatus(500, error);
    }

    return httpStatus(200, {});
}

export const handler = commonMiddleware(modifyPartition)
    .use(validator({
        inputSchema: modifyPartitionSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))