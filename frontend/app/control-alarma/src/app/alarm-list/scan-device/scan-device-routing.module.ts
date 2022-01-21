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
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class ScanDevicePageRoutingModule {}
