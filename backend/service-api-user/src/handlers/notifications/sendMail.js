import AWS from 'aws-sdk';

const ses = new AWS.SES({region: "sa-east-1"});

/**
 * El mensaje que viene en la queue tiene que tener el siguiente formato
 * {
 *    subject: "....",
 *    recipient: ".....",
 *    body: "....."
 *    isHtml: "true/false"
 * }
 */
async function sendMail(event, context) {
    const record = event.Records[0];
    const emailObject = JSON.parse(record.body);

    const {subject, body, recipient, isHtml} = emailObject;

    /**
     * El contenido del mail puede ser texto plano o HTML
     * El tipo de texto que se mande en el body del mensaje de SQS lo define
     * la propiedad isHTML
     */
    let bodyObject = {};
    if (isHtml) {
        bodyObject = {
        Html: {
            Charset: "UTF-8", 
            Data: body
        }
        }
    }
    else {
        bodyObject = {
        Text: {
            Data: body,
        }
        }
    }

    const params = {
        // Tiene que ser una dirección que esté habilitada desde la consola de SES
        Source: 'equipo.com38@gmail.com',
        Destination: {
        ToAddresses: [recipient],
        },
        Message: {
        Body: bodyObject,
        Subject: {
            Data: subject,
        },
        },
    };

    try {
        const result = await ses.sendEmail(params).promise();

        return result;
    } catch (error) {
        console.error(error);
    }
}

export const handler = sendMail;


