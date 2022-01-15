import { Component, OnInit } from '@angular/core';
import { Subscription } from 'rxjs';
import { DeviceService } from '../../../alarm-list/device.service';
import { Particion } from '../../../models/particion.model';
import { ActionSheetController, ModalController, LoadingController } from '@ionic/angular';
import { Nodo } from '../../../models/nodo.model';
import { NodeModalPage } from './node-modal/node-modal.page';
import { YesNoModalPage } from '../../../yes-no-modal/yes-no-modal.page';
import { CommandsService } from '../../../services/commands.service';

@Component({
  selector: 'app-nodes',
  templateUrl: './nodes.page.html',
  styleUrls: ['./nodes.page.scss'],
})
export class NodesPage implements OnInit {

  partition: Particion;
  private partitionSubscription: Subscription;
  private availableNodes: number[];

  constructor(
    private deviceService: DeviceService,
    private actionSheetController: ActionSheetController,
    private modalController: ModalController,
    private loadingController: LoadingController,
    private commandsService: CommandsService
  ) { }


  ngOnInit() {
    this.partitionSubscription = this.deviceService.currentPartition.subscribe( partition => {
        this.partition = partition;

        // El array availableZones tiene los números de nodo que no están usados
        this.availableNodes = Array.from({length: 128}, (x, i) => i);

        // Se ordenan los nodos por número de nodo
        this.partition.nodos.sort((a, b) => {return a.numero-b.numero});

        

        // Se parsea la información de los nodos (encendido o apagado)
        this.partition.nodos.forEach(nodo => {
          let index = 127 - nodo.numero;

          // Se elimina el número de zona de las disponibles
          let availableZoneIndex = this.availableNodes.findIndex((nodeNumber) => nodeNumber === nodo.numero)
          this.availableNodes.splice(availableZoneIndex, 1);

          if (this.partition.nodosEncendidos.charAt(index) === '1')
            nodo.encendido = true;
          else
            nodo.encendido = false;
        });
      }
    );
  }

  ngOnDestroy(): void {
    if (this.partitionSubscription)
      this.partitionSubscription.unsubscribe();
  }


  onNodeClicked(node: Nodo) {

  }

  async onAddNode() {
    const modal = await this.modalController.create({
      component: NodeModalPage,
      cssClass: 'auto-height',
      handle: false,
      componentProps: {
        "availableNodes": this.availableNodes,
        "name": ""
      }
    });

    modal.present();

    const {data} = await modal.onWillDismiss();
    
    if (data) {
      // Se crea un nuevo nodo y una vez creado, se vuelve a pedir el dispositivo
      const loading = await this.loadingController.create({
        keyboardClose: true,
        cssClass: 'custom-loading',
      });
  
      loading.present();

      this.deviceService.newNode(
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
          console.log("error al crear el nodo");
          loading.dismiss();
        });
    }
  }

  onNodeMore(node: Nodo, event: Event) {
    this.showNodeMoreActionSheet(node);

    // Se evita que se propague el evento de click al item
    event.stopPropagation();
    return false;
  }

  private async showNodeMoreActionSheet(node: Nodo) {

    let actionOnOff = {};
    if (node.encendido) {
      actionOnOff = {
        text: "Apagar nodo",
        handler: async () => {
          const loading = await this.loadingController.create({
            keyboardClose: true,
            message: "Enviando comando"
          });
      
          loading.present();

          this.commandsService.manageNodes(
            this.deviceService.currentDeviceComId,
            this.partition.numero,
            [{nodo: node.numero, encender: false}]).subscribe(() => {
              loading.dismiss();
            });
        }
      };
    }
    else {
      actionOnOff = {
        text: "Encender nodo",
        handler: async () => {
          const loading = await this.loadingController.create({
            keyboardClose: true,
            message: "Enviando comando"
          });
      
          loading.present();

          this.commandsService.manageNodes(
            this.deviceService.currentDeviceComId,
            this.partition.numero,
            [{nodo: node.numero, encender: true}]).subscribe(() => {
              loading.dismiss();
            });
        }
      };
    }

    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      mode: "ios",
      buttons: [
        {
          text: "Editar nodo",
          handler: async () => {
            this.actionSheetController.dismiss();

            const modal = await this.modalController.create({
              component: NodeModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "number": node.numero,
                "availableNodes": this.availableNodes,
                "name": node.nombre
              }
            });
        
            modal.present();
        
            const {data} = await modal.onWillDismiss();

            console.log(data);
            
            if (data) {
              // Se modifica el nodo y una vez modificado, se vuelve a pedir el dispositivo
              const loading = await this.loadingController.create({
                keyboardClose: true,
                cssClass: 'custom-loading',
              });
          
              loading.present();

              this.deviceService.updateNode(
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
                  console.log("error al modificar el nodo");
                  loading.dismiss();
                }
              );
            }
          }
        },
        {
          text: "Eliminar nodo",
          handler: async () => {
            this.actionSheetController.dismiss();

            const modal = await this.modalController.create({
              component: YesNoModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "message": `¿Deseás eliminar el nodo ${node.nombre}?`,
                "yesText": "Eliminar",
                "noText": "Cancelar"
              }
            });
        
            modal.present();
        
            const {data} = await modal.onWillDismiss();
            
            if (data && data.result === "yes") {
              // Se elimina el nodo y una vez eliminado, se vuelve a pedir el dispositivo
              const loading = await this.loadingController.create({
                keyboardClose: true,
                cssClass: 'custom-loading',
              });
          
              loading.present();

              this.deviceService.removeNode(
                this.deviceService.currentDeviceComId,
                this.partition.numero,
                node.numero
              ).subscribe(
                () => {
                  this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
                },
                () => {
                  console.log("error al eliminar el nodo");
                  loading.dismiss();
                }
              );
            }
          }
        },
        actionOnOff
      ]
    });

    await actionSheet.present();
  }
}
