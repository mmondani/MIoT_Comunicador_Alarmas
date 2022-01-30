import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { NodesPage } from './nodes.page';

const routes: Routes = [
  {
    path: '',
    component: NodesPage
  },
  {
    path: 'node-modal',
    loadChildren: () => import('./node-modal/node-modal.module').then( m => m.NodeModalPageModule)
  },
  {
    path: 'noche-modal',
    loadChildren: () => import('./noche-modal/noche-modal.module').then( m => m.NocheModalPageModule)
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class NodesPageRoutingModule {}
