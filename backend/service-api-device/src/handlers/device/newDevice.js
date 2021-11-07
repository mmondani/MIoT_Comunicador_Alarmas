
import commonMiddleware from '../../lib/commonMiddleware';
import validator from '@middy/validator';
import newDeviceSchema from '../../schemas/newDeviceSchema';
import {httpStatus} from '../../lib/httpStatus';


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


async function newDevice(event, context) {
  context.callbackWaitsForEmptyEventLoop = false;


  // Se crea el comunicador en Atlas
  const db = await connectToDatabase();

  let device = {
    comId: event.body.comId,
    online: false,
    nombre: "",
    sincronizaHora: true,
    codigoRegion: 1,
    icono: "",
    clavem: "",
    claveh: "",
    celularAsalto: "",
    versionFirmware: "",
    cantidadZonas: 32,
    usaApp: true,
    monitoreada: false,
    estadoRedElectrica: true,
    estadoBateria: "bien",
    estadoMpxh: true,
    usuarios: [],
    particiones: [
      {
        numero: 1,
        nombre: "Particion 1",
        retardoDisparo: 8,
        estado: "desactivada",
        sonando: false,
        lista: true,
        modo: "estoy",
        zonasAnormales: "00000000000000000000000000000000",
        zonasMemorizadas: "00000000000000000000000000000000",
        zonasIncluidas: "00000000000000000000000000000000",
        zonasCondicionales: "0000",
        nodosEncendidos: "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        tipoDisparo: 0,
        replayDisparo: "",
        zonas: [],
        nodos: [],
        usuariosAlarma: [],
        automatizaciones: [],
        eventosAlarma: []
      }
    ]
  }

  try {
    await db.collection("devices").insertOne(device);
  }
  catch (error) {
    console.log("[Atlas] " + error);

    if (error.code === 11000) {
      // Ya existe el comId
      return httpStatus(409, {error: "Ya se registró ese comId"});
    }
    else {
      return httpStatus(500, error);
    }

  }

  return httpStatus(201, device);
}

export const handler = commonMiddleware(newDevice)
  .use(validator({
    inputSchema: newDeviceSchema,
    ajvOptions: {
      useDefaults: true,
      strict: false,
    },
  }))