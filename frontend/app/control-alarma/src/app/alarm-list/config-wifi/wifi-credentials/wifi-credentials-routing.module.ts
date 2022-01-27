import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { WifiCredentialsPage } from './wifi-credentials.page';

const routes: Routes = [
  {
    path: '',
    component: WifiCredentialsPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class WifiCredentialsPageRoutingModule {}
