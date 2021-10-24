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
                modo: {
                    type: 'string',
                }
            },
            required: ['comId', 'particion', 'modo'],
        },
    },
    required: ['body'],
};

export default schema;