import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { PartitionsPageRoutingModule } from './partitions-routing.module';

import { PartitionsPage } from './partitions.page';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    PartitionsPageRoutingModule
  ],
  declarations: [PartitionsPage]
})
export class PartitionsPageModule {}
