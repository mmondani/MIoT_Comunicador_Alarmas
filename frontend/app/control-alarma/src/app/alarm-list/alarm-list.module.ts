import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { AlarmListPageRoutingModule } from './alarm-list-routing.module';

import { AlarmListPage } from './alarm-list.page';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    AlarmListPageRoutingModule
  ],
  declarations: [AlarmListPage]
})
export class AlarmListPageModule {}
