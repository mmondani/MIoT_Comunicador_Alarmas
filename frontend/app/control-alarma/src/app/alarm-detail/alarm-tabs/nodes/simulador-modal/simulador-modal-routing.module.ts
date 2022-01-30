import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { SimuladorModalPage } from './simulador-modal.page';

const routes: Routes = [
  {
    path: '',
    component: SimuladorModalPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class SimuladorModalPageRoutingModule {}
