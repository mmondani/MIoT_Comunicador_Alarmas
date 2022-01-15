import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { IsoDate2DateStringPipe } from './iso-date2-date-string.pipe';



@NgModule({
  declarations: [IsoDate2DateStringPipe],
  imports: [
    CommonModule
  ],
  exports: [
    IsoDate2DateStringPipe
  ]
})
export class SharedModule { }
