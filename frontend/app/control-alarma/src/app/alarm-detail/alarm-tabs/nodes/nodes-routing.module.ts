import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { NodesPage } from './nodes.page';

const routes: Routes = [
  {
    path: '',
    component: NodesPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class NodesPageRoutingModule {}
