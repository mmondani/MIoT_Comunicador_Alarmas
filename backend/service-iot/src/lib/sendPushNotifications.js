export const sendPushNotifications = async (db, sns, comId, title, body) => {
    let users = await db.collection("users").find({comunicadores: comId}, {apps: 1, _id: 0}).toArray();

    if (users.length > 0) {
        // Se agrupan todos los SNS Endpoints de todas las apps
        let endpointsArn = [];

        users.forEach(user => {
            if (user.apps.length > 0) {
            user.apps.forEach(app => {
                endpointsArn.push(app.snsEndpointArn);
            })
            }
        });

        // Se envia la push notification a cada uno de las apps
        let snsPromises = [];
        let snsPush = {"GCM": `{\"notification\": { \"title\": \"${title}\", \"body\": \"${body}\", \"sound\":\"default\", \"android_channel_id\":\"Miscellaneous\"},  \"android\": {\"priority\":\"high\"}}`}
        endpointsArn.forEach(endpoint => {
            let snsParams = {
                Message: JSON.stringify(snsPush),
                TargetArn: endpoint,
                MessageStructure: 'json'
            }

            snsPromises.push(sns.publish(snsParams).promise());
        });

        if (snsPromises.length > 0)
            await Promise.all(snsPromises);
    }
}