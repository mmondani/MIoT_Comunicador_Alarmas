import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { RecoverPinPageRoutingModule } from './recover-pin-routing.module';

import { RecoverPinPage } from './recover-pin.page';

@NgModule({
  imports: [
    CommonModule,
    IonicModule,
    RecoverPinPageRoutingModule,
    ReactiveFormsModule
  ],
  declarations: [RecoverPinPage]
})
export class RecoverPinPageModule {}
