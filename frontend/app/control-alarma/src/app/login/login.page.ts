import { Component, OnInit } from '@angular/core';
import { FormGroup, FormControl, Validators } from '@angular/forms';
import { Router } from '@angular/router';
import { AlertController, LoadingController } from '@ionic/angular';
import { AuthService } from './auth.service';
import { FcmService } from '../services/fcm.service';

@Component({
  selector: 'app-login',
  templateUrl: './login.page.html',
  styleUrls: ['./login.page.scss'],
})
export class LoginPage implements OnInit {
  form: FormGroup;
  showPassword = false;
  passwordToggleIcon = "eye";


  constructor(
    private router: Router, 
    private alertController: AlertController,
    private loadingController: LoadingController,
    private authService: AuthService,
    private fcmService: FcmService) { }


  ngOnInit() {
    this.form = new FormGroup({
      email: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required, Validators.email]
      }),
      password: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required]
      })
    })
  }


  async onLogin() {
    // Request al backend para hacer el login
    const loading = await this.loadingController.create({
      keyboardClose: true,
      message: "Ingresando..."
    });

    loading.present();

    this.authService.login(this.form.value.email, this.form.value.password)
      .subscribe(
        responseData => {
          // Se pudo loguear
          loading.dismiss();
          this.form.reset();

          // Se registra el servicio de push notifications
          this.fcmService.initPush();

          this.router.navigate(['/alarm-list']);
        },
        errorData => {
          // No se pudo loguear
          loading.dismiss();

          this.showAlert("Error!", "E-mail o contraseña incorrectos");
          console.log(errorData);
        }
      );
  }


  onTogglePassword () {
    this.showPassword = !this.showPassword;

    if (this.passwordToggleIcon === "eye")
      this.passwordToggleIcon = "eye-off";
    else
      this.passwordToggleIcon = "eye";
  }


  private async showAlert (title: string, message: string) {
    const alert = await this.alertController.create ({
      header: title,
      message: message,
      buttons: ["OK"],
      keyboardClose: true,
      mode: 'ios'
    });

    await alert.present();
  }
}
