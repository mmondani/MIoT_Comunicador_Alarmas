import { Zona } from './zona.model';
import { Nodo } from './nodo.model';
import { UsuarioAlarma } from './usuario-alarma.model';
import { Automatizacion } from './automatizacion.model';
import { EventoAlarma } from './evento-alarma.model';


export interface Particion {
    numero: number,
    nombre: string,
    retardoDisparo: number,
    estado: "desactivada" | "activada" | "activada_estoy" | "activada_me_voy" | "activacion_parcial" | "programacion",
    sonando: boolean,
    modo: "estoy" | "me_voy" | "ninguno",
    zonasAnormales: string,
    zonasMemorizadas: string,
    zonasIncluidas: string,
    zonasCondicionales: string,
    tipoDisparo: "ninguno" | "robo" | "asalto" | "incendio" | "incendio_manual" | "tamper" | "emergencia_medica" | "panico",
    replayDisparo: string,
    ready: boolean,
    nodosEncendidos: string,
    zonas: Zona[],
    nodos: Nodo[],
    usuariosAlarma: UsuarioAlarma[],
    automatizaciones: Automatizacion[],
    eventosAlarma: EventoAlarma[],
}