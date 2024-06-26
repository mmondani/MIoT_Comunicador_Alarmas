const schema = {
    type: 'object',
    properties: {
        body: {
            type: 'object',
            properties: {
                email: {
                    type: 'string',
                },
            },
            required: ['email'],
        },
    },
    required: ['body'],
};

export default schema;