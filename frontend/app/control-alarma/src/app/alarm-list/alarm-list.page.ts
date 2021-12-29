import { Component, OnInit } from '@angular/core';
import { switchMap } from 'rxjs/operators';
import { AuthService } from '../login/auth.service';
import { DeviceService } from './device.service';
import { Device } from '../models/device.model';
import { LoadingController } from '@ionic/angular';

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
    private deviceService: DeviceService
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

      deviceList.forEach(device => {
        console.log(device.comId);
      })
    });
  }

  onLogout() {
    this.authService.logout();
  }
}
