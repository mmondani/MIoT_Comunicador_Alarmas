import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { ConfigWifiPage } from './config-wifi.page';

const routes: Routes = [
  {
    path: '',
    component: ConfigWifiPage
  },
  {
    path: 'wifi-credentials',
    loadChildren: () => import('./wifi-credentials/wifi-credentials.module').then( m => m.WifiCredentialsPageModule)
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class ConfigWifiPageRoutingModule {}
