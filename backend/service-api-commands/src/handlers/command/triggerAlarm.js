
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import {IotData} from 'aws-sdk';
import triggerAlarmSchema from '../../schemas/triggerAlarmSchema';
import {httpStatus} from '../../lib/httpStatus';
import {BrokerMessage} from '../../lib/brokerMessage';
import {BrokerCommands} from '../../lib/brokerCommands';
import {BrokerRegisters} from '../../lib/brokerRegisters';


const iotdata = new IotData({endpoint: process.env.IOT_ENDPOINT});


async function triggerAlarm(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const {comId, particion, causa} = event.body;

    let msg = new BrokerMessage(
        BrokerCommands.DISPARAR,
        0,
        parseInt(particion)-1
    );

    switch(causa) {
        case "panico": msg.addByte(1); break;
        case "incendio": msg.addByte(2); break;
        case "medico": msg.addByte(3); break;
    }
    

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

export const handler = commonMiddleware(triggerAlarm)
    .use(validator({
        inputSchema: triggerAlarmSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))