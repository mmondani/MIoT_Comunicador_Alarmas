import {IotData, SNS} from 'aws-sdk';
import { parseHeader, parseRegisterOpenClose } from "../lib/parser";

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

async function iotEventOpenClose(event, context) {

  context.callbackWaitsForEmptyEventLoop = false;

  const db = await connectToDatabase();

  let comId = event.clientId;
  let payload = event.payload;
  let payloadBuffer = Buffer.from(payload, "hex");

  let parsedMessage = parseHeader(payloadBuffer);
  let payloadParsed = parseRegisterOpenClose(parsedMessage);

  console.log (`Mensaje recibido de ${comId} con payload ${payload}`);
  console.log(JSON.stringify(parsedMessage));
  console.log(JSON.stringify(payloadParsed));

  try {
    // Se actualiza el estado de la partición
    await db.collection("devices").updateOne (
      {comId: comId, "particiones.numero": parsedMessage.layer + 1},
      {$set:{
        "particiones.$.estado": payloadParsed.estado_alarma
      }}
    );

    // Se busca el nombre del usuario de la alarma y el nombre del comunicador en la base de datos
    // Se lo va a usar para armar la notificación y guardar el evento
    let alarmUsers = await db.collection("devices").aggregate(
      [
        {$match: {comId: comId}},
        {$unwind: "$particiones"},
        {$match: {"particiones.numero": parsedMessage.layer + 1}},
        {$project: {nombre: 1, "particiones.nombre": 1, "particiones.usuariosAlarma": 1, _id: 0}}
      ]
    ).toArray();

    let comName = `Comunicador ${comId}`;
    let partitionName = `partición ${parsedMessage.layer + 1}`;
    let userName = `usuario ${payloadParsed.usuario}`;
    if (alarmUsers && alarmUsers.length > 0) {
      comName = alarmUsers[0].nombre;
      partitionName = alarmUsers[0].particiones.nombre;
      alarmUsers[0].particiones.usuariosAlarma.forEach(user => {
        if (user.numero === payloadParsed.usuario)
          userName = user.nombre;
      });
    }

    let eventDescription;
    let eventTimeStamp = new Date();

    switch (payloadParsed.estado_alarma) {
      case "desactivada":
        eventDescription = `Alarma desactivada por ${userName}`;
        break;

      case "activada":
        eventDescription = `Alarma activada por ${userName}`;
        break;

      case "activada_estoy":
        eventDescription = `Alarma activada en modo Estoy por ${userName}`;
        break;

      case "activada_me_voy":
        eventDescription = `Alarma activada en modo Me Voy por ${userName}`;
        break;

      case "activacion_parcial":
        eventDescription = `Alarma activada parcialmente por ${userName}`;
        break;
    }


    // Se guarda el evento en la base de datos
    await db.collection("devices").updateOne (
      {comId: comId, "particiones.numero": parsedMessage.layer + 1},
      {$push:{
        "particiones.$.eventosAlarma": {
          timestamp: eventTimeStamp,
          evento: eventDescription
        }
      }}
    );


    // Se avisa por el broker que hay una novedad para este equipo
    let params = {
      topic: comId + "/new",
      payload: `{particion: ${parsedMessage.layer + 1}}`,
      qos: 0
    };

    await iotdata.publish(params).promise();


    // Se envía la notificación push a todos los usuarios que tienen dado de alta al
    // comunicador comId
    let users = await db.collection("users").find({comunicadores: comId}, {apps: 1, _id: 0}).toArray();

    if (users.length > 0) {
      // Se agrupan todos los SNS Endpoints de todas las apps
      let endpointsArn = [];

      users.forEach(user => {
        if (user.apps.length > 0) {
          user.apps.forEach(app => {
            endpointsArn.push(app.snsEndpointArn);
          })
        }
      });

      // Se envia la push notification a cada uno de las apps
      let snsPromises = [];
      let snsPush = {"GCM": `{\"notification\": { \"title\": \"${comName} - ${partitionName}\", \"body\": \"${eventDescription}\", \"sound\":\"default\", \"android_channel_id\":\"Miscellaneous\"},  \"android\": {\"priority\":\"high\"}}`}
      endpointsArn.forEach(endpoint => {
          let snsParams = {
              Message: JSON.stringify(snsPush),
              TargetArn: endpoint,
              MessageStructure: 'json'
          }

          snsPromises.push(sns.publish(snsParams).promise());
      });

      if (snsPromises.length > 0)
        await Promise.all(snsPromises);
    }
  }
  catch (error) {
    console.log("[IOT] " + error);
  }

  
  return 0;
}

export const handler = iotEventOpenClose;
