import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { AlarmDetailPage } from './alarm-detail.page';
import { AlarmTabsPage } from './alarm-tabs/alarm-tabs.page';

const routes: Routes = [
  {
    path: '',
    component: AlarmDetailPage,
    children: [
      {
        path: 'tabs',
        component: AlarmTabsPage,
        children: [
          {
            path: 'alarm',
            children: [
              {
                path: '',
                loadChildren: () => import('./alarm-tabs/alarm/alarm.module').then( m => m.AlarmPageModule)
              }
            ]
          },
          {
            path: 'zones',
            children: [
              {
                path: '',
                loadChildren: () => import('./alarm-tabs/zones/zones.module').then( m => m.ZonesPageModule)
              }
            ]
          },
          {
            path: 'nodes',
            children: [
              {
                path: '',
                loadChildren: () => import('./alarm-tabs/nodes/nodes.module').then( m => m.NodesPageModule)
              }
            ]
          },
          {
            path: 'advanced',
            children: [
              {
                path: '',
                loadChildren: () => import('./alarm-tabs/advanced/advanced.module').then( m => m.AdvancedPageModule)
              }
            ]
          },
          {
            path: '',
            redirectTo: 'tabs/alarm',
            pathMatch: 'full'
          }
        ]
      },
      {
        path: '',
        redirectTo: 'tabs/alarm',
        pathMatch: 'full'
      }
    ]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class AlarmDetailPageRoutingModule {}
