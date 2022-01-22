import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { AskCodePageRoutingModule } from './ask-code-routing.module';

import { AskCodePage } from './ask-code.page';

@NgModule({
  imports: [
    CommonModule,
    ReactiveFormsModule,
    IonicModule,
    AskCodePageRoutingModule
  ],
  declarations: [AskCodePage]
})
export class AskCodePageModule {}
