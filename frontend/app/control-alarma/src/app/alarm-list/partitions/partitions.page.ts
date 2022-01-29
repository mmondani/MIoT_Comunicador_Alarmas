import { Component, OnDestroy, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { Subscription } from 'rxjs';
import { DeviceService } from '../device.service';
import { Particion } from '../../models/particion.model';
import { Device } from '../../models/device.model';
import { ModalController, LoadingController, ActionSheetController } from '@ionic/angular';
import { PartitionModalPage } from './partition-modal/partition-modal.page';
import { YesNoModalPage } from '../../yes-no-modal/yes-no-modal.page';

@Component({
  selector: 'app-partitions',
  templateUrl: './partitions.page.html',
  styleUrls: ['./partitions.page.scss'],
})
export class PartitionsPage implements OnInit, OnDestroy {

  deviceComId: string;
  deviceName: string;
  device: Device;
  private deviceSubscription: Subscription;
  private availablePartitions: number[];


  constructor(
    private activatedRoute: ActivatedRoute,
    private deviceService: DeviceService,
    private modalController: ModalController,
    private loadingController: LoadingController,
    private actionSheetController: ActionSheetController
  ) { }


  ngOnInit() {
    this.deviceComId = this.activatedRoute.snapshot.params["id"];

    this.deviceService.currentDeviceComId = this.deviceComId;

    this.deviceService.getDeviceName(this.deviceComId)
      .subscribe(deviceName => {
        this.deviceName = deviceName;
      });

    this.deviceSubscription = this.deviceService.currentDevice.subscribe( device => {
      this.device = device;

      // El array availableUsers tiene los números de usuario que no están usados
      this.availablePartitions = Array.from({length: 8}, (x, i) => i+1);

      // Se ordenan las particiones por número de particion
      this.device.particiones.sort((a, b) => {return a.numero-b.numero});

      // Se eliminan los números de partición que no están disponibles
      this.device.particiones.forEach(particion => {
        let availableZoneIndex = this.availablePartitions.findIndex((zoneNumber) => zoneNumber === particion.numero)
        this.availablePartitions.splice(availableZoneIndex, 1);
      });
    });
  }


  ngOnDestroy(): void {
    if (this.deviceSubscription)
      this.deviceSubscription.unsubscribe();
  }

  async onAddPartition() {
    const modal = await this.modalController.create({
      component: PartitionModalPage,
      cssClass: 'auto-height',
      handle: false,
      componentProps: {
        "availablePartitions": this.availablePartitions,
        "name": ""
      }
    });

    modal.present();

    const {data} = await modal.onWillDismiss();
    
    if (data) {
      // Se crea una nueva partición y una vez creada, se vuelve a pedir el dispositivo
      const loading = await this.loadingController.create({
        keyboardClose: true,
        cssClass: 'custom-loading',
      });
  
      loading.present();

      this.deviceService.newPartition(
        this.deviceComId,
        data.number,
        data.name
      ).subscribe(
        () => {
          this.deviceService.getDevice(this.deviceComId).subscribe(() => loading.dismiss());
        },
        () => {
          console.log("error al crear la partición");
          loading.dismiss();
        }
      );
      
    }
  }

  onPartitionMore(partition: Particion, event: Event) {
    this.showPartitionMoreActionSheet(partition);

    // Se evita que se propague el evento de click a la card
    event.stopPropagation();
    return false;
  }

  private async showPartitionMoreActionSheet(partition: Particion) {
    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      mode: "ios",
      buttons: [
        {
          text: "Editar partición",
          cssClass: 'custom-action-sheet',
          handler: async () => {
            this.actionSheetController.dismiss();

            const modal = await this.modalController.create({
              component: PartitionModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "number": partition.numero,
                "availablePartitions": this.availablePartitions,
                "name": partition.nombre
              }
            });
        
            modal.present();
        
            const {data} = await modal.onWillDismiss()
            
            if (data) {
              // Se modifica la partición y una vez modificada, se vuelve a pedir el dispositivo
              const loading = await this.loadingController.create({
                keyboardClose: true,
                cssClass: 'custom-loading',
              });
          
              loading.present();

              this.deviceService.updatePartition(
                this.deviceService.currentDeviceComId,
                data.number,
                data.name
              ).subscribe(
                () => {
                  this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
                },
                () => {
                  console.log("error al modificar la partición");
                  loading.dismiss();
                }
              );
              
            }
          }
        },
        {
          text: "Eliminar partición",
          cssClass: 'custom-action-sheet',
          handler: async () => {
            this.actionSheetController.dismiss();

            const modal = await this.modalController.create({
              component: YesNoModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "message": `¿Deseás eliminar la partición ${partition.nombre}?`,
                "yesText": "Eliminar",
                "noText": "Cancelar"
              }
            });
        
            modal.present();
        
            const {data} = await modal.onWillDismiss()
            
            if (data && data.result === "yes") {
              // Se elimina la partición y una vez eliminada, se vuelve a pedir el dispositivo
              const loading = await this.loadingController.create({
                keyboardClose: true,
                cssClass: 'custom-loading',
              });
          
              loading.present();

              this.deviceService.removePartition(
                this.deviceService.currentDeviceComId,
                partition.numero
              ).subscribe(
                () => {
                  this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
                },
                () => {
                  console.log("error al eliminar la partición");
                  loading.dismiss();
                }
              );
              
            }
          }
        }
      ]
    });

    await actionSheet.present();
  }

}
