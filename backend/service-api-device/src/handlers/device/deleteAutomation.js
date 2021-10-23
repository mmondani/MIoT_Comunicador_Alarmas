
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import deleteAutomationSchema from '../../schemas/deleteAutomationSchema';
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


async function deleteAutomation(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, particion, numero, tipo} = event.body;

    try {
        let response = await db.collection("devices")
            .updateOne(
                {comId: comId},
                {$pull:{"particiones.$[p].automatizaciones":{numero: numero, tipo: tipo}}},
                {arrayFilters: [{"p.numero": particion}]}
            );

        if (response.modifiedCount === 0)
            return httpStatus(409, {error: `Error al eliminar la automatización de la partición ${particion} del comunicador ${comId}`})
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