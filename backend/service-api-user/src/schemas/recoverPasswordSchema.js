const schema = {
    type: 'object',
    properties: {
        body: {
            type: 'object',
            properties: {
                email: {
                    type: 'string',
                },
                password: {
                    type: 'string',
                },
                pin: {
                    type: 'string',
                }
            },
            required: ['email', 'password', 'pin'],
        },
    },
    required: ['body'],
};

export default schema;