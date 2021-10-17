const schema = {
    type: 'object',
    properties: {
        body: {
            type: 'object',
            properties: {
                appId: {
                    type: 'string',
                },
                token: {
                    type: 'string',
                },
                email: {
                    type: 'string',
                },
            },
            required: ['appId', 'token', 'email'],
        },
    },
    required: ['body'],
};

export default schema;