import { Component, OnInit } from '@angular/core';
import { FormControl, FormGroup, Validators } from '@angular/forms';
import { Router } from '@angular/router';
import { AlertController } from '@ionic/angular';

@Component({
  selector: 'app-create-account',
  templateUrl: './create-account.page.html',
  styleUrls: ['./create-account.page.scss'],
})
export class CreateAccountPage implements OnInit {
  form: FormGroup;

  constructor(private router: Router, private alertController: AlertController) { }

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
    console.log(this.form)

    if (this.form.value.password !== this.form.value.repeatPassword) {
      await this.showAlert();
    }
    else {
      // Crear cuenta

      this.form.reset();
      this.router.navigate(['/login']);
    }

    
  }

  private async showAlert () {
    const alert = await this.alertController.create ({
      header: "Error!",
      message: "Las contraseñas ingresadas no coinciden",
      buttons: ["OK"],
      keyboardClose: true,
      mode: 'ios'
    });

    await alert.present();
  }
}
