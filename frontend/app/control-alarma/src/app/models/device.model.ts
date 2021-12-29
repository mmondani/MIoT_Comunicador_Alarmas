
import { Particion } from './particion.model';
import { UsuarioApp } from './usuario-app.model';


export interface Device {
    _id: string,
    comId: string,
    online: boolean,
    nombre: string,
    sincronizarHora: boolean,
    codigoRegion: number,
    icono: string,
    clavem: string,
    claveh: string,
    celularAsalto: string,
    versionFirmware: string,
    cantidadZonas: number,
    usaApp: boolean,
    monitoreada: boolean,
    estadoRedElectrica: boolean,
    estadoBateria: "bien" | "dudosa" | "baja",
    estadoMpxh: boolean,
    usuarios: UsuarioApp[],
    particiones: Particion[]
}