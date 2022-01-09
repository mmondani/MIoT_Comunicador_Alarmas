import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { NodeModalPageRoutingModule } from './node-modal-routing.module';

import { NodeModalPage } from './node-modal.page';

@NgModule({
  imports: [
    CommonModule,
    ReactiveFormsModule,
    IonicModule,
    NodeModalPageRoutingModule
  ],
  declarations: [NodeModalPage]
})
export class NodeModalPageModule {}
