
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import {IotData} from 'aws-sdk';
import synchTimeSchema from '../../schemas/syncTimeSchema';
import {httpStatus} from '../../lib/httpStatus';
import {BrokerMessage} from '../../lib/brokerMessage';
import {BrokerCommands} from '../../lib/brokerCommands';
import {BrokerRegisters} from '../../lib/brokerRegisters';


const iotdata = new IotData({endpoint: process.env.IOT_ENDPOINT});


async function syncTime(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const {comId, sincronizaHora, codigoRegion} = event.body;

    let msg = new BrokerMessage(
        BrokerCommands.SET,
        BrokerRegisters.CONFIGURACION_TIEMPO,
        0
    );

    let sincroniza = (sincronizaHora)? 1 : 0;
    msg.addByte(sincroniza);
    msg.addByte(codigoRegion);
    

    let msgBuffer = msg.getBufferToSend();

    let params = {
        topic: comId + "/cmd",
        payload: msgBuffer.toString("hex"),
        qos: 0
    };
    

    try {
        await iotdata.publish(params).promise();
    }
    catch(error) {
        return httpStatus(500, error);
    }

    return httpStatus(200, {message: "Mensaje publicado"})
}

export const handler = commonMiddleware(syncTime)
    .use(validator({
        inputSchema: synchTimeSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))