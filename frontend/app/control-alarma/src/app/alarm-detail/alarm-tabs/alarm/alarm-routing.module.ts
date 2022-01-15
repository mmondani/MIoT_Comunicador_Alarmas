import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { AlarmPage } from './alarm.page';

const routes: Routes = [
  {
    path: 'events',
    loadChildren: () => import('./events/events.module').then( m => m.EventsPageModule)
  },
  {
    path: '',
    component: AlarmPage
  },
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class AlarmPageRoutingModule {}
