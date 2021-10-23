
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import newPartitionSchema from '../../schemas/newPartitionSchema';
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


async function newPartition(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, numero, nombre} = event.body;

    let partition = {
        numero: numero,
        nombre: nombre,
        retardoDisparo: 8,
        estado: "desactivada",
        sonando: false,
        lista: true,
        modo: "estoy",
        zonasAnormales: "00000000000000000000000000000000",
        zonasMemorizadas: "00000000000000000000000000000000",
        zonasIncluidas: "00000000000000000000000000000000",
        zonasCondicionales: "0000",
        tipoDisparo: 0,
        replayDisparo: "",
        zonas: [],
        nodos: [],
        usuariosAlarma: [],
        automatizaciones: [],
        eventosAlarma: []
    }

    try {
        let response = await db.collection("devices").updateOne(
            {comId: comId},
            {$push:{particiones: partition}}
        );

        if (response.modifiedCount === 0)
            return httpStatus(409, {error: "Error al crear la partición en el comunicador " + comId})
    }
    catch (error) {
        console.log("[Atlas] " + error);
        return httpStatus(500, error);
    }

    return httpStatus(201, partition);
}

export const handler = commonMiddleware(newPartition)
    .use(validator({
        inputSchema: newPartitionSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))