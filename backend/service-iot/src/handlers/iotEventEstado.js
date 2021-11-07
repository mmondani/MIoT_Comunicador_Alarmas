import { parseHeader, parseRegisterEstado } from "../lib/parser";

const MongoClient = require("mongodb").MongoClient;

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

async function iotEventEstadoSqs(event, context) {

  let records = event.Records;

  const db = await connectToDatabase();

  let promises = [];
  records.forEach(async function (record) {
    let message = JSON.parse(record.body);
    let comId = message.clientId;
    let payload = message.payload;
    let payloadBuffer = Buffer.from(payload, "hex");

    let parsedMessage = parseHeader(payloadBuffer);
    let payloadParsed = parseRegisterEstado(parsedMessage);

    //console.log (`Mensaje recibido de ${message.clientId} con payload ${message.payload}`);
    //console.log(JSON.stringify(parsedMessage));
    //console.log(JSON.stringify(payloadParsed));

    
    if (parsedMessage.layer === 0) {
      promises.push(
        db.collection("devices").updateOne(
          {comId: comId, "particiones.numero": parsedMessage.layer + 1},
          {$set:{
            "estadoRedElectrica": payloadParsed.estadoRedElectrica,
            "estadoBateria": payloadParsed.estadoBateria,
            "estadoMpxh": payloadParsed.estadoMpxh,
            "cantidadZonas": payloadParsed.cantidadZonas,
            "versionFirmware": payloadParsed.versionFirmware,
            "particiones.$.estado": payloadParsed.estado,
            "particiones.$.sonando": payloadParsed.sonando,
            "particiones.$.ready": payloadParsed.ready,
            "particiones.$.zonasAnormales": payloadParsed.zonasAnormales,
            "particiones.$.zonasMemorizadas": payloadParsed.zonasMemorizadas,
            "particiones.$.zonasIncluidas": payloadParsed.zonasIncluidas,
            "particiones.$.zonasCondicionales": payloadParsed.zonasCondicionales,
            "particiones.$.modo": payloadParsed.modo
          }}
        )
      );
    }
    else {
      promises.push(
        db.collection("devices").updateOne(
          {comId: comId, "particiones.numero": parsedMessage.layer + 1},
          {$set:{
            "particiones.$.estado": payloadParsed.estado,
            "particiones.$.sonando": payloadParsed.sonando,
            "particiones.$.ready": payloadParsed.ready,
            "particiones.$.zonasAnormales": payloadParsed.zonasAnormales,
            "particiones.$.zonasMemorizadas": payloadParsed.zonasMemorizadas,
            "particiones.$.zonasIncluidas": payloadParsed.zonasIncluidas,
            "particiones.$.zonasCondicionales": payloadParsed.zonasCondicionales,
            "particiones.$.modo": payloadParsed.modo
          }}
        )
      );
    }
  });

  try {
    await Promise.all(promises);
  }
  catch (error) {
    console.log("[IOT] " + error);
  }
  
  return 0;
}

export const handler = iotEventEstadoSqs;
