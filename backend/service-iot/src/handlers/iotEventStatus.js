import {IotData, SNS} from 'aws-sdk';
import { parseStatus } from "../lib/parser";
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

async function iotEventStatus(event, context) {

  let records = event.Records;

  context.callbackWaitsForEmptyEventLoop = false;

  const db = await connectToDatabase();

  let promises = []
  records.forEach(async function (record) {
    let message = JSON.parse(record.body);
    let comId = message.clientId;
    let payload = message.payload;

    let payloadParsed = parseStatus(payload);

    console.log (`Mensaje recibido de ${comId} con payload ${payload}`);
    console.log(JSON.stringify(payloadParsed));

    // Se actualiza el estado de la partición
    promises.push(db.collection("devices").updateOne (
      {comId: comId},
      {$set:{
        online: payloadParsed.online
      }}
    ));

    let eventDescription;
    let eventTimeStamp = new Date();

    if (payloadParsed.online) 
      eventDescription = `Comunicación restablecida`;
    else 
      eventDescription = `Pérdida de comunicación`;


    // Se guarda el evento en la base de datos
    promises.push(addEvent(db, comId, 0, eventDescription, eventTimeStamp));

    // Se avisa por el broker que hay una novedad para este equipo
    promises.push(updateMqtt(iotdata, comId, 0));
  });


  try {
    await Promise.all(promises);
  }
  catch (error) {
    console.log("[IOT] " + error);
  }

  
  return 0;
}

export const handler = iotEventStatus;
