import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { ZoneModalPageRoutingModule } from './zone-modal-routing.module';

import { ZoneModalPage } from './zone-modal.page';

@NgModule({
  imports: [
    CommonModule,
    ReactiveFormsModule,
    IonicModule,
    ZoneModalPageRoutingModule
  ],
  declarations: [ZoneModalPage]
})
export class ZoneModalPageModule {}
