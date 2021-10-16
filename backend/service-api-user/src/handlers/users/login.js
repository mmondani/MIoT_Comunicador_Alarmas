import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import loginSchema from '../../schemas/loginSchema';
import {httpStatus} from '../../lib/httpStatus';
const passwordUtilities = require("../../lib/passwordUtilities");
const jwt = require("jsonwebtoken");

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


async function login(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    // Se recupera el usuario de la base de datos para verificar el password
    try {
        let user = await db.collection("users").findOne({email: event.body.email});

        if (!user) {
            return httpStatus(400, {error: "Email o password incorrecto"});
        }
        else {
            if (passwordUtilities.isPasswordCorrect(event.body.password, user.password)) {
                // Lo que se coloque dentro del token va a poder ser aprovechado 
                // una vez que se lo verifique
                let token = jwt.sign({
                    email: user.email,
                    verificado: user.verificado
                }, 
                process.env.JWT_KEY,
                {
                    expiresIn: "1 day"
                });

                return httpStatus(200, {token: token});
            }
            else {
                return httpStatus(400, {error: "Email o password incorrecto"});
            }
        }
    }
    catch(error) {
        return httpStatus(500, error);
    }
}

export const handler = commonMiddleware(login)
    .use(validator({
        inputSchema: loginSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))