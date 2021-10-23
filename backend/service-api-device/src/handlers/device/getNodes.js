import commonMiddleware from '../../lib/commonMiddleware';
import {httpStatus} from '../../lib/httpStatus';

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

async function getNodes(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {comId, particion} = event.pathParameters;

    let nodes;

    try {
        nodes = await db.collection("devices")
            .findOne(
                {comId: comId, "particiones.numero": parseInt(particion)},
                {projection: {
                        "particiones.$": 1,
                        _id: 0
                    }
                }
            );

        if (nodes) {
            nodes = nodes.particiones[0].nodos;
        }
        else 
            return httpStatus(409, {error: `Error al solicitar los nodos de la partición ${particion} del comunicador ${comId}`})
    }
    catch (error) {
        console.log(error);
        
        return httpStatus(500, error);
    }

    return httpStatus(200, nodes);
}

export const handler = commonMiddleware(getNodes);