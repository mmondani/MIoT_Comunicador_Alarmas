import { Component, OnDestroy, OnInit } from '@angular/core';
import { ActionSheetController, ModalController, LoadingController } from '@ionic/angular';
import { Subscription } from 'rxjs';
import { DeviceService } from '../../../alarm-list/device.service';
import { Particion } from '../../../models/particion.model';
import { ZoneModalPage } from './zone-modal/zone-modal.page';
import { Zona } from '../../../models/zona.model';
import { YesNoModalPage } from '../../../yes-no-modal/yes-no-modal.page';

@Component({
  selector: 'app-zones',
  templateUrl: './zones.page.html',
  styleUrls: ['./zones.page.scss'],
})
export class ZonesPage implements OnInit, OnDestroy {

  partition: Particion;
  private partitionSubscription: Subscription;
  private availableZones: number[];

  constructor(
    private deviceService: DeviceService,
    private actionSheetController:ActionSheetController,
    private modalController: ModalController,
    private loadingController: LoadingController
  ) { }


  ngOnInit() {
    this.partitionSubscription = this.deviceService.currentPartition.subscribe( partition => {
        this.partition = partition;

        // El array availableZones tiene los números de zona que no están usados
        this.availableZones = Array.from({length: 32}, (x, i) => i+1);

        // Se ordenan las zonas por número de zona
        this.partition.zonas.sort((a, b) => {return a.numero-b.numero});

        // Se parsea la información de las zonas (estado de la zona, inclusión, memorizada)
        this.partition.zonas.forEach(zona => {
          let index = 32 - zona.numero;

          // Se elimina el número de zona de las disponibles
          let availableZoneIndex = this.availableZones.findIndex((zoneNumber) => zoneNumber === zona.numero)
          this.availableZones.splice(availableZoneIndex, 1);

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
        });
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
      cssClass: 'auto-height',
      handle: false,
      componentProps: {
        "availableZones": this.availableZones,
        "name": ""
      }
    });

    modal.present();

    const {data} = await modal.onWillDismiss();
    
    if (data) {
      // Se crea una nueva zona y una vez creada, se vuelve a pedir el dispositivo
      const loading = await this.loadingController.create({
        keyboardClose: true,
        cssClass: 'custom-loading',
      });
  
      loading.present();

      this.deviceService.newZone(
        this.deviceService.currentDeviceComId,
        this.partition.numero,
        data.number,
        data.name,
        ""
      ).subscribe(
        () => {
          this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
        },
        () => {
          console.log("error al crear la zona");
          loading.dismiss();
        }
      );
    }
  }


  onZoneClicked(zone: Zona) {
    console.log("zona clickeada - " + zone.numero);
  }


  onZoneMore(zone: Zona, event: Event) {
    this.showZoneMoreActionSheet(zone);

    // Se evita que se propague el evento de click a la card
    event.stopPropagation();
    return false;
  }

  private async showZoneMoreActionSheet(zone: Zona) {
    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      mode: "ios",
      buttons: [
        {
          text: "Editar zona",
          handler: async () => {
            this.actionSheetController.dismiss();

            const modal = await this.modalController.create({
              component: ZoneModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "number": zone.numero,
                "availableZones": this.availableZones,
                "name": zone.nombre
              }
            });
        
            modal.present();
        
            const {data} = await modal.onWillDismiss()
            
            if (data) {
              // Se modifica la zona y una vez modificada, se vuelve a pedir el dispositivo
              const loading = await this.loadingController.create({
                keyboardClose: true,
                cssClass: 'custom-loading',
              });
          
              loading.present();

              this.deviceService.updateZone(
                this.deviceService.currentDeviceComId,
                this.partition.numero,
                data.number,
                data.name,
                ""
              ).subscribe(
                () => {
                  this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
                },
                () => {
                  console.log("error al modificar la zona");
                  loading.dismiss();
                }
              );
            }
          }
        },
        {
          text: "Eliminar zona",
          handler: async () => {
            this.actionSheetController.dismiss();

            const modal = await this.modalController.create({
              component: YesNoModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "message": `¿Deseás eliminar la zona ${zone.nombre}?`,
                "yesText": "Eliminar",
                "noText": "Cancelar"
              }
            });
        
            modal.present();
        
            const {data} = await modal.onWillDismiss()
            
            if (data && data.result === "yes") {
              // Se elimina la zona y una vez eliminada, se vuelve a pedir el dispositivo
              const loading = await this.loadingController.create({
                keyboardClose: true,
                cssClass: 'custom-loading',
              });
          
              loading.present();

              this.deviceService.removeZone(
                this.deviceService.currentDeviceComId,
                this.partition.numero,
                zone.numero
              ).subscribe(
                () => {
                  this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
                },
                () => {
                  console.log("error al eliminar la zona");
                  loading.dismiss();
                }
              );
            }
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
