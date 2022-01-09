import { HttpClient, HttpHeaders } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Device } from '../models/device.model';
import { environment } from '../../environments/environment';
import { BehaviorSubject, partition } from 'rxjs';
import { filter, map, switchMap, take, tap } from 'rxjs/operators';
import { AuthService } from '../login/auth.service';
import { Particion } from '../models/particion.model';
import { Zona } from '../models/zona.model';
import { Nodo } from '../models/nodo.model';

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

    
    /**
     * Cuando se configura el device actual, se emite ese device
     * en el subject currentDevice
     */
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

  get currentDeviceComId() {
    return this._currentDeviceComId;
  }

  set currentPartitionNumber(partitionNumber: number) {
    this._currentPartitionNumber = partitionNumber;

    /**
     * Cuando se configura la prtición actual, se emite esa partición
     * en el subject currentDevice
     */
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
      take(1),
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

  getDevice (comId: string) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.get<Device>(environment.api_url + "/device/id/" + comId, {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        })
      })
    ).pipe(
      tap(device => {
        /**
         * Si es el dispositivo actual se emite el dispositivo y la partición actual,
         * para actualizar la UI
         */
        if (device.comId === this._currentDeviceComId) {
          // Se actualiza el registro local de los dispositivos
          this.deviceList.pipe(
            take(1)
          ).subscribe(deviceList => {
            deviceList.forEach( (dev, i) => {
              if (dev.comId === device.comId) {
                deviceList[i] = device;
              }
            });

            this._deviceList.next(deviceList);
          })

          // Se emite la versión más nueva del device para actualizar la UI
          this._currentDevice.next(device);

          // Se emite la partición de la versión más nueva del device para actualiza la UI
          device.particiones.forEach(particion =>{
            if (particion.numero === this._currentPartitionNumber)
              this._currentPartition.next(particion);
          })
        }
        
      })
    );
  }

  newZone (comId: string, partitionNumber: number, zoneNumber: number, zoneName: string, zoneIcon: string) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.post<Zona>(environment.api_url + "/device/zone", {
          comId: comId,
          particion: partitionNumber,
          numero: zoneNumber,
          nombre: zoneName,
          icono: zoneIcon
        }, {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        });
      }),
    );
  }


  updateZone (comId: string, partitionNumber: number, zoneNumber: number, zoneName?: string, zoneIcon?: string) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.patch<Zona>(environment.api_url + "/device/zone", {
          comId: comId,
          particion: partitionNumber,
          numero: zoneNumber,
          nombre: zoneName,
          icono: zoneIcon
        }, {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        });
      }),
    );
  }

  removeZone (comId: string, partitionNumber: number, zoneNumber: number) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.request('delete', environment.api_url + "/device/zone", {
          body: {
            comId: comId,
            particion: partitionNumber,
            numero: zoneNumber
          },
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        });
      })
    );
  }


  newNode (comId: string, partitionNumber: number, nodeNumber: number, nodeName: string, nodeIcon: string) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.post<Nodo>(environment.api_url + "/device/node", {
          comId: comId,
          particion: partitionNumber,
          numero: nodeNumber,
          nombre: nodeName,
          icono: nodeIcon
        }, {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        });
      }),
    );
  }
  

  updateNode (comId: string, partitionNumber: number, nodeNumber: number, nodeName?: string, nodeIcon?: string) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.patch<Nodo>(environment.api_url + "/device/node", {
          comId: comId,
          particion: partitionNumber,
          numero: nodeNumber,
          nombre: nodeName,
          icono: nodeIcon
        }, {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        });
      }),
    );
  }

  removeNode (comId: string, partitionNumber: number, nodeNumber: number) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.request('delete', environment.api_url + "/device/node", {
          body: {
            comId: comId,
            particion: partitionNumber,
            numero: nodeNumber
          },
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        });
      })
    );
  }


  newAlarmUser (comId: string, partitionNumber: number, userNumber: number, userName: string) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.post<Nodo>(environment.api_url + "/device/alarm-user", {
          comId: comId,
          particion: partitionNumber,
          numero: userNumber,
          nombre: userName
        }, {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        });
      }),
    );
  }

  updateAlarmUser (comId: string, partitionNumber: number, userNumber: number, userName?: string) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.patch<Nodo>(environment.api_url + "/device/alarm-user", {
          comId: comId,
          particion: partitionNumber,
          numero: userNumber,
          nombre: userName
        }, {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        });
      }),
    );
  }

  removeAlarmUser (comId: string, partitionNumber: number, userNumber: number) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.request('delete', environment.api_url + "/device/alarm-user", {
          body: {
            comId: comId,
            particion: partitionNumber,
            numero: userNumber
          },
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        });
      })
    );
  }
}
