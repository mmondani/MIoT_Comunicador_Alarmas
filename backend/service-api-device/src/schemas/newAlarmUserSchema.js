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
                }
            },
            required: ['comId', 'particion', 'numero', 'nombre'],
        },
    },
    required: ['body'],
};

export default schema;