import {SQS} from 'aws-sdk';
import validator from '@middy/validator';
import commonMiddleware from '../../lib/commonMiddleware';
import {httpStatus} from '../../lib/httpStatus';
import recoverPasswordSchema from '../../schemas/recoverPasswordSchema'
import * as passwordUtilities from "../../lib/passwordUtilities";

const sqs = new SQS();
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


async function recoverPassword(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;


    const db = await connectToDatabase();

    const {email, password, pin} = event.body;

    try {
        let user = await db.collection("users").findOne({email: email});

        // Se verifica que el usuario exista
        if (!user)
            return httpStatus(409, {error: "Email o pin incorrecto"});


        // Se verifica que el pin sea el correcto y que no haya pasado más de 1 hora
        let userPin = user.forgotPasswordPin.split(".")[0];
        let pinTimestamp = user.forgotPasswordPin.split(".")[1];
        let currentTimestamp = Date.now();

        if (userPin != pin)
            return httpStatus(409, {error: "Email o pin incorrecto"});

        if ((currentTimestamp - pinTimestamp) >= 1000 * 60 * 60)
            return httpStatus(409, {error: "Email o pin incorrecto"});


        // Se actualiza el password y se elimina el pin
        await db.collection("users")
                .updateOne(
                    {email: email},
                    {$set: {
                        password: passwordUtilities.hashPassword(password),
                        forgotPasswordPin: "0000.000000000"
                    }});

        
        return httpStatus(200, {});
    }
    catch(error) {
        return httpStatus(500, error);
    }
}

export const handler = commonMiddleware(recoverPassword)
    .use(validator({
            inputSchema: recoverPasswordSchema,
            ajvOptions: {
            useDefaults: true,
            strict: false,
            },
        }));