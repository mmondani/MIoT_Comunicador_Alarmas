import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { AlarmListPage } from './alarm-list.page';

const routes: Routes = [
  {
    path: '',
    component: AlarmListPage
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class AlarmListPageRoutingModule {}
