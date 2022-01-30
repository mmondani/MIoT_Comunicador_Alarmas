import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { NocheModalPage } from './noche-modal.page';

const routes: Routes = [
  {
    path: '',
    component: NocheModalPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class NocheModalPageRoutingModule {}
