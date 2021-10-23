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
                tipo: {
                    type: 'string',
                }
            },
            required: ['comId', 'particion', 'numero', 'tipo'],
        },
    },
    required: ['body'],
};

export default schema;