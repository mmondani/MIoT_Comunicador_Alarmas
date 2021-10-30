import {IotData} from 'aws-sdk';
import { parseHeader, parsePedirFyH } from "../lib/parser";
import {BrokerMessage} from '../lib/brokerMessage';
import {BrokerCommands} from '../lib/brokerCommands';
import {BrokerRegisters} from '../lib/brokerRegisters';

const MongoClient = require("mongodb").MongoClient;
const iotdata = new IotData({endpoint: process.env.IOT_ENDPOINT});

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

async function iotPedirFyH(event, context) {

  const db = await connectToDatabase();

  let comId = event.clientId;
  let payload = event.payload;
  let payloadBuffer = Buffer.from(payload, "hex");

  let parsedMessage = parseHeader(payloadBuffer);
  let payloadParsed = parsePedirFyH(parsedMessage);

  console.log (`Mensaje recibido de ${comId} con payload ${payload}`);
  console.log(JSON.stringify(parsedMessage));
  console.log(JSON.stringify(payloadParsed));

  try {
    let husoHorario = await db.collection("husos_horarios").findOne({codigo:payloadParsed.cod_region});

    if(husoHorario) {

      let msg = new BrokerMessage(
          BrokerCommands.CONFIGURAR_FECHA_HORA,
          BrokerRegisters.NONE,
          0
      );

      let d = new Date();
      d.setUTCHours(d.getUTCHours() + husoHorario.huso);

      msg.addByte(d.getUTCDate());
      msg.addByte(d.getUTCMonth() + 1);
      msg.addByte(d.getUTCFullYear() - 2000);
      msg.addByte(d.getUTCHours());
      msg.addByte(d.getUTCMinutes());
      msg.addByte(d.getUTCSeconds());

      let msgBuffer = msg.getBufferToSend();

      let params = {
          topic: comId + "/cmd",
          payload: msgBuffer.toString("hex"),
          qos: 0
      };

      console.log(`Mensaje enviado:`);
      console.log(JSON.stringify(params));

      await iotdata.publish(params).promise();
    }
  }
  catch (error) {
    console.log("[IOT] " + error);
  }
  
  return 0;
}

export const handler = iotPedirFyH;
