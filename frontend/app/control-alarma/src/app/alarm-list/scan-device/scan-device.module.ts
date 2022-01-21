import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { ScanDevicePageRoutingModule } from './scan-device-routing.module';

import { ScanDevicePage } from './scan-device.page';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    ScanDevicePageRoutingModule
  ],
  declarations: [ScanDevicePage]
})
export class ScanDevicePageModule {}
