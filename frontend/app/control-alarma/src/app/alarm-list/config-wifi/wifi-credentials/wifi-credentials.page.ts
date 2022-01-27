import { Component, OnInit } from '@angular/core';
import { FormGroup, FormControl, Validators } from '@angular/forms';
import * as Socket from '@vendus/sockets-for-cordova';

@Component({
  selector: 'app-wifi-credentials',
  templateUrl: './wifi-credentials.page.html',
  styleUrls: ['./wifi-credentials.page.scss'],
})
export class WifiCredentialsPage implements OnInit {

  form: FormGroup;
  showPassword = false;
  passwordToggleIcon = "eye";

  security = [
    {code: 0, text: "Sin seguridad"},
    {code: 1, text: "WEP"},
    {code: 2, text: "WPA/WPA2 Personal"}
  ];

  constructor() { }

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

  onConfigure() {
    let dataToSend = `${this.form.value.ssid}&${this.form.value.password}&${this.form.value.security}\0`;

    let socket =new Socket();

    socket.onData = (data) => {
      // Se recibió data en el socket
      let dataString = this.arrayBuffer2str(data);
      console.log("Data received: " + dataString);

      if (dataString === dataToSend) {
        console.log("todo igual!");
      }
    };

    socket.onError = (errorMessage) => {
      // Hubo un error
      console.log("error: " + errorMessage);
    };

    socket.onClose = (hasError) => {
      // Se cerró la conexión
      console.log("se cerró");
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
}
