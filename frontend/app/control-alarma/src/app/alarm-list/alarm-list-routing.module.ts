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
  },
  {
    path: 'config-device/:id',
    loadChildren: () => import('./config-device/config-device.module').then( m => m.ConfigDevicePageModule)
  },
  {
    path: 'qr-modal',
    loadChildren: () => import('./qr-modal/qr-modal.module').then( m => m.QrModalPageModule)
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class AlarmListPageRoutingModule {}
