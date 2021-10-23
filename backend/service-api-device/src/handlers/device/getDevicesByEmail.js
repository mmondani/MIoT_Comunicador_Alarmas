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

async function getDevicesByEmail(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {email} = event.pathParameters;

    let devices;

    try {
        devices = await db.collection("devices")
            .find({"usuarios.email":email})
            .toArray();
    }
    catch (error) {
        console.log(error);
        
        return httpStatus(500, error);
    }

    return httpStatus(200, devices);
}

export const handler = commonMiddleware(getDevicesByEmail);