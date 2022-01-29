import { Component, OnInit } from '@angular/core';
import { FormGroup, FormControl, Validators } from '@angular/forms';
import * as Socket from '@vendus/sockets-for-cordova';
import { AlertController, LoadingController, NavController } from '@ionic/angular';

@Component({
  selector: 'app-wifi-credentials',
  templateUrl: './wifi-credentials.page.html',
  styleUrls: ['./wifi-credentials.page.scss'],
})
export class WifiCredentialsPage implements OnInit {

  form: FormGroup;
  showPassword = false;
  passwordToggleIcon = "eye";
  private configTimeout;

  security = [
    {code: 0, text: "Sin seguridad"},
    {code: 1, text: "WEP"},
    {code: 2, text: "WPA/WPA2 Personal"}
  ];

  constructor(
    private alertController: AlertController,
    private loadingController: LoadingController,
    private navController: NavController
  ) { }

  ngOnInit() {
    this.form = new FormGroup({
      ssid: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required]
      }),
      password: new FormControl(null, {
        updateOn: 'change',
        validators: [Validators.required]
      }),
      security: new FormControl(2, {
        updateOn: 'change',
        validators: [Validators.required]
      })
    })
  }

  async onConfigure() {
    let dataToSend = `${this.form.value.ssid}&${this.form.value.password}&${this.form.value.security}\0`;

    let socket =new Socket();

    const loading = await this.loadingController.create({
      keyboardClose: true,
      message: "Configurando"
    });

    loading.present();

    this.configTimeout = setTimeout(async () => {
      loading.dismiss();
      
      await this.showAlert(
        "Error de configuración",
        "Ocurrió un error al intentar configurar la red Wi-Fi en tu comunicador. Volvé a intentarlo.");
    }, 10000);

    socket.onData = async (data) => {
      // Se recibió data en el socket
      let dataString = this.arrayBuffer2str(data);
      console.log("Data received: " + dataString);

      loading.dismiss();
      clearTimeout(this.configTimeout);

      if (dataString === dataToSend) {
        await this.showAlert(
          "Configuración exitosa",
          "Se logró configurar exitósamente la red Wi-Fi en tu comunicador");
      }
      else {
        await this.showAlert(
          "Error de configuración",
          "Ocurrió un error al intentar configurar la red Wi-Fi en tu comunicador. Volvé a intentarlo.");
      }
    };

    socket.onError = async (errorMessage) => {
      // Hubo un error
      loading.dismiss();
      clearTimeout(this.configTimeout);
      
      await this.showAlert(
        "Error de configuración",
        "Ocurrió un error al intentar configurar la red Wi-Fi en tu comunicador. Volvé a intentarlo.");
    };

    socket.onClose = async (hasError) => {
      // Se cerró la conexión
      loading.dismiss();
      clearTimeout(this.configTimeout);

      console.log("se cerró");

      await this.showAlert(
        "Error de configuración",
        "Ocurrió un error al intentar configurar la red Wi-Fi en tu comunicador. Volvé a intentarlo.");
    };

    socket.open("192.168.1.101", 6666, 
      () => {
        // Se pudo conectar
        console.log ("Se pudo conectar");

        var data = new Uint8Array(dataToSend.length);
        for (var i = 0; i < data.length; i++) {
          data[i] = dataToSend.charCodeAt(i);
        }
        socket.write(data, () =>{}, () =>{});
      }, 
      () => {
        // No se pudo conectar
        console.log ("No se pudo conectar");
      });

  }

  onTogglePassword() {
    this.showPassword = !this.showPassword;

    if (this.passwordToggleIcon === "eye")
      this.passwordToggleIcon = "eye-off";
    else
      this.passwordToggleIcon = "eye";
  }


  private arrayBuffer2str(buf) {
		var str= '';
		var ui8= new Uint8Array(buf);
		for (var i= 0 ; i < ui8.length ; i++) {
			str= str+String.fromCharCode(ui8[i]);
		}
		return str;
	}

  private async showAlert (tilte: string, message: string) {
    const alert = await this.alertController.create ({
      header: tilte,
      message: message,
      buttons: ["OK"],
      keyboardClose: true,
      mode: 'ios'
    });

    await alert.present();

    this.navController.navigateForward(["/"], {animated: true});
  }
}
