import { Component, OnDestroy, OnInit } from '@angular/core';
import { ActionSheetController, ModalController } from '@ionic/angular';
import { Subscription } from 'rxjs';
import { DeviceService } from '../../../alarm-list/device.service';
import { Particion } from '../../../models/particion.model';
import { ZoneModalPage } from './zone-modal/zone-modal.page';

@Component({
  selector: 'app-zones',
  templateUrl: './zones.page.html',
  styleUrls: ['./zones.page.scss'],
})
export class ZonesPage implements OnInit, OnDestroy {

  partition: Particion;
  private partitionSubscription: Subscription;

  constructor(
    private deviceService: DeviceService,
    private actionSheetController:ActionSheetController,
    private modalController: ModalController
  ) { }


  ngOnInit() {
    this.partitionSubscription = this.deviceService.currentPartition.subscribe( partition => {
        this.partition = partition;

        // Se parsea la información de las zonas
        this.partition.zonas.forEach(zona => {
          let index = 32 - zona.numero;

          if (this.partition.zonasAnormales.charAt(index) === "1")
            zona.estado = "anormal";
          else
            zona.estado = "normal";

            if (this.partition.zonasMemorizadas.charAt(index) === "1")
              zona.memorizada = true;
            else
              zona.memorizada = false;

            if (this.partition.zonasIncluidas.charAt(index) === "1") {
              zona.inclusion = "incluida";

              if (zona.numero <= 4) {
                if (this.partition.zonasCondicionales.charAt(4 - zona.numero) === "1")
                  zona.inclusion = "temporizada";
              }
            }
            else
              zona.inclusion = "excluida";
        })
      }
    );
  }

  ngOnDestroy(): void {
    if (this.partitionSubscription)
      this.partitionSubscription.unsubscribe();
  }


  async onAddZone() {
    const modal = await this.modalController.create({
      component: ZoneModalPage,
      initialBreakpoint: 0.3,
      breakpoints: [0.3],
      handle: false,
      componentProps: {
        "number": "1",
        "name": ""
      }
    });

    modal.present();


    const {data} = await modal.onWillDismiss()
    console.log(data);
  }


  onZoneClicked(zoneNumber: number) {
    console.log("zona clickeada - " + zoneNumber);
  }


  onZoneMore(zoneNumber: number, event: Event) {
    this.showZoneMoreActionSheet();

    // Se evita que se propague el evento de click a la card
    event.stopPropagation();
    return false;
  }

  private async showZoneMoreActionSheet() {
    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      mode: "ios",
      buttons: [
        {
          text: "Editar zona",
          handler: () => {
            console.log("Editar zona");
          }
        },
        {
          text: "Eliminar zona",
          handler: () => {
            console.log("Eliminar zona");
          }
        },
        {
          text: "Cambiar inclusión",
          handler: () => {
            console.log("Cambiar inclusión");
          }
        }
      ]
    });

    await actionSheet.present();
  }
}
