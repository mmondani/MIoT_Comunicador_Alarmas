const crypto = require ("crypto");


export const hashPassword = (password) => {
    let salt = crypto.randomBytes(16).toString("base64");
    let hash = crypto.createHmac("sha512", salt)
                    .update(password)
                    .digest("base64");

    return salt + "$" + hash;
};


export const isPasswordCorrect = (incomingPassword, storedPassword) => {
    let passwordFields = storedPassword.split("$");
    let salt = passwordFields[0];

    let hash = crypto.createHmac("sha512", salt)
        .update(incomingPassword)
        .digest("base64");
    
    if (hash === passwordFields[1])
        return true;
    else
        return false;
};


export const randomString = (length = 8) => {
    let chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    let str = '';
    
    for (let i = 0; i < length; i++) {
        str += chars.charAt(Math.floor(Math.random() * chars.length));
    }

    return str;
};