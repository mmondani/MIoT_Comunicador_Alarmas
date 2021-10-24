
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import {IotData} from 'aws-sdk';
import armDisarmSchema from '../../schemas/armDisarmSchema';
import {httpStatus} from '../../lib/httpStatus';
import {BrokerMessage} from '../../lib/brokerMessage';


const iotdata = new IotData({endpoint: process.env.IOT_ENDPOINT});


async function armDisarm(event, context) {
  context.callbackWaitsForEmptyEventLoop = false;

  const {comId, particion, estado, clave} = event.body;

  let msg = new BrokerMessage(
    0x02,
    0x0001,
    parseInt(particion)-1
  );

  switch(estado) {
    case "desactivada": msg.addByte(1); break;
    case "activada": msg.addByte(2); break;
    case "activada_estoy": msg.addByte(3); break;
    case "activada_me_voy": msg.addByte(4); break;
  }

  let bufferClave = Buffer.from(clave);
  let auxClave = bufferClave.map(c => {return (c - 0x30)});
  msg.addBytes(auxClave)
  

  let msgBuffer = msg.getBufferToSend();

  let params = {
    topic: comId + "/cmd",
    payload: msgBuffer.toString("hex"),
    qos: 1
  };
  

  try {
    await iotdata.publish(params).promise();
  }
  catch(error) {
    return httpStatus(500, error);
  }

  return httpStatus(200, {message: "Mensaje publicado"})
}

export const handler = commonMiddleware(armDisarm)
    .use(validator({
        inputSchema: armDisarmSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))