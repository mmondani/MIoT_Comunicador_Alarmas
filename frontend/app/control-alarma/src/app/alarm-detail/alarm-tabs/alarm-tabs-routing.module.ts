import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { AlarmTabsPage } from './alarm-tabs.page';

const routes: Routes = [
  {
    path: '',
    component: AlarmTabsPage,
    children: [
      {
        path: 'alarm',
        loadChildren: () => import('../alarm-tabs/alarm/alarm.module').then( m => m.AlarmPageModule)
      },
      {
        path: '',
        redirectTo: 'alarm',
        pathMatch: 'full'
      }
    ]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class AlarmTabsPageRoutingModule {}
