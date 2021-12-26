import { Component, OnInit } from '@angular/core';
import { FormControl, FormGroup, Validators } from '@angular/forms';
import { Router } from '@angular/router';
import { AlertController, LoadingController } from '@ionic/angular';
import { AuthService } from '../auth.service';

@Component({
  selector: 'app-create-account',
  templateUrl: './create-account.page.html',
  styleUrls: ['./create-account.page.scss'],
})
export class CreateAccountPage implements OnInit {
  form: FormGroup;

  constructor(
    private router: Router, 
    private alertController: AlertController,
    private loadingController: LoadingController,
    private authService: AuthService) { }

  ngOnInit() {
    this.form = new FormGroup({
      name: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required]
      }),
      email: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required, Validators.email]
      }),
      password: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required, Validators.minLength(6)]
      }),
      repeatPassword: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required, Validators.minLength(6)]
      })
    });
  }


  async onCreateAccount() {

    if (this.form.value.password !== this.form.value.repeatPassword) {
      await this.showAlert("Error!", "Las contraseñas ingresadas no coinciden.");
    }
    else {
      const loading = await this.loadingController.create({
        keyboardClose: true,
        message: "Creando la cuenta..."
      });

      loading.present();

      // Request al backend para crear la cuenta
      this.authService.createAccount(this.form.value.email, this.form.value.password)
        .subscribe(
          async () =>{
            // Se creó la cuenta
            loading.dismiss();
    
            await this.showAlert("Cuenta creada", "Te envíamos un e-mail para confirmar tu dirección de correo electrónico.");

            this.form.reset();
            this.router.navigate(['/login']);
          },
          async () => {
            loading.dismiss();
            await this.showAlert("Error!", "Ya existe una cuenta asociada a ese e-mail.");
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
