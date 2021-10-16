const schema = {
    type: 'object',
    properties: {
        body: {
            type: 'object',
            properties: {
                email: {
                    type: 'string',
                },
                currentPassword: {
                    type: 'string',
                },
                newPassword: {
                    type: 'string',
                },
            },
            required: ['email', 'currentPassword', 'newPassword'],
        },
    },
    required: ['body'],
};

export default schema;