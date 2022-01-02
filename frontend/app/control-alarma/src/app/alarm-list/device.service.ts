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
  private _currentDevice = new BehaviorSubject<Device> (null);
  private _currentPartition = new BehaviorSubject<Particion> (null);
  private _currentDeviceComId: string;
  private _currentPartitionNumber: number;


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

  get currentDevice() {
    return this._currentDevice.asObservable().pipe(
      map(device => {
        if(device)
          return device;
        else
          return null;
      })
    );
  }

  get currentPartition() {
    return this._currentPartition.asObservable().pipe(
      map(partition => {
        if(partition)
          return partition;
        else
          return null;
      })
    );
  }

  set currentDeviceComId(comId: string) {
    this._currentDeviceComId = comId;

    
    this.deviceList.subscribe(deviceList => {
      let device;

      deviceList.forEach(dev => {
        if (dev.comId === comId)
          device = dev;
      })

      if (device)
        this._currentDevice.next(device);
    })
  }

  set currentPartitionNumber(partitionNumber: number) {
    this._currentPartitionNumber = partitionNumber;

    this.deviceList.subscribe(deviceList => {
      let partition;

      deviceList.forEach(dev => {
        if (dev.comId === this._currentDeviceComId) {
          dev.particiones.forEach(part => {
            if (part.numero === partitionNumber)
              partition = part;
          })
        }
      })

      if (partition)
        this._currentPartition.next(partition);
    })
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
