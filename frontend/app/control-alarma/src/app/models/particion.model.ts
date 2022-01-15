import { Zona } from './zona.model';
import { Nodo } from './nodo.model';
import { UsuarioAlarma } from './usuario-alarma.model';
import { Automatizacion } from './automatizacion.model';
import { EventoAlarma } from './evento-alarma.model';

export type Estado = "desactivada" | "activada" | "activada_estoy" | "activada_me_voy" | "activacion_parcial" | "programacion";
export type Modo = "estoy" | "me_voy" | "ninguno";
export type TipoDisparo = "ninguno" | "robo" | "asalto" | "incendio" | "incendio_manual" | "tamper" | "emergencia_medica" | "panico";
export type CausaDisparo = "incendio" | "medico" | "panico";

export interface Particion {
    numero: number,
    nombre: string,
    retardoDisparo: number,
    estado: Estado,
    sonando: boolean,
    modo: Modo,
    zonasAnormales: string,
    zonasMemorizadas: string,
    zonasIncluidas: string,
    zonasCondicionales: string,
    tipoDisparo: TipoDisparo,
    replayDisparo: string,
    ready: boolean,
    nodosEncendidos: string,
    zonas: Zona[],
    nodos: Nodo[],
    usuariosAlarma: UsuarioAlarma[],
    automatizaciones: Automatizacion[],
    eventosAlarma: EventoAlarma[],
}