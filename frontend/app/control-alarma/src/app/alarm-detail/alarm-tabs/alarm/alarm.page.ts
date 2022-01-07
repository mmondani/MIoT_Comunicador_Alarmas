import { Component, OnDestroy, OnInit } from '@angular/core';
import { ActionSheetController } from '@ionic/angular';
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
    private deviceService: DeviceService,
    private actionSheetController: ActionSheetController
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


  async onModoClick() {
    if (this.partitionState === 'desactivada') {
      const actionModoEstoy = {
          text: "Cambiar a modo Estoy",
          handler: () => {
            console.log("Cambiar a modo Estoy");
          }
        };

      const actionModoMeVoy = {
          text: "Cambiar a modo Me Voy",
          handler: () => {
            console.log("Cambiar a modo Me Voy");
          }
        };


      let buttons = [];
      if (this.partition.modo === 'estoy') {
        buttons = [
          actionModoMeVoy
        ];
      }
      else if (this.partition.modo === "me_voy") {
        buttons = [
          actionModoEstoy
        ];
      }
      else {
        buttons = [
          actionModoEstoy,
          actionModoMeVoy
        ];
      }

      const actionSheet = await this.actionSheetController.create({
        cssClass: "action-sheet",
        buttons: buttons
      });

      await actionSheet.present();
    }
  }

  onEventosClick() {
    if (this.partitionState === 'desactivada') {
      console.log("onEventosClick");
    }
  }

  async onEstadoClick() {
    const actionActivar = {
        text: "Activar",
        handler: () => {
          console.log("Activar");
        }
      };

    const actionActivarEstoy = {
        text: "Activar en modo Estoy",
        handler: () => {
          console.log("Activar en modo Estoy");
        }
      };

    const actionActivarMeVoy = {
        text: "Activar en modo Me Voy",
        handler: () => {
          console.log("Activar en modo Me Voy");
        }
      };

    const actionDesactivar = {
        text: "Desactivar",
        handler: () => {
          console.log("Desactivar");
        }
      };

    let buttons = [];
    if (this.partitionState === 'desactivada') {
      buttons = [
        actionActivar,
        actionActivarEstoy,
        actionActivarMeVoy
      ]
    }
    else if (this.partitionState === 'activada' || this.partitionState === 'activada_estoy' || this.partitionState === 'activada_me_voy' || this.partition.sonando) {
      buttons = [
        actionDesactivar
      ]
    }

    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      buttons: buttons
    });

    await actionSheet.present();
  }

  async onPanicoClick() {
    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      buttons: [
        {
          text: "Disparar por pánico",
          handler: () => {
            console.log("Disparar por pánico");
          }
        }
      ]
    });

    await actionSheet.present();
  }

  async onIncendioClick() {
    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      buttons: [
        {
          text: "Disparar por incendio",
          handler: () => {
            console.log("Disparar por incendio");
          }
        }
      ]
    });

    await actionSheet.present();
  }

  async onMedicoClick() {
    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      buttons: [
        {
          text: "Disparar emergencia médica",
          handler: () => {
            console.log("Disparar emergencia médica");
          }
        }
      ]
    });

    await actionSheet.present();
  }
}
