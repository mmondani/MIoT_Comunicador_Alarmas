import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';

import { IonicModule } from '@ionic/angular';

import { WifiCredentialsPageRoutingModule } from './wifi-credentials-routing.module';

import { WifiCredentialsPage } from './wifi-credentials.page';

@NgModule({
  imports: [
    CommonModule,
    ReactiveFormsModule,
    IonicModule,
    WifiCredentialsPageRoutingModule
  ],
  declarations: [WifiCredentialsPage]
})
export class WifiCredentialsPageModule {}
