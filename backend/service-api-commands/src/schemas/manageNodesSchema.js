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
                nodos: {
                    type: 'array',
                }
            },
            required: ['comId', 'particion', 'nodos'],
        },
    },
    required: ['body'],
};

export default schema;