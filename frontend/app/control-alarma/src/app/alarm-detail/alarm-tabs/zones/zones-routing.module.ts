import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { ZonesPage } from './zones.page';

const routes: Routes = [
  {
    path: '',
    component: ZonesPage
  },
  {
    path: 'zone-modal',
    loadChildren: () => import('./zone-modal/zone-modal.module').then( m => m.ZoneModalPageModule)
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class ZonesPageRoutingModule {}
