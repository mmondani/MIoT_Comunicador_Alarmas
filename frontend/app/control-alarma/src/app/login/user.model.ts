export class User {
    constructor (public email: string, private _token: string) {};

    get token() {
        if (this.tokenExpired(this._token))
            return null;
            
        return this._token;
    }

    private tokenExpired(token: string) {
        const expiry = (JSON.parse(atob(token.split('.')[1]))).exp;
        return (Math.floor((new Date).getTime() / 1000)) >= expiry;
    }
}