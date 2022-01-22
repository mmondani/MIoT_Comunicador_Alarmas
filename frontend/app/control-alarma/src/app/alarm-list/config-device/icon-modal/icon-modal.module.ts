import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { IconModalPageRoutingModule } from './icon-modal-routing.module';

import { IconModalPage } from './icon-modal.page';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    IconModalPageRoutingModule
  ],
  exports: [IconModalPage],
  declarations: [IconModalPage]
})
export class IconModalPageModule {}
