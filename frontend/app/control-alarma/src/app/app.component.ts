import { Component, OnDestroy, OnInit } from '@angular/core';
import { Subscription } from 'rxjs';
import { AuthService } from './login/auth.service';
import { Router } from '@angular/router';
import { App, AppState } from '@capacitor/app'
import { take } from 'rxjs/operators';

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
    private router: Router
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
    App.addListener('appStateChange', this.checkAuthOnResume.bind(this))
  } 

  ngOnDestroy(): void {
    if (this.authSubcription)
      this.authSubcription.unsubscribe();
  }

  private checkAuthOnResume(state: AppState) {
    if (state.isActive) {
      this.authService
        .autoLogin()
        .pipe(take(1))
        .subscribe(success => {
          if (!success) {
            console.log("Re-loguear!!!");
            this.authService.logout();
          }
        });
    }
  }
}
