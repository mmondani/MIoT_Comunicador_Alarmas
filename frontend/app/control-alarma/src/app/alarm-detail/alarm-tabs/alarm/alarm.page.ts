import { Component, OnDestroy, OnInit } from '@angular/core';
import { partition, Subscription } from 'rxjs';
import { take } from 'rxjs/operators';
import { DeviceService } from '../../../alarm-list/device.service';
import { Particion } from '../../../models/particion.model';

@Component({
  selector: 'app-alarm',
  templateUrl: './alarm.page.html',
  styleUrls: ['./alarm.page.scss'],
})
export class AlarmPage implements OnInit, OnDestroy {

  partition: Particion;
  partitionState: string;
  private partitionSubscription: Subscription;

  constructor(
    private deviceService: DeviceService
  ) { }


  ngOnInit() {
    this.partitionSubscription = this.deviceService.currentPartition
    .subscribe( partition => {

        if(partition) {
          if (partition.sonando)
            this.partitionState = "disparo"
          else {
            if (partition.estado === "desactivada")
              this.partitionState = "desactivada";
            else if (partition.estado === "activada") {
              if (partition.modo === "estoy")
                this.partitionState = "activada_estoy";
              else if (partition.modo === "me_voy")
                this.partitionState = "activada_me_voy"
              else
                this.partitionState = "activada";
            }
          }

          this.partition = partition;
        }   
      }
    );
  }

  ngOnDestroy(): void {
    if (this.partitionSubscription)
      this.partitionSubscription.unsubscribe();
  }


  onModoClick() {
    if (this.partitionState === 'desactivada') {
      console.log("onModoClick");
    }
  }

  onEventosClick() {
    if (this.partitionState === 'desactivada') {
      console.log("onEventosClick");
    }
  }

  onEstadoClick() {
    console.log("onEstadoClick");
  }

  onPanicoClick() {
    console.log("onPanicoClick");
  }

  onIncendioClick() {
    console.log("onIncendioClick");
  }

  onMedicoClick() {
    console.log("onMedicoClick");
  }
}
