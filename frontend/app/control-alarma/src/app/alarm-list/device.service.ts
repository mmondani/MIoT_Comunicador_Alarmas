import { HttpClient, HttpHeaders } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Device } from '../models/device.model';
import { environment } from '../../environments/environment';
import { BehaviorSubject } from 'rxjs';
import { filter, map, switchMap, take, tap } from 'rxjs/operators';
import { AuthService } from '../login/auth.service';
import { Particion } from '../models/particion.model';

@Injectable({
  providedIn: 'root'
})
export class DeviceService {
  private _deviceList = new BehaviorSubject<Device[]> (null) ;
  private _currentDevice: Device;
  private _currentPartition: Particion;


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

  getDeviceItem(comId: string) {
    return this._deviceList.asObservable().pipe(
      map(deviceList => {
        let ret: Device;

        if (!deviceList)
          ret = null;

        deviceList.forEach(device => {
          if (device.comId === comId)
            ret = device;
        })

        return ret;
      })
    )
  }

  getDeviceName(comId: string) {
    return this._deviceList.asObservable().pipe(
      map(deviceList => {
        let ret: string;

        if (!deviceList)
          ret = "";

        deviceList.forEach(device => {
          if (device.comId === comId)
            ret = device.nombre;
        })

        return ret;
      })
    )
  }

  getDevicePartitionNames(comId: string) {
    return this._deviceList.asObservable().pipe(
      map(deviceList => {
        let ret: string[] = [];

        if (!deviceList)
          ret = [];

        deviceList.forEach(device => {
          if (device.comId === comId) {
            device.particiones.forEach(particion => {
              ret.push(particion.nombre);
            })
          }
        })

        return ret;
      })
    )
  }

  getDevicePartitionName(comId: string, partitionNumber: number) {
    return this._deviceList.asObservable().pipe(
      map(deviceList => {
        let ret: string = "";

        if (!deviceList)
          ret = "";

        deviceList.forEach(device => {
          if (device.comId === comId) {
            device.particiones.forEach(particion => {
              if (particion.numero == partitionNumber) {
                ret = particion.nombre;
              }
            })
          }
        })

        return ret;
      })
    )
  }


  getDevicePartitionNumbers(comId: string) {
    return this._deviceList.asObservable().pipe(
      map(deviceList => {
        let ret: number[] = [];

        if (!deviceList)
          ret = [];

        deviceList.forEach(device => {
          if (device.comId === comId) {
            device.particiones.forEach(particion => {
              ret.push(particion.numero);
            })
          }
        })

        return ret;
      })
    )
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
