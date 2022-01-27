import { Component, OnInit } from '@angular/core';
import { Router } from '@angular/router';
import { NavController } from '@ionic/angular';

@Component({
  selector: 'app-config-wifi',
  templateUrl: './config-wifi.page.html',
  styleUrls: ['./config-wifi.page.scss'],
})
export class ConfigWifiPage implements OnInit {

  configNewDevice: boolean;


  constructor(
    private router: Router,
    private navController: NavController
  ) { 
    this.configNewDevice = this.router.url.includes("scan-device");
  }

  ngOnInit() {
  }

  onContinue() {
    this.navController.navigateForward([this.router.url, 'wifi-credentials']);
  }

  onCancel() {
    this.router.navigateByUrl("/");
  }
}
