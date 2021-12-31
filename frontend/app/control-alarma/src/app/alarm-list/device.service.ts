import { HttpClient, HttpHeaders } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Device } from '../models/device.model';
import { environment } from '../../environments/environment';
import { BehaviorSubject } from 'rxjs';
import { map, switchMap, tap } from 'rxjs/operators';
import { AuthService } from '../login/auth.service';

@Injectable({
  providedIn: 'root'
})
export class DeviceService {
  private _deviceList = new BehaviorSubject<Device[]> (null) ;


  constructor(
    private authService: AuthService,
    private http: HttpClient
  ) { }


  get deviceList() {
    return this._deviceList.asObservable().pipe(
      map(deviceList => {
        if(deviceList)
          return deviceList;
        else
          return [];
      })
    );
  }


  getDevices (email: string) {
    return this.authService.token.pipe(
      switchMap(token => {
        return this.http.get<Device[]>(environment.api_url + "/device/user/" + email, {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        })
      })
    ).pipe(
      tap(deviceList => {
        this._deviceList.next(deviceList)
      })
    );

  }
  
}