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
  },
  {
    path: 'fototimer-modal',
    loadChildren: () => import('./fototimer-modal/fototimer-modal.module').then( m => m.FototimerModalPageModule)
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class NodesPageRoutingModule {}
