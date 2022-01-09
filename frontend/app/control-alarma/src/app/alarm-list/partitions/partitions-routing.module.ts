import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { PartitionsPage } from './partitions.page';

const routes: Routes = [
  {
    path: '',
    component: PartitionsPage
  },
  {
    path: 'partition-modal',
    loadChildren: () => import('./partition-modal/partition-modal.module').then( m => m.PartitionModalPageModule)
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class PartitionsPageRoutingModule {}
