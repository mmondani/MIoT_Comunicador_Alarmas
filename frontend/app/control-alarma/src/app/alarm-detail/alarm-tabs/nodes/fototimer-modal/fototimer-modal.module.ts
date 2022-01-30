import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { FototimerModalPageRoutingModule } from './fototimer-modal-routing.module';

import { FototimerModalPage } from './fototimer-modal.page';

@NgModule({
  imports: [
    CommonModule,
    ReactiveFormsModule,
    IonicModule,
    FototimerModalPageRoutingModule
  ],
  declarations: [FototimerModalPage]
})
export class FototimerModalPageModule {}
