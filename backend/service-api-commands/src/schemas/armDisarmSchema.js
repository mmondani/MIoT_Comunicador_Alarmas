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
                estado: {
                    type: 'string',
                },
                clave: {
                    type: 'string',
                }
            },
            required: ['comId', 'particion', 'estado', 'clave'],
        },
    },
    required: ['body'],
};

export default schema;