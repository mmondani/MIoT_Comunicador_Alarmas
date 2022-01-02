import { Component, OnInit } from '@angular/core';
import { switchMap } from 'rxjs/operators';
import { AuthService } from '../login/auth.service';
import { DeviceService } from './device.service';
import { Device } from '../models/device.model';
import { ActionSheetController, LoadingController, NavController } from '@ionic/angular';

@Component({
  selector: 'app-alarm-list',
  templateUrl: './alarm-list.page.html',
  styleUrls: ['./alarm-list.page.scss'],
})
export class AlarmListPage implements OnInit {

  deviceList: Device[];

  constructor(
    private loadingController: LoadingController,
    private authService: AuthService,
    private deviceService: DeviceService,
    private actionSheetController: ActionSheetController,
    private navigationController: NavController
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
    });
  }

  onItemClicked(comId: string) {
    this.navigationController.navigateForward(['alarm-detail', comId, 1], {animated: true});
  }

  onAlarmMore(comId: string, event: Event) {
    this.showAlarmMoreActionSheet();

    // Se evita que se propague el evento de click a la card
    event.stopPropagation();
    return false;
  }

  private async showAlarmMoreActionSheet() {
    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      buttons: [
        {
          text: "Configurar comunicador",
          handler: () => {
            console.log("configurar comunicador");
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
