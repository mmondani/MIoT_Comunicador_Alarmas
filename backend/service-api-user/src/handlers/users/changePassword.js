import validator from '@middy/validator';
import changePasswordSchema from '../../schemas/changePasswordSchema';
import commonMiddleware from '../../lib/commonMiddleware';
import {httpStatus} from '../../lib/httpStatus';
import * as passwordUtilities from '../../lib/passwordUtilities';


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


async function changePassword(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const {email, currentPassword, newPassword} = event.body;

    const db = await connectToDatabase();

    
    try {
        let user = await db.collection("users").findOne({email: email});


        // Se verifica que el usuario exista
        if (!user)
            return httpStatus(409, {error: "Email o password incorrecto"})


        // Se verifica que el currentPassword sea el password que actualmente está configurado
        if (!passwordUtilities.isPasswordCorrect(currentPassword, user.password))
            return httpStatus(409, {error: "Email o password incorrecto"})


        // Se actualiza el password
        await db.collection("users")
                .updateOne(
                    {email: email},
                    {$set: {password: passwordUtilities.hashPassword(newPassword)}});

        
        return httpStatus(200, {});
    }
    catch(error) {
        return httpStatus(500, error);
    }
}

export const handler = commonMiddleware(changePassword)
    .use(validator({
        inputSchema: changePasswordSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))