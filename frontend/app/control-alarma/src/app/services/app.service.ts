import { Injectable } from '@angular/core';
import { AuthService } from '../login/auth.service';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { Storage } from '@capacitor/storage';
import { from } from 'rxjs';
import { map, switchMap } from 'rxjs/operators';
import { v4 as uuidv4 } from 'uuid';
import { environment } from '../../environments/environment';


@Injectable({
  providedIn: 'root'
})
export class AppService {

  constructor(
    private authService: AuthService,
    private http: HttpClient
  ) { }


  updateAppToken (tokenApi: string, tokenPush: string, email: string) {
    /**
     * Busca en el storage el appId que identifica a esta app
     * Si no está, lo genera y lo envía al backend para asociar
     * appId, token e email
     */
    return from(Storage.get({key: "appIdData"})).pipe(
      switchMap(async storedData => {
        let parsedData: {appId: string};

        if (!storedData || !storedData.value) {
          // No existe el appId en el storage, se lo genera
          parsedData = {appId: uuidv4()};

          await Storage.set({
            key: "appIdData",
            value: JSON.stringify(parsedData)
          });
        }
        else {
          parsedData = JSON.parse(storedData.value) as {appId: string}
        }

        console.log("appId: " + parsedData.appId);
        console.log("token: " + tokenPush);
        console.log("email: " + email);

        return this.http.post(environment.api_url + "/app", {
          appId: parsedData.appId,
          token: tokenPush,
          email: email
        }, {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${tokenApi}`
          })
        })
      })
    );
  }
}
