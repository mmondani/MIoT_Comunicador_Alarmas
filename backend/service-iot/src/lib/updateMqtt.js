export const updateMqtt = async (iotdata, comId, layer) => {
    let params = {
        topic: comId + "/new",
        payload: `{"particion": ${layer + 1}}`,
        qos: 0
    };

    await iotdata.publish(params).promise();
}