export interface Automatizacion {
    numero: number,
    nombre: string,
    tipo: "fototimer" | "programacion_horaria" | "noche" | "simulador",
    horaInicio: string,
    horaFin: string,
    horas: number,
    nodos: number[]
}