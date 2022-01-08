import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { YesNoModalPageRoutingModule } from './yes-no-modal-routing.module';

import { YesNoModalPage } from './yes-no-modal.page';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    YesNoModalPageRoutingModule
  ],
  declarations: [YesNoModalPage]
})
export class YesNoModalPageModule {}
