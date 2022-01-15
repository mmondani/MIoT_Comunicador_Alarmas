import { Injectable } from '@angular/core';
import { AuthService } from '../login/auth.service';
import { Modo, Estado, CausaDisparo } from '../models/particion.model';
import { switchMap, take } from 'rxjs/operators';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { environment } from '../../environments/environment';
import { partition } from 'rxjs';


@Injectable({
  providedIn: 'root'
})
export class CommandsService {

  constructor(
    private authService: AuthService,
    private http: HttpClient
  ) { }

  armAlarm (comId: string, partition: number, state: Estado, code: string) {

  }

  disarmAlarm (comId: string, partition: number, state: Estado, code: string) {
    
  }

  changeMode (comId: string, partition: number, mode: Modo) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.post(environment.api_url + "/command/mode", {
          comId: comId,
          particion: partition,
          modo: mode
        },
        {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        })
      })
    );
  }

  trigger (comId: string, partition: number, cause: CausaDisparo) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.post(environment.api_url + "/command/trigger", {
          comId: comId,
          particion: partition,
          causa: cause
        },
        {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        })
      })
    );
  }

  includeExcludeZones (comId: string, partition: number, zones:{zona:number, excluir: boolean}[]) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.post(environment.api_url + "/command/exclude", {
          comId: comId,
          particion: partition,
          zonas: zones
        },
        {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        })
      })
    );
  }

  manageNodes (comId: string, partition: number, nodes: {nodo: number, encender: boolean}[]) {
    return this.authService.token.pipe(
      take(1),
      switchMap(token => {
        return this.http.post(environment.api_url + "/command/manage-nodes", {
          comId: comId,
          particion: partition,
          nodos: nodes
        },
        {
          headers: new HttpHeaders( {
            Authorization: `Bearer ${token}`
          })
        })
      })
    );
  }
}
