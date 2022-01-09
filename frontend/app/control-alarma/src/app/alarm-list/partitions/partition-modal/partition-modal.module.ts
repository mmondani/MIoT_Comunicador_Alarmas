import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { PartitionModalPageRoutingModule } from './partition-modal-routing.module';

import { PartitionModalPage } from './partition-modal.page';

@NgModule({
  imports: [
    CommonModule,
    ReactiveFormsModule,
    IonicModule,
    PartitionModalPageRoutingModule
  ],
  declarations: [PartitionModalPage]
})
export class PartitionModalPageModule {}
