import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { ConfigDevicePageRoutingModule } from './config-device-routing.module';

import { ConfigDevicePage } from './config-device.page';
import { IconModalPageModule } from './icon-modal/icon-modal.module';

@NgModule({
  imports: [
    CommonModule,
    ReactiveFormsModule,
    IonicModule,
    ConfigDevicePageRoutingModule,
    IconModalPageModule
  ],
  declarations: [ConfigDevicePage]
})
export class ConfigDevicePageModule {}
