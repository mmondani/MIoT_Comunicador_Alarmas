const schema = {
    type: 'object',
    properties: {
        body: {
            type: 'object',
            properties: {
                comId: {
                    type: 'string',
                },
                sincronizaHora: {
                    type: 'boolean',
                },
                codigoRegion: {
                    type: 'number',
                }
            },
            required: ['comId', 'sincronizaHora', 'codigoRegion'],
        },
    },
    required: ['body'],
};

export default schema;