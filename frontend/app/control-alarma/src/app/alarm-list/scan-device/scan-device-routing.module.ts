import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { ScanDevicePage } from './scan-device.page';

const routes: Routes = [
  {
    path: '',
    component: ScanDevicePage
  },
  {
    path: 'manual',
    loadChildren: () => import('./manual/manual.module').then( m => m.ManualPageModule)
  },
  {
    path: 'ask-code/:id',
    loadChildren: () => import('./ask-code/ask-code.module').then( m => m.AskCodePageModule)
  },
  {
    path: 'config-device/:id',
    loadChildren: () => import('../config-device/config-device.module').then( m => m.ConfigDevicePageModule)
  },
  {
    path: 'config-wifi',
    loadChildren: () => import('../config-wifi/config-wifi.module').then( m => m.ConfigWifiPageModule)
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class ScanDevicePageRoutingModule {}
