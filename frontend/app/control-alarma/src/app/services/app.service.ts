import { Injectable } from '@angular/core';
import { AuthService } from '../login/auth.service';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { Storage } from '@capacitor/storage';
import { from } from 'rxjs';
import { map, switchMap, tap } from 'rxjs/operators';
import { environment } from '../../environments/environment';


@Injectable({
  providedIn: 'root'
})
export class AppService {

  constructor(
    private authService: AuthService,
    private http: HttpClient
  ) { }


  updateAppToken (tokenApi: string, tokenPush: string, appId: string, email: string) {
    
    console.log("appId: " + appId);
    console.log("token: " + tokenPush);
    console.log("email: " + email);

    return this.http.post(environment.api_url + "/app", {
        appId: appId,
        token: tokenPush,
        email: email
      }, {
        headers: new HttpHeaders( {
          Authorization: `Bearer ${tokenApi}`
        })
    })
  }
}
