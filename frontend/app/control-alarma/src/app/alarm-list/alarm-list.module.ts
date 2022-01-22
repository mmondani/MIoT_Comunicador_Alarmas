import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { AlarmListPageRoutingModule } from './alarm-list-routing.module';

import { AlarmListPage } from './alarm-list.page';
import { QRCodeModule } from 'angularx-qrcode';
import { QrModalPageModule } from './qr-modal/qr-modal.module';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    AlarmListPageRoutingModule,
    QRCodeModule,
    QrModalPageModule
  ],
  declarations: [AlarmListPage]
})
export class AlarmListPageModule {}
