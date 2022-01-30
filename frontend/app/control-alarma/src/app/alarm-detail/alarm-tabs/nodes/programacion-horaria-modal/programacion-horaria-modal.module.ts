import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { ProgramacionHorariaModalPageRoutingModule } from './programacion-horaria-modal-routing.module';

import { ProgramacionHorariaModalPage } from './programacion-horaria-modal.page';

@NgModule({
  imports: [
    CommonModule,
    ReactiveFormsModule,
    IonicModule,
    ProgramacionHorariaModalPageRoutingModule
  ],
  declarations: [ProgramacionHorariaModalPage]
})
export class ProgramacionHorariaModalPageModule {}
