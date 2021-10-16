import commonMiddleware from '../../lib/commonMiddleware';
import {httpStatus, httpStatusWithHtml} from '../../lib/httpStatus';


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


async function verifyEmail(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const {code} = event.pathParameters;

    if (!code) {
        return httpStatus(400, {message: "Falta el campo code"});
    }

    const db = await connectToDatabase();

    try {
        let result = await db.collection("users")
                    .updateOne(
                        {codigoVerificacion: code},
                        {$set: {verificado: true}});

        // No se encontró el código de verificación
        if (result.modifiedCount == 0)
            return httpStatusWithHtml(200, 
                `<!DOCTYPE html>
                <html lang="en">
                <head>
                    <meta charset="UTF-8">
                    <meta http-equiv="X-UA-Compatible" content="IE=edge">
                    <meta name="viewport" content="width=device-width, initial-scale=1.0">
                    <title>Document</title>
                </head>
                <body>
                    <div style="font-family:sans-serif; text-align: center;">
                        <h1>Ups! Ocurrió un error</h1>
                        <p>Ocurrió un error al intentar verificar tu cuenta de correo electrónico</p>
                    </div>
                </body>
                </html>`);
        

        return httpStatusWithHtml(200, 
            `<!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="UTF-8">
                <meta http-equiv="X-UA-Compatible" content="IE=edge">
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <title>Document</title>
            </head>
            <body>
                <div style="font-family:sans-serif; text-align: center;">
                    <h1>Cuenta de correo electrónico verificada!</h1>
                    <p>Ya podés disfrutar todas las funciones de tu comunicador</p>
                </div>
            </body>
            </html>`);
    }
    catch(error) {
        return httpStatus(500, error);
    }
}

export const handler = commonMiddleware(verifyEmail);