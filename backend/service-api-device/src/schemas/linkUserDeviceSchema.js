const schema = {
    type: 'object',
    properties: {
        body: {
            type: 'object',
            properties: {
                comId: {
                    type: 'string',
                },
                email: {
                    type: 'string',
                },
                rol: {
                    type: 'string',
                }
            },
            required: ['comId', 'email', 'rol'],
        },
    },
    required: ['body'],
};

export default schema;