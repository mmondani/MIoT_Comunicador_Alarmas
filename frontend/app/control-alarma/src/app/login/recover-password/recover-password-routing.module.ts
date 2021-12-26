import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { RecoverPasswordPage } from './recover-password.page';

const routes: Routes = [
  {
    path: '',
    component: RecoverPasswordPage
  },
  {
    path: 'recover-pin',
    loadChildren: () => import('./recover-pin/recover-pin.module').then( m => m.RecoverPinPageModule)
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class RecoverPasswordPageRoutingModule {}
