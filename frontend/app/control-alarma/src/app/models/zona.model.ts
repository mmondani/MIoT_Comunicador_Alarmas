export interface Zona {
    numero: number,
    nombre: string,
    icono: string,
    estado: 'normal' | 'anormal',
    inclusion: 'incluida' | 'excluida' | 'temporizada',
    memorizada: boolean
}