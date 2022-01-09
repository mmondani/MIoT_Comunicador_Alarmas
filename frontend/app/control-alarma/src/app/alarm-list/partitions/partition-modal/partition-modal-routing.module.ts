import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { PartitionModalPage } from './partition-modal.page';

const routes: Routes = [
  {
    path: '',
    component: PartitionModalPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class PartitionModalPageRoutingModule {}
