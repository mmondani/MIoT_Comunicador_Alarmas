import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { SimuladorModalPageRoutingModule } from './simulador-modal-routing.module';

import { SimuladorModalPage } from './simulador-modal.page';

@NgModule({
  imports: [
    CommonModule,
    ReactiveFormsModule,
    IonicModule,
    SimuladorModalPageRoutingModule
  ],
  declarations: [SimuladorModalPage]
})
export class SimuladorModalPageModule {}
