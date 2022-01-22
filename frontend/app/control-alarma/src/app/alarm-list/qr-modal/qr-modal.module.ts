import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { QrModalPageRoutingModule } from './qr-modal-routing.module';

import { QrModalPage } from './qr-modal.page';
import { QRCodeModule } from 'angularx-qrcode';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    QrModalPageRoutingModule,
    QRCodeModule
  ],
  exports: [QrModalPage],
  declarations: [QrModalPage]
})
export class QrModalPageModule {}
