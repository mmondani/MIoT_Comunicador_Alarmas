const schema = {
    type: 'object',
    properties: {
        body: {
            type: 'object',
            properties: {
                comId: {
                    type: 'string',
                },
                nombre: {
                    type: 'string',
                },
                icono: {
                    type: 'string',
                },
                clavem: {
                    type: 'string',
                },
                claveh: {
                    type: 'string',
                },
                celularAsalto: {
                    type: 'string',
                },
                usaApp: {
                    type: 'boolean',
                },
                monitoreada: {
                    type: 'boolean',
                },
                sincronizaHora: {
                    type: 'boolean',
                },
                codigoRegion: {
                    type: 'number',
                }
            },
            required: ['comId'],
        },
    },
    required: ['body'],
};

export default schema;