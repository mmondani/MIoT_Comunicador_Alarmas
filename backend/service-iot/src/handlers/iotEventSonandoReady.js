import {IotData, SNS} from 'aws-sdk';
import { parseHeader, parseRegisterSonandoReady } from "../lib/parser";
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

async function iotEventSonandoReady(event, context) {

  context.callbackWaitsForEmptyEventLoop = false;

  const db = await connectToDatabase();

  let comId = event.clientId;
  let payload = event.payload;
  let payloadBuffer = Buffer.from(payload, "hex");

  let parsedMessage = parseHeader(payloadBuffer);
  let payloadParsed = parseRegisterSonandoReady(parsedMessage);

  //console.log (`Mensaje recibido de ${comId} con payload ${payload}`);
  //console.log(JSON.stringify(parsedMessage));
  //console.log(JSON.stringify(payloadParsed));

  try {
    // Se actualiza el estado de las particiones
    await db.collection("devices").updateOne (
      {comId: comId},
      {$set:{
        cantidadZonas: payloadParsed.cantidadZonas,
        "particiones.$[p1].sonando": payloadParsed.particiones[0].sonando,
        "particiones.$[p1].lista": payloadParsed.particiones[0].lista,
        "particiones.$[p1].modo": payloadParsed.particiones[0].modo,
        "particiones.$[p2].sonando": payloadParsed.particiones[1].sonando,
        "particiones.$[p2].lista": payloadParsed.particiones[1].lista,
        "particiones.$[p2].modo": payloadParsed.particiones[1].modo,
        "particiones.$[p3].sonando": payloadParsed.particiones[2].sonando,
        "particiones.$[p3].lista": payloadParsed.particiones[2].lista,
        "particiones.$[p3].modo": payloadParsed.particiones[2].modo,
        "particiones.$[p4].sonando": payloadParsed.particiones[3].sonando,
        "particiones.$[p4].lista": payloadParsed.particiones[3].lista,
        "particiones.$[p4].modo": payloadParsed.particiones[3].modo,
        "particiones.$[p5].sonando": payloadParsed.particiones[4].sonando,
        "particiones.$[p5].lista": payloadParsed.particiones[4].lista,
        "particiones.$[p5].modo": payloadParsed.particiones[4].modo,
        "particiones.$[p6].sonando": payloadParsed.particiones[5].sonando,
        "particiones.$[p6].lista": payloadParsed.particiones[5].lista,
        "particiones.$[p6].modo": payloadParsed.particiones[5].modo,
        "particiones.$[p7].sonando": payloadParsed.particiones[6].sonando,
        "particiones.$[p7].lista": payloadParsed.particiones[6].lista,
        "particiones.$[p7].modo": payloadParsed.particiones[6].modo,
        "particiones.$[p8].sonando": payloadParsed.particiones[7].sonando,
        "particiones.$[p8].lista": payloadParsed.particiones[7].lista,
        "particiones.$[p8].modo": payloadParsed.particiones[7].modo,
      }},
      {arrayFilters: [
        {"p1.numero": 1},
        {"p2.numero": 2},
        {"p3.numero": 3},
        {"p4.numero": 4},
        {"p5.numero": 5},
        {"p6.numero": 6},
        {"p7.numero": 7},
        {"p8.numero": 8},
      ]}
    );

    // Se avisa por el broker que hay una novedad para este equipo
    await updateMqtt(iotdata, comId, parsedMessage.layer);
  }
  catch (error) {
    console.log("[IOT] " + error);
  }

  
  return 0;
}

export const handler = iotEventSonandoReady;
