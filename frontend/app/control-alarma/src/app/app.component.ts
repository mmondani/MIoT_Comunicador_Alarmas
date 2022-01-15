import { Component, OnDestroy, OnInit } from '@angular/core';
import { Subscription } from 'rxjs';
import { AuthService } from './login/auth.service';
import { Router } from '@angular/router';
import { App, AppState } from '@capacitor/app'
import { switchMap, take } from 'rxjs/operators';
import { LoadingController } from '@ionic/angular';
import { DeviceService } from './alarm-list/device.service';

@Component({
  selector: 'app-root',
  templateUrl: 'app.component.html',
  styleUrls: ['app.component.scss'],
})
export class AppComponent implements OnInit, OnDestroy{
  private authSubcription: Subscription;
  private previousAuthState = false;

  constructor(
    private authService: AuthService,
    private router: Router,
    private loadingController: LoadingController,
    private deviceService: DeviceService
  ) {}


  ngOnInit(): void {
    // Detecto cuando el login ya no es válido
    this.authSubcription = this.authService.userIsAuthenticated.subscribe(isAuthenticated => {
      if (!isAuthenticated && this.previousAuthState !== isAuthenticated) {
        console.log("Re-loguear!!!");
        this.router.navigate(['/login']);
      }

      this.previousAuthState = isAuthenticated;
    });

    // Se agrega un listener para chequear si hay que reloguear cuando se hace
    // un resume de la app
    App.addListener('appStateChange', this.checkAuthOnResume.bind(this));

    //App.addListener('appStateChange', this.getDevicesListOnResume.bind(this));

    //this.requestDevices();
  } 

  ngOnDestroy(): void {
    if (this.authSubcription)
      this.authSubcription.unsubscribe();
  }

  onLogout() {
    this.authService.logout();
  }

  private checkAuthOnResume(state: AppState) {
    console.log("resume 1");
    if (state.isActive) {
      this.authService
        .autoLogin()
        .pipe(take(1))
        .subscribe(async success => {
          if (!success) {
            console.log("Re-loguear!!!");
            this.authService.logout();
          }
          else {
            this.requestDevices();
          }
        });

      
    }
  }

  /*private getDevicesListOnResume () {
    console.log("resume 2");
  }*/

  private async requestDevices() {
    // Request al backend de la lista de dispositivos
    let email = await this.authService.userEmail.pipe (
        take(1)
      ).toPromise();

    if (email) {
      const loading = await this.loadingController.create({
        keyboardClose: true,
        message: "Buscando dispositivos..."
      });
  
      loading.present();
  
      this.authService.userEmail.pipe (
        take(1),
        switchMap(email => {
          console.log("solicitando devices de: " + email);
          return this.deviceService.getDevices(email);
        })
      ).subscribe(() => loading.dismiss());
    }
  }
}
