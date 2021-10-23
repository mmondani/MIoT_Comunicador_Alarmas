const schema = {
    type: 'object',
    properties: {
        body: {
            type: 'object',
            properties: {
                comId: {
                    type: 'string',
                },
                numero: {
                    type: 'number',
                }
            },
            required: ['comId', 'numero'],
        },
    },
    required: ['body'],
};

export default schema;