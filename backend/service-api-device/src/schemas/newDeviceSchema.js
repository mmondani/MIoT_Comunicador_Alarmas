const schema = {
    type: 'object',
    properties: {
        body: {
            type: 'object',
            properties: {
                comId: {
                    type: 'string',
                }
            },
            required: ['comId'],
        },
    },
    required: ['body'],
};

export default schema;