import { Component, OnDestroy, OnInit } from '@angular/core';
import { switchMap } from 'rxjs/operators';
import { AuthService } from '../login/auth.service';
import { DeviceService } from './device.service';
import { Device } from '../models/device.model';
import { ActionSheetController, LoadingController, NavController, AlertController } from '@ionic/angular';
import { MqttService } from '../services/mqtt.service';

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
    private alertController: AlertController
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

  onAlarmMore(comId: string, event: Event) {
    this.showAlarmMoreActionSheet(comId);

    // Se evita que se propague el evento de click a la card
    event.stopPropagation();
    return false;
  }

  private async showAlarmMoreActionSheet(comId: string) {
    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      mode: 'ios',
      buttons: [
        {
          text: "Configurar comunicador",
          handler: () => {
            console.log("configurar comunicador");
          }
        },
        {
          text: "Particiones",
          handler: () => {
            this.navigationController.navigateForward(['partitions', comId], {animated: true});
          }
        },
        {
          text: "Configurar red Wi-Fi",
          handler: () => {
            console.log("configurar wifi");
          }
        },
        {
          text: "Mostar QR",
          handler: () => {
            console.log("mostrar QR");
          }
        },
        {
          text: "Desvincular comunicador",
          handler: () => {
            console.log("desvincular comunicador");
          }
        }
      ]
    });

    await actionSheet.present();
  }
}
