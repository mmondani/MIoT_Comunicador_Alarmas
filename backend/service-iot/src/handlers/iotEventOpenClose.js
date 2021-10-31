import { parseHeader, parseRegisterOpenClose } from "../lib/parser";

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

async function iotEventOpenClose(event, context) {

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

    // Se busca el nombre del usuario de la alarma en la base de datos
    // Se lo va a usar para armar la notificación y guardar el evento
    let users = await db.collection("devices").aggregate(
      [
        {$match: {comId: comId}},
        {$unwind: "$particiones"},
        {$match: {"particiones.numero": parsedMessage.layer + 1}},
        {$project: {"particiones.usuariosAlarma": 1, _id: 0}}
      ]
    ).toArray();

    console.log(JSON.stringify(users));

    let userName = `usuario ${payloadParsed.usuario}`;
    if (users && users.length > 0) {
      users[0].particiones.usuariosAlarma.forEach(user => {
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

    // Se envía la notificación push
  }
  catch (error) {
    console.log("[IOT] " + error);
  }

  
  return 0;
}

export const handler = iotEventOpenClose;
