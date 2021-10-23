
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import newAutomationSchema from '../../schemas/newAutomationSchema';
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

    try {
        let response = await db.collection("devices").updateOne(
            {comId: comId, "particiones.numero": particion},
            {$push:{"particiones.$.automatizaciones": automation}}
        );

        if (response.modifiedCount === 0)
            return httpStatus(409, {error: `Error al crear la automatización de la particion ${particion} en el comunicador ${comId}`})
    }
    catch (error) {
        console.log("[Atlas] " + error);
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