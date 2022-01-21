import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { AskCodePageRoutingModule } from './ask-code-routing.module';

import { AskCodePage } from './ask-code.page';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    AskCodePageRoutingModule
  ],
  declarations: [AskCodePage]
})
export class AskCodePageModule {}
