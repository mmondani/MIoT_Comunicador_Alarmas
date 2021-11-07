import {IotData, SNS} from 'aws-sdk';
import { parseHeader, parseRegisterBateria } from "../lib/parser";
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

async function iotEventRed(event, context) {

  context.callbackWaitsForEmptyEventLoop = false;

  const db = await connectToDatabase();

  let comId = event.clientId;
  let payload = event.payload;
  let payloadBuffer = Buffer.from(payload, "hex");

  let parsedMessage = parseHeader(payloadBuffer);
  let payloadParsed = parseRegisterBateria(parsedMessage);

  console.log (`Mensaje recibido de ${comId} con payload ${payload}`);
  console.log(JSON.stringify(parsedMessage));
  console.log(JSON.stringify(payloadParsed));

  try {
    // Se actualiza el estado de la partición
    await db.collection("devices").updateOne (
      {comId: comId},
      {$set:{
        estadoBateria: payloadParsed.estadoBateria
      }}
    );

    // Se busca el nombre del comunicador en la base de datos
    // Se lo va a usar para armar la notificación y guardar el evento
    let comInfo = await db.collection("devices").findOne(
      {comId: comId},
      {nombre:1, _id: 0}
    );

    let comName = `Comunicador ${comId}`;
    if (comInfo)
      comName = comInfo.nombre;

    let eventDescription;
    let eventTimeStamp = new Date();

    eventDescription = `Estado de la batería de la alarma: ${payloadParsed.estadoBateria}`;


    // Se guarda el evento en la base de datos
    await addEvent(db, comId, parsedMessage.layer, eventDescription, eventTimeStamp);

    // Se avisa por el broker que hay una novedad para este equipo
    await updateMqtt(iotdata, comId, parsedMessage.layer);


    // Se envía la notificación push a todos los usuarios que tienen dado de alta al
    // comunicador comId
    await sendPushNotifications(
      db,
      sns,
      comId,
      comName,
      eventDescription
    );
  }
  catch (error) {
    console.log("[IOT] " + error);
  }

  
  return 0;
}

export const handler = iotEventRed;
