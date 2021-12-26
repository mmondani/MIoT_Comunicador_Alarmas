import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { RecoverPinPage } from './recover-pin.page';

const routes: Routes = [
  {
    path: '',
    component: RecoverPinPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class RecoverPinPageRoutingModule {}
