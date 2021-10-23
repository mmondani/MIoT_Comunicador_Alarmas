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
                }
            },
            required: ['comId', 'particion', 'numero'],
        },
    },
    required: ['body'],
};

export default schema;