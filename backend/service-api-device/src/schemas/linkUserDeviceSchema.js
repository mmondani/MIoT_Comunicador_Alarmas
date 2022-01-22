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
                clave: {
                    type: 'string',
                }
            },
            required: ['comId', 'email'],
        },
    },
    required: ['body'],
};

export default schema;