import { Component, OnInit } from '@angular/core';
import { FormGroup, FormControl, Validators } from '@angular/forms';
import { Router } from '@angular/router';
import { LoadingController, AlertController } from '@ionic/angular';
import { AuthService } from '../../auth.service';

@Component({
  selector: 'app-recover-pin',
  templateUrl: './recover-pin.page.html',
  styleUrls: ['./recover-pin.page.scss'],
})
export class RecoverPinPage implements OnInit {
  form: FormGroup;

  constructor(
    private loadingController: LoadingController, 
    private alertController: AlertController, 
    private router: Router,
    private authService: AuthService) { }
    

  ngOnInit() {
    this.form = new FormGroup({
      email: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required, Validators.email]
      }),
      pin: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required]
      }),
      password: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required, Validators.minLength(6)]
      }),
      repeatPassword: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required, Validators.minLength(6)]
      })
    })
  }


  async onRecoverPassword() {

    if (this.form.value.password !== this.form.value.repeatPassword) {
      await this.showAlert("Error!", "Las contraseñas ingresadas no coinciden.");
    }
    else {
      // Se hace un request al backend para completar el proceso de recuperación de la contraseña
      const loading = await this.loadingController.create({
        keyboardClose: true,
        message: "Iniciando la recuperación..."
      });
  
      loading.present();

      this.authService.recoverPasswordPin(this.form.value.email, this.form.value.password, this.form.value.pin)
        .subscribe(
          async () => {
            loading.dismiss();
  
            await this.showAlert("Recuperación", "La contraseña fue modificada con éxito");
    
            this.form.reset();
            this.router.navigate(['/login']);
          },
          async () => {
            loading.dismiss();
  
            await this.showAlert("Error!", "E-mail o pin incorrectos");
          }
        )
    }
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
