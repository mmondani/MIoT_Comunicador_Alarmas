import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { NocheModalPageRoutingModule } from './noche-modal-routing.module';

import { NocheModalPage } from './noche-modal.page';

@NgModule({
  imports: [
    CommonModule,
    ReactiveFormsModule,
    IonicModule,
    NocheModalPageRoutingModule
  ],
  declarations: [NocheModalPage]
})
export class NocheModalPageModule {}
