export const addEvent = async (db, comId, layer, event, timestamp) => {
    await db.collection("devices").updateOne (
        {comId: comId, "particiones.numero": layer + 1},
        {$push:{
            "particiones.$.eventosAlarma": {
            timestamp: timestamp,
            evento: event
            }
        }}
    );
}