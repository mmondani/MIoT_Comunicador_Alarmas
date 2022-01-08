import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { YesNoModalPage } from './yes-no-modal.page';

const routes: Routes = [
  {
    path: '',
    component: YesNoModalPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class YesNoModalPageRoutingModule {}
