import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { NodeModalPage } from './node-modal.page';

const routes: Routes = [
  {
    path: '',
    component: NodeModalPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class NodeModalPageRoutingModule {}
