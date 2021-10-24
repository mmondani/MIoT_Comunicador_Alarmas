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
                zonas: {
                    type: 'array',
                }
            },
            required: ['comId', 'particion', 'zonas'],
        },
    },
    required: ['body'],
};

export default schema;