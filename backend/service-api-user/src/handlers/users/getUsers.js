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

async function getUsers(event, context) {

    console.log("Llamada por: " + event.requestContext.authorizer.email);

    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    let users;

    try {
        users = await db.collection("users").find({}).limit(20).toArray();
    }
    catch (error) {
        console.log(error);
        
        return httpStatus(500, error);
    }

    return httpStatus(200, users);
}

export const handler = commonMiddleware(getUsers);