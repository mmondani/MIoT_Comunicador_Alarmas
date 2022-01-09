import { NgModule } from '@angular/core';
import { PreloadAllModules, RouterModule, Routes } from '@angular/router';
import { AuthGuard } from './login/auth.guard';

const routes: Routes = [
  {
    path: 'alarm-list',
    loadChildren: () => import('./alarm-list/alarm-list.module').then( m => m.AlarmListPageModule),
    canLoad: [AuthGuard]
  },
  {
    path: 'login',
    loadChildren: () => import('./login/login.module').then( m => m.LoginPageModule)
  },
  {
    path: 'alarm-detail/:id/:partition',
    loadChildren: () => import('./alarm-detail/alarm-detail.module').then( m => m.AlarmDetailPageModule)
  },
  {
    path: 'partitions/:id',
    loadChildren: () => import('./alarm-list/partitions/partitions.module').then( m => m.PartitionsPageModule)
  },
  {
    path: '',
    loadChildren: () => import('./alarm-list/alarm-list.module').then( m => m.AlarmListPageModule),
    pathMatch: 'full',
    canLoad: [AuthGuard]
  },
  {
    path: 'yes-no-modal',
    loadChildren: () => import('./yes-no-modal/yes-no-modal.module').then( m => m.YesNoModalPageModule)
  }
];

@NgModule({
  imports: [
    RouterModule.forRoot(routes, { preloadingStrategy: PreloadAllModules })
  ],
  exports: [RouterModule]
})
export class AppRoutingModule { }
