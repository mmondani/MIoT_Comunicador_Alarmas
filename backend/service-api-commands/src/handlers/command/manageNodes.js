
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import {IotData} from 'aws-sdk';
import manageNodesSchema from '../../schemas/manageNodesSchema';
import {httpStatus} from '../../lib/httpStatus';
import {BrokerMessage} from '../../lib/brokerMessage';
import {BrokerCommands} from '../../lib/brokerCommands';
import {BrokerRegisters} from '../../lib/brokerRegisters';


const iotdata = new IotData({endpoint: process.env.IOT_ENDPOINT});


async function manageNodes(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const {comId, particion, nodos} = event.body;

    let msg = new BrokerMessage(
        BrokerCommands.SET,
        BrokerRegisters.ESTADO_NODOS,
        parseInt(particion)-1
    );

    /**
     * Cada una de los nodos tiene el siguiente formato:
     * 
     * {
     *      nodo: 1,
     *      encender: true
     * }
     */
     nodos.forEach(nodo => {
        msg.addByte(nodo.nodo);

        if (nodo.encender)
            msg.addByte(1);
        else
            msg.addByte(0);
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

export const handler = commonMiddleware(manageNodes)
    .use(validator({
        inputSchema: manageNodesSchema,
        ajvOptions: {
        useDefaults: true,
        strict: false,
        },
    }))