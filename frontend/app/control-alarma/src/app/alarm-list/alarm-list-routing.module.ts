import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { AlarmListPage } from './alarm-list.page';

const routes: Routes = [
  {
    path: '',
    component: AlarmListPage
  },
  {
    path: 'scan-device',
    loadChildren: () => import('./scan-device/scan-device.module').then( m => m.ScanDevicePageModule)
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class AlarmListPageRoutingModule {}
