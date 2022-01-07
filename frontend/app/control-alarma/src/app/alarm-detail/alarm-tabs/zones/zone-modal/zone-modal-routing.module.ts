import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { ZoneModalPage } from './zone-modal.page';

const routes: Routes = [
  {
    path: '',
    component: ZoneModalPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class ZoneModalPageRoutingModule {}
