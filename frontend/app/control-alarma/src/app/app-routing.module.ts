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
    path: 'alarm-detail/:id',
    loadChildren: () => import('./alarm-detail/alarm-detail.module').then( m => m.AlarmDetailPageModule)
  },
  {
    path: '',
    loadChildren: () => import('./alarm-list/alarm-list.module').then( m => m.AlarmListPageModule),
    pathMatch: 'full',
    canLoad: [AuthGuard]
  }
];

@NgModule({
  imports: [
    RouterModule.forRoot(routes, { preloadingStrategy: PreloadAllModules })
  ],
  exports: [RouterModule]
})
export class AppRoutingModule { }
