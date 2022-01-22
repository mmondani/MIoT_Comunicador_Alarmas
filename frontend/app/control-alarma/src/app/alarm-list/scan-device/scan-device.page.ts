import { Component, OnInit } from '@angular/core';
import { BarcodeScanner } from '@capacitor-community/barcode-scanner';
import { AlertController, LoadingController, NavController } from '@ionic/angular';
import { AuthService } from '../../login/auth.service';
import { DeviceService } from '../device.service';
import { take, switchMap } from 'rxjs/operators';

@Component({
  selector: 'app-scan-device',
  templateUrl: './scan-device.page.html',
  styleUrls: ['./scan-device.page.scss'],
})
export class ScanDevicePage implements OnInit {
  scanActive: boolean = false;

  constructor(
    private authService: AuthService,
    private deviceService: DeviceService,
    private alertController: AlertController,
    private loadingController: LoadingController,
    private navController: NavController
  ) { }

  ngOnInit() {
    this.startScanner();
  }

  ionViewWillLeave() {
    BarcodeScanner.stopScan();
    this.scanActive = false;
  }

  async checkPermission() {
    return new Promise(async (resolve, reject) => {
      const status = await BarcodeScanner.checkPermission({ force: true });
      if (status.granted) {
        resolve(true);
      } else if (status.denied) {
        BarcodeScanner.openAppSettings();
        resolve(false);
      }
    });
  }

  async startScanner() {
    const allowed = await this.checkPermission();

    if (allowed) {
      this.scanActive = true;
      BarcodeScanner.hideBackground();

      const result = await BarcodeScanner.startScan();

      if (result.hasContent) {
        let userEmail;
        
        this.scanActive = false;
        
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
              result.content
            );
          })
        ).subscribe(() => {
          /**
           * Se pudo linkear el usuario y el dispositivo
           * Se piden todos los dispositivos asociados al usuario
           */
          loading.dismiss();
    
          this.deviceService.getDevices(userEmail).subscribe(deviceList => {
            this.navController.navigateForward(['alarm-list', 'config-device', result.content], {animated: true});
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
            this.navController.navigateForward(['alarm-list', 'scan-device', 'ask-code', result.content], {animated: true});
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
    
      } else {
        const alert = await this.alertController.create ({
          header: "Agregar comunicador",
          message: "No se pudo leer el código QR",
          buttons: ["OK"],
          keyboardClose: true,
          mode: 'ios'
        });
    
        await alert.present();
      }
    } else {
      const alert = await this.alertController.create ({
        header: "Agregar comunicador",
        message: "No se permitió usar la cámara",
        buttons: ["OK"],
        keyboardClose: true,
        mode: 'ios'
      });
  
      await alert.present();
    }
  }

  stopScanner() {
    BarcodeScanner.stopScan();
    this.scanActive = false;
  }

}
