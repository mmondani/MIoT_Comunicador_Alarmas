import {SQS} from 'aws-sdk';
import {v4 as uuid} from 'uuid'
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import newUserSchema from '../../schemas/newUserSchema';
import {httpStatus} from '../../lib/httpStatus';
const passwordUtilities = require("../../lib/passwordUtilities");

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


async function newUser(event, context) {
  context.callbackWaitsForEmptyEventLoop = false;


  // Se crea el usuario en Atlas
  const db = await connectToDatabase();

  let now = new Date();
  let user = {
    email: event.body.email,
    password: passwordUtilities.hashPassword(event.body.password),
    fechaCreacion: now.toISOString(),
    verificado: false,
    codigoVerificacion: uuid(),
    apps:[],
    comunicadores:[]
  }

  try {
    await db.collection("users").insertOne(user);
  }
  catch (error) {
    console.log("[Atlas] " + error);

    if (error.code === 11000) {
      // Ya existe el mail
      return httpStatus(409, {error: "Ya se registró esa dirección de e-mail"});
    }
    else {
      return httpStatus(500, error);
    }

  }

  // Se envía un email de verificación de la cuenta
  try {
    const verifyEmailTemplate = `
      <div style="font-family:sans-serif; text-align: center;">
        <h1>Muchas gracias por confiar en nosotros</h1>
        <p>Por favor, hacé click en el siguiente botón para completar el proceso de verificación de tu cuenta de correo electrónico.</p>
        <a href="${process.env.API_BASE_URL + "/user/verify-email/" + user.codigoVerificacion}">
            <button style="margin-top: 100px;background-color: #034da2;border: none; color: white; padding: 15px 32px; text-align: center; text-decoration: none; display: inline; font-size: 16px;">
                Confirmá tu correo
            </button>
        </a>
      </div>`;

    await sqs.sendMessage({
      QueueUrl: process.env.MAIL_QUEUE_URL,
      MessageBody: JSON.stringify({
        subject: "Verificá tu cuenta de e-mail",
        recipient: user.email,
        body: verifyEmailTemplate,
        isHtml: true
      })
    }).promise();
  }
  catch (error) {
    return httpStatus(500, error);
  }

  return httpStatus(201, {email: user.email, fechaCreacion: user.fechaCreacion});
}

export const handler = commonMiddleware(newUser)
  .use(validator({
    inputSchema: newUserSchema,
    ajvOptions: {
      useDefaults: true,
      strict: false,
    },
  }))