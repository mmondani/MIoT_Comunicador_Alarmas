import { Component, OnInit } from '@angular/core';
import { FormGroup, FormControl, Validators } from '@angular/forms';
import { DeviceService } from '../../device.service';
import { AuthService } from '../../../login/auth.service';
import { AlertController, LoadingController, NavController } from '@ionic/angular';
import { take, switchMap } from 'rxjs/operators';
import { ActivatedRoute } from '@angular/router';

@Component({
  selector: 'app-ask-code',
  templateUrl: './ask-code.page.html',
  styleUrls: ['./ask-code.page.scss'],
})
export class AskCodePage implements OnInit {

  private deviceId;
  form: FormGroup;

  constructor(
    private authService: AuthService,
    private deviceService: DeviceService,
    private alertController: AlertController,
    private loadingController: LoadingController,
    private navController: NavController,
    private activatedRoute: ActivatedRoute
  ) { }

  ngOnInit() {
    this.deviceId = this.activatedRoute.snapshot.params["id"];

    this.form = new FormGroup({
      deviceCode: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required]
      })
    })
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
          this.deviceId,
          this.form.value.deviceCode
        );
      })
    ).subscribe(() =>{
      /**
       * Se pudo linkear el usuario y el dispositivo
       * Se piden todos los dispositivos asociados al usuario
       */
      loading.dismiss();

      this.deviceService.getDevices(userEmail).subscribe(deviceList => {
        this.navController.navigateForward(['alarm-list'], {animated: true});
      })
    }, async (error) => {
      /**
       * No se pudo linkear el usuario y el disposivo.
       * Se determina si es porque se necesita una clave o es otro motivo
       */
      let errorCause = error.error.error;

      loading.dismiss();

      const alert = await this.alertController.create ({
        header: "Agregar comunicador",
        message: errorCause,
        buttons: ["OK"],
        keyboardClose: true,
        mode: 'ios'
      });
  
      await alert.present();
    })
  }
}
