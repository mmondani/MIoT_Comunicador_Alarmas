import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { AlarmTabsPageRoutingModule } from './alarm-tabs-routing.module';

import { AlarmTabsPage } from './alarm-tabs.page';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    IonicModule,
    AlarmTabsPageRoutingModule
  ],
  exports: [AlarmTabsPage],
  declarations: [AlarmTabsPage]
})
export class AlarmTabsPageModule {}
