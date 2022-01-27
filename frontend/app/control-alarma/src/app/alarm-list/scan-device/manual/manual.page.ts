import { Component, OnInit } from '@angular/core';
import { FormGroup, FormControl, Validators } from '@angular/forms';
import { DeviceService } from '../../device.service';
import { AuthService } from '../../../login/auth.service';
import { take, switchMap } from 'rxjs/operators';
import { AlertController, LoadingController, NavController } from '@ionic/angular';

@Component({
  selector: 'app-manual',
  templateUrl: './manual.page.html',
  styleUrls: ['./manual.page.scss'],
})
export class ManualPage implements OnInit {

  form: FormGroup;

  constructor(
    private authService: AuthService,
    private deviceService: DeviceService,
    private alertController: AlertController,
    private loadingController: LoadingController,
    private navController: NavController
  ) { }

  ngOnInit() {
    this.form = new FormGroup({
      deviceId: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required, Validators.minLength(8), Validators.maxLength(8)]
      })
    });
  }


  async onAddDevice() {
    let userEmail;

    const loading = await this.loadingController.create({
      keyboardClose: true,
      message: "Agregando"
    });

    loading.present();

    this.authService.userEmail.pipe(
      take(1),
      switchMap(email => {
        userEmail = email;
        
        return this.deviceService.linkUserAndDevice(
          email,
          this.form.value.deviceId
        );
      })
    ).subscribe(() => {
      /**
       * Se pudo linkear el usuario y el dispositivo
       * Se piden todos los dispositivos asociados al usuario
       */
      loading.dismiss();

      this.deviceService.getDevices(userEmail).subscribe(deviceList => {
        this.navController.navigateForward(['alarm-list', 'scan-device', 'config-device', this.form.value.deviceId], {animated: true});
      })
    }, async (error) => {
      /**
       * No se pudo linkear el usuario y el disposivo.
       * Se determina si es porque se necesita una clave o es otro motivo
       */
      let errorCause = error.error.error;

      loading.dismiss();

      if (errorCause === "Clave requerida") {
        // Se necesita una clave para linkear. Se va a la pantalla para ingresar la clave
        this.navController.navigateForward(['alarm-list', 'scan-device', 'ask-code', this.form.value.deviceId], {animated: true});
      }
      else {
        const alert = await this.alertController.create ({
          header: "Agregar comunicador",
          message: errorCause,
          buttons: ["OK"],
          keyboardClose: true,
          mode: 'ios'
        });
    
        await alert.present();
      }
    })

  }
}
