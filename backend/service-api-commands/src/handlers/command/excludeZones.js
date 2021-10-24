
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import {IotData} from 'aws-sdk';
import excludeZonesSchema from '../../schemas/excludeZonesSchema';
import {httpStatus} from '../../lib/httpStatus';
import {BrokerMessage} from '../../lib/brokerMessage';
import {BrokerCommands} from '../../lib/brokerCommands';
import {BrokerRegisters} from '../../lib/brokerRegisters';


const iotdata = new IotData({endpoint: process.env.IOT_ENDPOINT});


async function excludeZones(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const {comId, particion, zonas} = event.body;

    let msg = new BrokerMessage(
        BrokerCommands.SET,
        BrokerRegisters.INCLUSION,
        parseInt(particion)-1
    );

    /**
     * Cada una de las zonas tiene el siguiente formato:
     * 
     * {
     *      zona: 1,
     *      excluir: true
     * }
     */
    zonas.forEach(zona => {
        msg.addByte(zona.zona);

        if (zona.excluir)
            msg.addByte(0);
        else
            msg.addByte(1);
    });


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

export const handler = commonMiddleware(excludeZones)
    .use(validator({
        inputSchema: excludeZonesSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))