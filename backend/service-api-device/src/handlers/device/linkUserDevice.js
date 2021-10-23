
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import linkUserDeviceSchema from '../../schemas/linkUserDeviceSchema';
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


async function linkUserDevice(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();


    const {comId, email, rol} = event.body;

    try {
        let device = await db.collection("devices").findOne({comId: comId});

        if (!device)
            return httpStatus(409, {error: "No existe el comunicador " + comId});

        let user = await db.collection("users").findOne({email: email});

        if (!user)
            return httpStatus(409, {error: "No existe el usuario " + email});


        // Se chequea si el dispositivo ya está vinculado con el usuario
        let alreadyLinked = false;
        device.usuarios.forEach(u => {
            if (u.email === email)
                alreadyLinked = true;
        });

        if (alreadyLinked)
            return httpStatus(409, {error: `El usuario ${email} y el comunicador ${comId} ya están vinculados`});

        // Si no están vinculados, se crea el vínculo
        let link = {
            email: email,
            rol: rol
        }

        await db.collection("devices").updateOne(
            {comId: comId},
            {$push:{usuarios: link}});

        await db.collection("users").updateOne(
            {email: email},
            {$push:{comunicadores: comId}});

    }
    catch (error) {
        console.log("[Atlas] " + error);

        return httpStatus(500, error);
    }

    return httpStatus(200, {});
}

export const handler = commonMiddleware(linkUserDevice)
    .use(validator({
        inputSchema: linkUserDeviceSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))