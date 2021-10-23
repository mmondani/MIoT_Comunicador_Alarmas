
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import newAlarmUserSchema from '../../schemas/newAlarmUserSchema';
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


async function newAlarmUser(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, particion, numero, nombre} = event.body;

    let alarmUser = {
        numero: numero,
        nombre: nombre
    }

    try {
        let response = await db.collection("devices").updateOne(
            {comId: comId, "particiones.numero": particion},
            {$push:{"particiones.$.usuariosAlarma": alarmUser}}
        );

        if (response.modifiedCount === 0)
            return httpStatus(409, {error: `Error al crear el usuario de la alarma ${numero} en el comunicador ${comId}`})
    }
    catch (error) {
        console.log("[Atlas] " + error);
        return httpStatus(500, error);
    }

    return httpStatus(201, alarmUser);
}

export const handler = commonMiddleware(newAlarmUser)
    .use(validator({
        inputSchema: newAlarmUserSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))