import {SNS} from 'aws-sdk';
import validator from '@middy/validator';
import commonMiddleware from '../../lib/commonMiddleware';
import {httpStatus} from '../../lib/httpStatus';
import setAppTokenSchema from '../../schemas/setAppTokenSchema'

const sns = new SNS({
    region: "sa-east-1",
    apiVersion: "2010-03-31"
});
const MongoClient = require("mongodb").MongoClient;

// Se define la conexión a la base de datos por fuera del handler para que pueda ser reusada
// por llamados sucesivos
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


async function setAppToken(event, context) {
    context.callbackWaitsForEmptyEventLoop = false;

    const db = await connectToDatabase();

    const {appId, token, email} = event.body;

    try {
        // Se determina si algún usuario ya tiene asociado el appId,
        // es decir, que ya está logueado en esa app.
        let users = await db.collection("users").find({"apps.appId": appId}).toArray();

        // Se el objeto que va a representar una app dentro de la base de datos
        let appObject = {
            appId: appId,
            tokenPush: token,
            snsEndpointArn: ""
        }

        if (users.length === 0) {
            // Si ningún usuario tiene asociada esta app, significa que es la 
            // primera vez que la app está haciendo un request a este endpoint

            // Se crea el endpoint en SNS para esta app
            let endpointArn = await createSNSEndpoint(process.env.SNS_PLATFORM_FCM_ARN, token, appId);
            appObject.snsEndpointArn = endpointArn;

            // Se agrega el appObject al usuario indicado por el email del body
            await db.collection("users").updateOne(
                {email: email},
                {$push:{apps: appObject}});

            console.log("Se asoció una app a un usuario y se creó un token");
            return httpStatus(200, {});
        }
        else {
            // Ya el appId está asociado a algún usuario
            // Se obtiene el objeto de la app 
            let user = users[0];

            let app = user.apps.filter((item) => item.appId === appId)[0];

            if (user.email === email) {
                // La app sigue están asociada al mismo usuario
                if (app.tokenPush === token) {
                    // El token de la app con appId no cambió. No hay que hacer nada
                    console.log("El token no cambió");
                    return httpStatus(200, {});
                }
                else {
                    // El token cambió para la app con appId. Hay que dar de baja el endpoint de SNS, crear uno nuevo
                    // y actualizar la base de datos
                    await deleteSNSEndpoint(app.snsEndpointArn);

                    // Se crea el endpoint con el nuevo token
                    let endpointArn = await createSNSEndpoint(process.env.SNS_PLATFORM_FCM_ARN, token, appId);

                    // Se actualiza la base de datos con el nuevo token y el nuevo ARN del endpoint
                    await db.collection("users").updateOne(
                        {"apps.appId": appId},
                        {$set:{"apps.$.tokenPush": token, "apps.$.snsEndpointArn": endpointArn}});

                    console.log("El token cambió y se actualizó");
                    return httpStatus(200, {});
                }
            }
            else {
                // La app ahora está asociada a otro usuario (porque se logueó con otra cuenta)
                // Se elimina del array apps del usuario el objeto que representa a este appId
                await db.collection("users").updateOne(
                    {email: user.email},
                    {$pull:{apps:{appId: appId}}}
                );

                if (app.tokenPush !== token) {
                    // Si además cambió el token, se elimina el endpoint actual, se crea uno nuevo y 
                    // se actualiza el object de app
                    await deleteSNSEndpoint(app.snsEndpointArn);

                    // Se crea el endpoint con el nuevo token
                    let endpointArn = await createSNSEndpoint(process.env.SNS_PLATFORM_FCM_ARN, token, appId);

                    app.snsEndpointArn = endpointArn;

                    console.log("El token cambió");
                }

                // Se inserta el object app en el array apps del nuevo email
                await db.collection("users").updateOne(
                    {email: email},
                    {$push:{apps: app}});
    
                console.log("La app cambió de usuario");
                return httpStatus(200, {});
            }
        }
    }
    catch (error) {
        console.log(error);
        return httpStatus(500, error);
    }
}

export const handler = commonMiddleware(setAppToken)
    .use(validator({
            inputSchema: setAppTokenSchema,
            ajvOptions: {
            useDefaults: true,
            strict: false,
            },
        }));


async function createSNSEndpoint(snsArn, token, appId) {
    let snsParams = {
        Token: token,
        PlatformApplicationArn: snsArn,
        CustomUserData: appId
    }

    let data = await sns.createPlatformEndpoint(snsParams).promise();

    return data.EndpointArn;
}


async function deleteSNSEndpoint(endpointArn) {
    let snsParams = {
        EndpointArn: endpointArn
    }

    await sns.deleteEndpoint(snsParams).promise();
}