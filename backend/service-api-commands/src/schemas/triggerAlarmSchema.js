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
                causa: {
                    type: 'string',
                }
            },
            required: ['comId', 'particion', 'causa'],
        },
    },
    required: ['body'],
};

export default schema;