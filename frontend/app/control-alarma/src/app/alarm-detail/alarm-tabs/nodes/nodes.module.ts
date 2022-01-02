import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { NodesPageRoutingModule } from './nodes-routing.module';

import { NodesPage } from './nodes.page';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    NodesPageRoutingModule
  ],
  declarations: [NodesPage]
})
export class NodesPageModule {}
