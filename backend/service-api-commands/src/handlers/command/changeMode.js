
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import {IotData} from 'aws-sdk';
import changeModeSchema from '../../schemas/changeModeSchema';
import {httpStatus} from '../../lib/httpStatus';
import {BrokerMessage} from '../../lib/brokerMessage';
import {BrokerCommands} from '../../lib/brokerCommands';
import {BrokerRegisters} from '../../lib/brokerRegisters';


const iotdata = new IotData({endpoint: process.env.IOT_ENDPOINT});


async function changeMode(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const {comId, particion, modo} = event.body;

    let msg = new BrokerMessage(
        BrokerCommands.SET,
        BrokerRegisters.SONANDO_READY,
        parseInt(particion)-1
    );

    switch(modo) {
        case "estoy": msg.addByte(1); break;
        case "me_voy": msg.addByte(2); break;
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

export const handler = commonMiddleware(changeMode)
    .use(validator({
        inputSchema: changeModeSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))