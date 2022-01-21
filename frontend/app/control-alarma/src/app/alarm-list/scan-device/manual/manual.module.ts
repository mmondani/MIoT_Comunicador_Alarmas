import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { ManualPageRoutingModule } from './manual-routing.module';

import { ManualPage } from './manual.page';

@NgModule({
  imports: [
    CommonModule,
    ReactiveFormsModule,
    IonicModule,
    ManualPageRoutingModule
  ],
  declarations: [ManualPage]
})
export class ManualPageModule {}
