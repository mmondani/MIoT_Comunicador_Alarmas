import { Injectable } from '@angular/core';
import { CanLoad, Route, Router, UrlSegment, UrlTree } from '@angular/router';
import { Storage } from '@capacitor/storage';
import { from, Observable, of } from 'rxjs';
import { map, switchMap, take, tap } from 'rxjs/operators';
import { AuthService } from './auth.service';
import { FcmService } from '../services/fcm.service';

@Injectable({
  providedIn: 'root'
})
export class AuthGuard implements CanLoad {

  constructor(
    private authService: AuthService, 
    private router: Router,
    private fcmService: FcmService
  ) {}

  canLoad(
    route: Route, 
    segments: UrlSegment[]
  ): boolean | UrlTree | Observable<boolean | UrlTree> | Promise<boolean | UrlTree> {

      return this.authService.userIsAuthenticated.pipe(
        take(1), 
        switchMap(isAuthenticated => {
          // Si no está autenticado, trata de levantar la información de un login
          // anterior guardado en la memoria
          if (!isAuthenticated) {
            return this.authService.autoLogin();
          } 
          
          return of(isAuthenticated);
        }),
        switchMap(isAuthenticated => {
          if (!isAuthenticated)
            return from(this.loginAgain());

          return of(isAuthenticated);
        }),
        tap(isAuthenticated => {
          if (!isAuthenticated)
            this.router.navigate(['/login']);
        })
      );
  }

  private loginAgain() {

    return from(Storage.get({key: "user"})).pipe(
      take(1),
      map(storedData => {
        let user;

        if (!storedData || !storedData.value)
          return null
        else
          user = JSON.parse(storedData.value);

        return user;
      }),
      switchMap(user => {
        if (!user)
          return of(false);

        return this.authService.login(user.email, user.password)
      }),
      tap(responseData => {
          if (!responseData) {
            // No se pudo loguear
            console.log("no se pudo reloguear 1");

            this.authService.logout();

            return;
          }

          // Se pudo re-loguear
          console.log("se pudo reloguear");

          // Se registra el servicio de push notifications
          this.fcmService.initPush();
        },
        errorData => {
          // No se pudo loguear
          console.log("no se pudo reloguear 2");

          this.authService.logout();
        }
      ),
      map(responseData => {
        if (!responseData)
          return false;

        return true;
      })
    )
  }
}
