import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { AlarmTabsPage } from './alarm-tabs.page';

const routes: Routes = [

];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class AlarmTabsPageRoutingModule {}
