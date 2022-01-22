import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { ConfigDevicePage } from './config-device.page';

const routes: Routes = [
  {
    path: '',
    component: ConfigDevicePage
  },
  {
    path: 'icon-modal',
    loadChildren: () => import('./icon-modal/icon-modal.module').then( m => m.IconModalPageModule)
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class ConfigDevicePageRoutingModule {}
