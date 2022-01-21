import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { AskCodePage } from './ask-code.page';

const routes: Routes = [
  {
    path: '',
    component: AskCodePage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class AskCodePageRoutingModule {}
