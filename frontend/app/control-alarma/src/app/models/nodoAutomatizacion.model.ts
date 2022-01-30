export class NodoAutomatizacion {
    numero: number;
    nombre: string;
    tipo: "nodo" | "fototimer" | "programacion_horaria" | "noche" | "simulador";
    encendido: boolean;
    horaInicio: string;
    horaFin: string;
    horas: number;
    nodos: number[];

    constructor(number: number, name: string, type: "nodo" | "fototimer" | "programacion_horaria" | "noche" | "simulador", on?: boolean, timeStart?: string, timeEnd?: string, hours?: number, nodes?: number[]) {
        this.numero = number;
        this.nombre = name;
        this.tipo = type;
        this.encendido = on;
        this.horaInicio = timeStart;
        this.horaFin = timeEnd;
        this.horas = hours;
        this.nodos = nodes;
    }
}