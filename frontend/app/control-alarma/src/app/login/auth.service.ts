import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { BehaviorSubject, from } from 'rxjs';
import { map, take, tap } from 'rxjs/operators'
import { Storage } from '@capacitor/storage'

import { environment } from '../../environments/environment';
import { User } from '../models/user.model';


interface LoginResponse {
  token: string
}

@Injectable({
  providedIn: 'root'
})
export class AuthService {
  private _user = new BehaviorSubject<User>(null);

  constructor(private http: HttpClient) { }

  get userIsAuthenticated() {
    return this._user.asObservable().pipe(map(user => {
      if (user)
        return !!user.token;
      else
        return false;
    }))
  }

  get userEmail () {
    return this._user.asObservable().pipe(map(user => {
      if (user)
        return user.email;
      else
        return null;
    }));
  }

  get token () {
    return this._user.asObservable().pipe(map(user => {
      if (user)
        return user.token;
      else
        return null;
    }));
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

  login (email: string, password: string) {
    return this.http.post<LoginResponse> (
      environment.api_url + "/user/login",
      {
        email: email,
        password: password
      }
    ).pipe(tap (userData => {
      this._user.next(new User(email, userData.token));

      // Se guarda la información del usuario logueado en la memoria del dispositivo
      this.storeLoginData(email, userData.token)
    }));
  }

  autoLogin () {
    // Determino si hay información de un login anterior en la memoria.
    // Devuelve true si hay información o false si no lo hay
    return from(Storage.get({key: "authData"})
    ).pipe(
      take(1),
      map(storedData => {
        if (!storedData || !storedData.value)
          return null;
        
        const parsedData = JSON.parse(storedData.value) as {email: string, token: string};

        const user = new User(parsedData.email, parsedData.token);

        // Chequea si el token está vencido
        if (!user.token) {
          return null;
        }

        return user;
      }),
      tap( user => {
        if (user)
          this._user.next(user);
      }),
      map(user => {
        return !!user;
      })
    );
  }

  logout() {
    this._user.next(null);

    Storage.remove({key: "authData"});
  }

  private storeLoginData (email: string, token: string) {
    const data = {email: email, token: token};

    Storage.set({
      key: "authData",
      value: JSON.stringify(data)
    })
  }
}
