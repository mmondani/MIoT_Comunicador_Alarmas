import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { ZonesPageRoutingModule } from './zones-routing.module';

import { ZonesPage } from './zones.page';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    ZonesPageRoutingModule
  ],
  declarations: [ZonesPage]
})
export class ZonesPageModule {}
