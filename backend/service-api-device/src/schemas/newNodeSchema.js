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
                icono: {
                    type: 'string',
                }
            },
            required: ['comId', 'particion', 'numero', 'nombre', 'icono'],
        },
    },
    required: ['body'],
};

export default schema;