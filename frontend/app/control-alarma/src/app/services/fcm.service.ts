import { Injectable } from '@angular/core';
import { Capacitor } from '@capacitor/core';
import { PushNotifications } from '@capacitor/push-notifications';
import { AppService } from './app.service';
import { AuthService } from '../login/auth.service';
import { take } from 'rxjs/operators';
import { Storage } from '@capacitor/storage';
import { from } from 'rxjs';
import { v4 as uuidv4 } from 'uuid';

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
            /**
             * Busca en el storage el appId que identifica a esta app
             * Si no está, lo genera y lo envía al backend para asociar
             * appId, token e email
             */
            from(Storage.get({key: "appIdData"})).subscribe(async storedData => {
              let parsedData: {appId: string};

              if (!storedData || !storedData.value) {
                // No existe el appId en el storage, se lo genera
                parsedData = {appId: uuidv4()};
  
                await Storage.set({
                  key: "appIdData",
                  value: JSON.stringify(parsedData)
                });
              }
              else {
                parsedData = JSON.parse(storedData.value) as {appId: string}
              }

              this.appService.updateAppToken(token, tokenPush.value, parsedData.appId, email).subscribe(
                () => {console.log("ok update token")},
                (error) => {console.log("error update token: " + error)}
              );
            });
          });
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
