import { Component, OnInit } from '@angular/core';
import { Subscription } from 'rxjs';
import { DeviceService } from '../../../alarm-list/device.service';
import { Particion } from '../../../models/particion.model';
import { ActionSheetController } from '@ionic/angular';
import { Nodo } from '../../../models/nodo.model';

@Component({
  selector: 'app-nodes',
  templateUrl: './nodes.page.html',
  styleUrls: ['./nodes.page.scss'],
})
export class NodesPage implements OnInit {

  partition: Particion;
  private partitionSubscription: Subscription;

  constructor(
    private deviceService: DeviceService,
    private actionSheetController: ActionSheetController
  ) { }


  ngOnInit() {
    this.partitionSubscription = this.deviceService.currentPartition.subscribe( partition => {
        this.partition = partition;

        // Se parsea la información de las zonas
        this.partition.nodos.forEach(nodo => {
          let index = 127 - nodo.numero;

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

  onNodeMore(node: Nodo, event: Event) {
    this.showZoneMoreActionSheet(node);

    // Se evita que se propague el evento de click al item
    event.stopPropagation();
    return false;
  }

  private async showZoneMoreActionSheet(node: Nodo) {

    let actionOnOff = {};
    if (node.encendido) {
      actionOnOff = {
        text: "Apagar nodo",
          handler: () => {
            console.log("Apagar nodo");
          }
      };
    }
    else {
      actionOnOff = {
        text: "Encender nodo",
          handler: () => {
            console.log("Encender nodo");
          }
      };
    }

    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      mode: "ios",
      buttons: [
        {
          text: "Editar nodo",
          handler: () => {
            console.log("Editar nodo");
          }
        },
        {
          text: "Eliminar nodo",
          handler: () => {
            console.log("Eliminar nodo");
          }
        },
        actionOnOff
      ]
    });

    await actionSheet.present();
  }
}
