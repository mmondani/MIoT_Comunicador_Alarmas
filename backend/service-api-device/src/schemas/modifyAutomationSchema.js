const schema = {
    type: 'object',
    properties: {
        body: {
            type: 'object',
            properties: {
                comId: {
                    type: 'string',
                },
                particion: {
                    type: 'number',
                },
                numero: {
                    type: 'number',
                },
                nombre: {
                    type: 'string',
                },
                tipo: {
                    type: 'string',
                },
                horaInicio: {
                    type: 'string',
                },
                horaFin: {
                    type: 'string',
                },
                horas: {
                    type: 'number',
                },
                nodos: {
                    type: 'array',
                }
            },
            required: ['comId', 'particion', 'numero', 'tipo'],
        },
    },
    required: ['body'],
};

export default schema;