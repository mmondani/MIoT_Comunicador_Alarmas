
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import deletePartitionSchema from '../../schemas/deletePartitionSchema';
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


async function deletePartition(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, numero} = event.body;

    try {
        let response = await db.collection("devices")
            .updateOne(
                {comId: comId},
                {$pull:{particiones:{numero: numero}}}
            );

        if (response.modifiedCount === 0)
            return httpStatus(409, {error: `Error al eliminar la partición ${numero} del comunicador ${comId}`})
    }
    catch (error) {
        console.log("[Atlas] " + error);
        return httpStatus(500, error);
    }

    return httpStatus(200, {});
}

export const handler = commonMiddleware(deletePartition)
    .use(validator({
        inputSchema: deletePartitionSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))