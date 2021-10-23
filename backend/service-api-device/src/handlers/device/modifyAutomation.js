
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import modifyAutomationSchema from '../../schemas/modifyAutomationSchema';
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


async function modifyAutomation(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, particion, numero, nombre, tipo, horaInicio, horaFin, horas, nodos} = event.body;

    let params = {
        "particiones.$[p].automatizaciones.$[a].nombre": nombre,
        "particiones.$[p].automatizaciones.$[a].horaInicio": horaInicio,
        "particiones.$[p].automatizaciones.$[a].horaFin": horaFin,
        "particiones.$[p].automatizaciones.$[a].horas": horas,
        "particiones.$[p].automatizaciones.$[a].nodos": nodos
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
                {arrayFilters: [{"p.numero": particion}, {"a.numero": numero, "a.tipo": tipo}]});

        if (response.modifiedCount === 0)
            return httpStatus(409, {error: `Error al modificar la automatización de la partición ${particion} del comunicador ${comId}`});
    }
    catch (error) {
        console.log("[Atlas] " + error);
        return httpStatus(500, error);
    }

    return httpStatus(200, {});
}

export const handler = commonMiddleware(modifyAutomation)
    .use(validator({
        inputSchema: modifyAutomationSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))