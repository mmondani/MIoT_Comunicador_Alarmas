import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { AlarmDetailPage } from './alarm-detail.page';
import { AlarmTabsPage } from './alarm-tabs/alarm-tabs.page';

const routes: Routes = [
  {
    path: '',
    component: AlarmDetailPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class AlarmDetailPageRoutingModule {}
