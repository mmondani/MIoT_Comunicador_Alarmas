import {IotData, SNS} from 'aws-sdk';
import { parseHeader, parseRegisterConfigNodos } from "../lib/parser";
import {updateMqtt} from "../lib/updateMqtt";

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

async function iotEventConfigNodos(event, context) {

  context.callbackWaitsForEmptyEventLoop = false;

  const db = await connectToDatabase();

  let comId = event.clientId;
  let payload = event.payload;
  let payloadBuffer = Buffer.from(payload, "hex");

  let parsedMessage = parseHeader(payloadBuffer);
  let payloadParsed = parseRegisterConfigNodos(parsedMessage);

  //console.log (`Mensaje recibido de ${comId} con payload ${payload}`);
  //console.log(JSON.stringify(parsedMessage));
  //console.log(JSON.stringify(payloadParsed));

  try {
    // Se actualiza la automatización
    let params = {
      "particiones.$[p].automatizaciones.$[a].horaInicio": payloadParsed.horaInicio,
      "particiones.$[p].automatizaciones.$[a].horaFin": payloadParsed.horaFin,
      "particiones.$[p].automatizaciones.$[a].horas": payloadParsed.horas,
      "particiones.$[p].automatizaciones.$[a].nodos": payloadParsed.nodos
    };

    for (let prop in params) {
        if (!params[prop])
            delete params[prop];
    }

    await db.collection("devices")
          .updateOne(
              {comId: comId},
              {$set: params},
              {arrayFilters: [{"p.numero": payloadParsed.particion}, {"a.numero": payloadParsed.numero, "a.tipo": payloadParsed.tipo}]});


    // Se avisa por el broker que hay una novedad para este equipo
    await updateMqtt(iotdata, comId, parsedMessage.layer);
  }
  catch (error) {
    console.log("[IOT] " + error);
  }

  
  return 0;
}

export const handler = iotEventConfigNodos;
