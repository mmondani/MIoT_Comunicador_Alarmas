import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { FototimerModalPage } from './fototimer-modal.page';

const routes: Routes = [
  {
    path: '',
    component: FototimerModalPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class FototimerModalPageRoutingModule {}
