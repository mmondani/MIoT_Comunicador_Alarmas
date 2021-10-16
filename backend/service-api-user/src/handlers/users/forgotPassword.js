import {SQS} from 'aws-sdk';
import validator from '@middy/validator';
import commonMiddleware from '../../lib/commonMiddleware';
import {httpStatus} from '../../lib/httpStatus';
import forgotPasswordSchema from '../../schemas/forgotPasswordSchema'
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


async function forgotPassword(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;


    const db = await connectToDatabase();

    const {email} = event.body;

    try {

        // En la base de datos, en el campo forgotPasswordPin se va a guardar el PIN generado
        // y el timestamp en el que se generó
        let forgotPasswordPin = Math.floor((Math.random() * 9999)).toString().padStart(4, "0");
        let forgotPasswordPinAndTimestamp = forgotPasswordPin + "." + Date.now()
        let result = await db.collection("users")
                    .updateOne(
                        {email: email},
                        {$set: {forgotPasswordPin: forgotPasswordPinAndTimestamp}});

        // No se encontró el email
        if (result.modifiedCount == 0)
            return httpStatus(404, {error: "No se encontró la dirección de email"});
        

        // Se envía un e-mail el pin para recuperar el password
        const recoverPasswordTemplate = `
            <div style="font-family:sans-serif;">
                <h2>Solicitaste recuperar tu contraseña:</h2>
                <p>Usuario: ${email}</p>
                <p>PIN: ${forgotPasswordPin}</p>
            </div>`;

        await sqs.sendMessage({
            QueueUrl: process.env.MAIL_QUEUE_URL,
            MessageBody: JSON.stringify({
                subject: "Solicitud de recuperación de contraseña",
                recipient: email,
                body: recoverPasswordTemplate,
                isHtml: true
            })
        }).promise(); 

        return httpStatus(200, {});
    }
    catch(error) {
        return httpStatus(500, error);
    }
}

export const handler = commonMiddleware(forgotPassword)
    .use(validator({
            inputSchema: forgotPasswordSchema,
            ajvOptions: {
            useDefaults: true,
            strict: false,
            },
        }));