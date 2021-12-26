import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { environment } from '../../environments/environment';

@Injectable({
  providedIn: 'root'
})
export class AuthService {
  private _userAuthenticated = false;

  constructor(private http: HttpClient) { }

  get userIsAuthenticated() {
    return this._userAuthenticated;
  }

  createAccount (email: string, password: string) {
    return this.http.post(
      environment.api_url + "/user",
      {
        email: email,
        password: password
      }
    );
  }

  recoverPassword (email: string) {
    return this.http.post(
      environment.api_url + "/user/forgot-password",
      {
        email: email
      }
    );
  }

  recoverPasswordPin (email: string, password: string, pin: string) {
    return this.http.post(
      environment.api_url + "/user/recover-password",
      {
        email: email,
        password: password,
        pin: pin
      }
    );
  }
}
