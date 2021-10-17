export const httpStatus = (code, body) => {
    return {
        statusCode: code,
        body: JSON.stringify(body),
    }
}

export const httpStatusWithHtml = (code, htmlString) => {
    return {
        statusCode: code,
        headers: {
            "Content-Type": "text/html"
        },
        body: htmlString,
    }
}