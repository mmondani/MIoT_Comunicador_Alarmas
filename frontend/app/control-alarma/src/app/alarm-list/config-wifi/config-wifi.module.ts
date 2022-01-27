import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { ConfigWifiPageRoutingModule } from './config-wifi-routing.module';

import { ConfigWifiPage } from './config-wifi.page';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    ConfigWifiPageRoutingModule
  ],
  declarations: [ConfigWifiPage]
})
export class ConfigWifiPageModule {}
