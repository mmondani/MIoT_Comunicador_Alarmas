import { Pipe, PipeTransform } from '@angular/core';

@Pipe({
  name: 'isoDate2DateString'
})
export class IsoDate2DateStringPipe implements PipeTransform {

  transform(timestamp: string): string {
    let date = new Date(timestamp);
      let year = date.getFullYear();
      let month = date.getMonth()+1;
      let day = date.getDate();
      let hour = date.getHours();
      let minutes = date.getMinutes();
      let seconds = date.getSeconds();

      return `${day.toString().padStart(2, "0")}/${month.toString().padStart(2, "0")}/${year} - ${hour.toString().padStart(2, "0")}:${minutes.toString().padStart(2, "0")}:${seconds.toString().padStart(2, "0")}`;
    
  }

}
