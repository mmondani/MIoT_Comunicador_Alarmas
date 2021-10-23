const schema = {
    type: 'object',
    properties: {
        body: {
            type: 'object',
            properties: {
                comId: {
                    type: 'string',
                },
                numero: {
                    type: 'number',
                },
                nombre: {
                    type: 'string',
                }
            },
            required: ['comId', 'numero', 'nombre'],
        },
    },
    required: ['body'],
};

export default schema;