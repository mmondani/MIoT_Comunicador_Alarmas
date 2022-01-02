import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { AlarmDetailPageRoutingModule } from './alarm-detail-routing.module';

import { AlarmDetailPage } from './alarm-detail.page';
import { AlarmTabsPageModule } from './alarm-tabs/alarm-tabs.module';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    AlarmDetailPageRoutingModule,
    AlarmTabsPageModule
  ],
  declarations: [AlarmDetailPage]
})
export class AlarmDetailPageModule {}
