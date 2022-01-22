import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { IconModalPage } from './icon-modal.page';

const routes: Routes = [
  {
    path: '',
    component: IconModalPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class IconModalPageRoutingModule {}
