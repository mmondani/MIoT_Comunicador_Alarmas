import { Injectable } from '@angular/core';
import { Capacitor } from '@capacitor/core';
import { PushNotifications } from '@capacitor/push-notifications';
import { AppService } from './app.service';
import { AuthService } from '../login/auth.service';
import { take } from 'rxjs/operators';

@Injectable({
  providedIn: 'root'
})
export class FcmService {

  constructor(
    private appService: AppService,
    private authService: AuthService
  ) { }

  async initPush() {
    if (Capacitor.getPlatform() !== "web") {
      await this.registerPush();
    }
  }

  private async registerPush() {
    let permStatus = await PushNotifications.checkPermissions();

    if (permStatus.receive === 'prompt') {
      permStatus = await PushNotifications.requestPermissions();
    }

    if (permStatus.receive !== 'granted') {
      throw new Error('User denied permissions!');
    }

    await this.addListeners();

    await PushNotifications.register();
  }

  private async addListeners () {
    await PushNotifications.addListener('registration', tokenPush => {
      console.info('Registration token: ', tokenPush.value);

      // Se actualiza el token en la base de datos
      this.authService.token.pipe(take(1)).subscribe(token => {
          this.authService.userEmail.pipe(take(1)).subscribe(email => {
            this.appService.updateAppToken(token, tokenPush.value, email).subscribe(
              () => {console.log("ok update token")},
              (error) => {console.log("error update token: " + error)}
            );
          })
        });
    });
  
    await PushNotifications.addListener('registrationError', err => {
      console.error('Registration error: ', err.error);
    });
  
    await PushNotifications.addListener('pushNotificationReceived', notification => {
      console.log('Push notification received: ', notification);
    });
  
    await PushNotifications.addListener('pushNotificationActionPerformed', notification => {
      console.log('Push notification action performed', notification.actionId, notification.inputValue);
    });
  }
  
}
