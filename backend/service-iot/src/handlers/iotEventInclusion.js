import {IotData, SNS} from 'aws-sdk';
import { parseHeader, parseRegisterInlusion } from "../lib/parser";
import {addEvent} from "../lib/addEvent";
import {updateMqtt} from "../lib/updateMqtt";
import {sendPushNotifications} from "../lib/sendPushNotifications";

const MongoClient = require("mongodb").MongoClient;
const iotdata = new IotData({endpoint: process.env.IOT_ENDPOINT});
const sns = new SNS({
  region: "sa-east-1",
  apiVersion: "2010-03-31"
});

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

async function iotEventInclusion(event, context) {

  context.callbackWaitsForEmptyEventLoop = false;

  const db = await connectToDatabase();

  let comId = event.clientId;
  let payload = event.payload;
  let payloadBuffer = Buffer.from(payload, "hex");

  let parsedMessage = parseHeader(payloadBuffer);
  let payloadParsed = parseRegisterInlusion(parsedMessage);

  //console.log (`Mensaje recibido de ${comId} con payload ${payload}`);
  //console.log(JSON.stringify(parsedMessage));
  //console.log(JSON.stringify(payloadParsed));

  try {
    // Se actualiza el estado de la partición
    await db.collection("devices").updateOne (
      {comId: comId, "particiones.numero": parsedMessage.layer + 1},
      {$set:{
        "particiones.$.zonasIncluidas": payloadParsed.zonasIncluidas,
        "particiones.$.zonasCondicionales": payloadParsed.zonasCondicionales,
      }}
    );

    // Se avisa por el broker que hay una novedad para este equipo
    await updateMqtt(iotdata, comId, parsedMessage.layer);
  }
  catch (error) {
    console.log("[IOT] " + error);
  }

  
  return 0;
}

export const handler = iotEventInclusion;
