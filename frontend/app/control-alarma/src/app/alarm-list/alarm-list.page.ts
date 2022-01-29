import { Component, OnDestroy, OnInit } from '@angular/core';
import { switchMap, take } from 'rxjs/operators';
import { AuthService } from '../login/auth.service';
import { DeviceService } from './device.service';
import { Device } from '../models/device.model';
import { ActionSheetController, LoadingController, NavController, AlertController, ModalController } from '@ionic/angular';
import { MqttService } from '../services/mqtt.service';
import { NavigationExtras } from '@angular/router';
import { YesNoModalPage } from '../yes-no-modal/yes-no-modal.page';
import { QrModalPage } from './qr-modal/qr-modal.page';

@Component({
  selector: 'app-alarm-list',
  templateUrl: './alarm-list.page.html',
  styleUrls: ['./alarm-list.page.scss'],
})
export class AlarmListPage implements OnInit, OnDestroy {

  deviceList: Device[];

  constructor(
    private loadingController: LoadingController,
    private authService: AuthService,
    private deviceService: DeviceService,
    private actionSheetController: ActionSheetController,
    private navigationController: NavController,
    private mqttService: MqttService,
    private alertController: AlertController,
    private modalController: ModalController
  ) { }

  async ngOnInit() {
    // Request al backend de la lista de dispositivos
    const loading = await this.loadingController.create({
      keyboardClose: true,
      message: "Buscando dispositivos..."
    });

    loading.present();

    this.authService.userEmail.pipe (
      switchMap(email => {
        return this.deviceService.getDevices(email);
      })
    ).subscribe(() => loading.dismiss());

    this.deviceService.deviceList.subscribe(deviceList => {
      this.deviceList = deviceList;

      // Se suscribe a las novedades de cada comunicador que puedan venir desde el broker MQTT
      this.deviceList.forEach(device => {
        this.mqttService.subscribeToDeviceNews(device.comId);
      })
    });
  }

  ngOnDestroy(): void {
    console.log("onDestroy");

    // Se desuscribe a las novedades de cada comunicador que puedan venir desde el broker MQTT
    this.deviceList.forEach(device => {
      this.mqttService.unsubscribeToDeviceNews(device.comId);
    })
  }


  async onItemClicked(device: Device) {
    if (device.online)
      this.navigationController.navigateForward(['alarm-detail', device.comId, 1], {animated: true});
    else {
      const alert = await this.alertController.create ({
        header: "Comunicador offline",
        message: "El comunicador de tu alarma no se encuentra conectado",
        buttons: ["OK"],
        keyboardClose: true,
        mode: 'ios'
      });
  
      await alert.present();
    }
  }

  onAlarmMore(device: Device, event: Event) {
    this.showAlarmMoreActionSheet(device);

    // Se evita que se propague el evento de click a la card
    event.stopPropagation();
    return false;
  }

  private async showAlarmMoreActionSheet(device: Device) {
    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      mode: 'ios',
      buttons: [
        {
          text: "Configurar comunicador",
          cssClass: 'custom-action-sheet',
          handler: () => {
            let navExtras: NavigationExtras = {
              state: {
                icon: device.icono,
                name: device.nombre,
                timeZone: device.codigoRegion
              }
            };

            this.navigationController.navigateForward(['alarm-list', 'config-device', device.comId], navExtras);
          }
        },
        {
          text: "Particiones",
          cssClass: 'custom-action-sheet',
          handler: () => {
            this.navigationController.navigateForward(['partitions', device.comId], {animated: true});
          }
        },
        {
          text: "Configurar red Wi-Fi",
          cssClass: 'custom-action-sheet',
          handler: () => {
            this.navigationController.navigateForward(['alarm-list', 'config-wifi']);
          }
        },
        {
          text: "Mostar QR",
          cssClass: 'custom-action-sheet',
          handler: async () => {
            const modal = await this.modalController.create({
              component: QrModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "comId": device.comId
              }
            });

            await modal.present();
          }
        },
        {
          text: "Desvincular comunicador",
          cssClass: 'custom-action-sheet',
          handler: async () => {
            this.actionSheetController.dismiss();

            const modal = await this.modalController.create({
              component: YesNoModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "message": `¿Deseás desvinuclar el comunicador ${device.nombre}?`,
                "yesText": "Desvincular",
                "noText": "Cancelar"
              }
            });
        
            modal.present();
        
            const {data} = await modal.onWillDismiss()
            
            if (data && data.result === "yes") {
              // Se desvincula el comunicador y se vuelven a pedir los comunicadores asociados al email
              const loading = await this.loadingController.create({
                keyboardClose: true,
                message: "Desvinculando"
              });
          
              loading.present();

              let userEmail;

              this.authService.userEmail.pipe(
                take(1),
                switchMap(email => {
                  userEmail = email;

                  return this.deviceService.unlinkUserAndDevice(
                    userEmail,
                    device.comId
                  );
                })
              ).subscribe(
                () => {
                  this.deviceService.getDevices(userEmail).subscribe(() => loading.dismiss());
                },
                () => {
                  console.log("error al desvincular el comunicador");
                  loading.dismiss();
                }
              );
              
            }
          }
        }
      ]
    });

    await actionSheet.present();
  }
}
