import jwt from 'jsonwebtoken';

const generatePolicy = (principalId, methodArn) => {
    const apiGatewayWildcard = methodArn.split('/', 2).join('/') + '/*';

    return {
        principalId,
        policyDocument: {
        Version: '2012-10-17',
        Statement: [
            {
            Action: 'execute-api:Invoke',
            Effect: 'Allow',
            Resource: apiGatewayWildcard,
            },
        ],
        },
    };
};

async function auth(event, context) {
    if (!event.authorizationToken) {
        throw 'Unauthorized';
    }

    const token = event.authorizationToken.replace('Bearer ', '');

    try {
        const claims = jwt.verify(token, process.env.JWT_KEY);

        // El mail todavía no fue verificado
        if (!claims.verificado) {
            throw 'Unauthorized';
        }

        const policy = generatePolicy(claims.email, event.methodArn);

        return {
            ...policy,
            context: claims
        };
    } catch (error) {
        console.log(error);
        throw 'Unauthorized';
    }
};


export const handler = auth;