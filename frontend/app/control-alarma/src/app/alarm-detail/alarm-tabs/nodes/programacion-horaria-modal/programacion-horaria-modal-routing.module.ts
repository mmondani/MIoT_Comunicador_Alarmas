import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { ProgramacionHorariaModalPage } from './programacion-horaria-modal.page';

const routes: Routes = [
  {
    path: '',
    component: ProgramacionHorariaModalPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class ProgramacionHorariaModalPageRoutingModule {}
