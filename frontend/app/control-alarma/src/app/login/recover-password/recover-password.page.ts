import { Component, OnInit } from '@angular/core';
import { FormGroup, FormControl, Validators } from '@angular/forms';
import { Router } from '@angular/router';
import { AuthService } from '../auth.service';
import { AlertController, LoadingController } from '@ionic/angular';

@Component({
  selector: 'app-recover-password',
  templateUrl: './recover-password.page.html',
  styleUrls: ['./recover-password.page.scss'],
})
export class RecoverPasswordPage implements OnInit {
  form: FormGroup;

  constructor(
    private router: Router,
    private authService: AuthService,
    private alertController: AlertController,
    private loadingController: LoadingController) { }

  ngOnInit() {
    this.form = new FormGroup({
      email: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required, Validators.email]
      })
    })
  }


  async onForgotPassword() {
    // Se hace el request al backend para iniciar la recuperación de contraseña
    const loading = await this.loadingController.create({
      keyboardClose: true,
      message: "Iniciando la recuperación..."
    });

    loading.present();

    this.authService.recoverPassword(this.form.value.email)
      .subscribe(
        () => {
          loading.dismiss();
          this.form.reset();
          this.router.navigate(['/login/recover-password/recover-pin']);
        },
        () => {
          loading.dismiss();
          this.showAlert("Error!", "Se produjo un error al intentar recuperar la contraseña.");
          this.form.reset();
        }
      );
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
