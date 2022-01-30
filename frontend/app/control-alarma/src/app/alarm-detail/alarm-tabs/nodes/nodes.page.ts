import { Component, OnInit } from '@angular/core';
import { Subscription } from 'rxjs';
import { DeviceService } from '../../../alarm-list/device.service';
import { Particion } from '../../../models/particion.model';
import { ActionSheetController, ModalController, LoadingController } from '@ionic/angular';
import { Nodo } from '../../../models/nodo.model';
import { NodeModalPage } from './node-modal/node-modal.page';
import { YesNoModalPage } from '../../../yes-no-modal/yes-no-modal.page';
import { CommandsService } from '../../../services/commands.service';
import { NodoAutomatizacion } from '../../../models/nodoAutomatizacion.model';
import { NocheModalPage } from './noche-modal/noche-modal.page';
import { FototimerModalPage } from './fototimer-modal/fototimer-modal.page';

@Component({
  selector: 'app-nodes',
  templateUrl: './nodes.page.html',
  styleUrls: ['./nodes.page.scss'],
})
export class NodesPage implements OnInit {

  partition: Particion;
  nodesAutomations: NodoAutomatizacion[] = [];
  private partitionSubscription: Subscription;
  private availableNodes: number[];
  private availableNoches: number[];
  private availableFotoTimers: number[];
  private availableProgramacionHorarias: number[];
  private availableSimulador: number[];

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
        this.nodesAutomations = [];

        // El array availableNodes tiene los números de nodo que no están usados
        this.availableNodes = Array.from({length: 128}, (x, i) => i);

        // Los arrays availableNoches, availableFotoTimers, availableProgramacionHorarias y availableSimulador
        // contienen los números de automatizaciones disponibles. Puede haber hasta 16
        this.availableNoches = Array.from({length: 16}, (x, i) => i+1);
        this.availableFotoTimers = Array.from({length: 16}, (x, i) => i+1);
        this.availableProgramacionHorarias = Array.from({length: 16}, (x, i) => i+1);
        this.availableSimulador = Array.from({length: 16}, (x, i) => i+1);


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

        // Se juntan los nodos y las automatizaciones en un mismo array para mostrarlo en la lsit
        this.partition.nodos.forEach(node => {
          this.nodesAutomations.push(new NodoAutomatizacion(node.numero, node.nombre, "nodo", node.encendido));
        });

        this.partition.automatizaciones.forEach(automation => {
          let availableIndex;

          switch(automation.tipo) {
            case "fototimer":
              availableIndex = this.availableFotoTimers.findIndex((automationNumber) => automationNumber === automation.numero)
              this.availableFotoTimers.splice(availableIndex, 1);
              break;

            case "noche":
              availableIndex = this.availableNoches.findIndex((automationNumber) => automationNumber === automation.numero)
              this.availableNoches.splice(availableIndex, 1);
              break;

            case "programacion_horaria":
              availableIndex = this.availableProgramacionHorarias.findIndex((automationNumber) => automationNumber === automation.numero)
              this.availableProgramacionHorarias.splice(availableIndex, 1);
              break;  

            case "simulador":
              availableIndex = this.availableSimulador.findIndex((automationNumber) => automationNumber === automation.numero)
              this.availableSimulador.splice(availableIndex, 1);
              break;
          }
          

          this.nodesAutomations.push(new NodoAutomatizacion(automation.numero, automation.nombre, automation.tipo, null, automation.horaInicio, automation.horaFin, automation.horas, automation.nodos));
        })
      }
    );
  }

  ngOnDestroy(): void {
    if (this.partitionSubscription)
      this.partitionSubscription.unsubscribe();
  }


  onNodeClicked(node: NodoAutomatizacion) {

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

  async onAddAutomation() {
    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      mode: "ios",
      buttons: [
        {
          text: "Modo noche",
          cssClass: 'custom-action-sheet',
          handler: async () => {
            this.actionSheetController.dismiss();

            if (this.availableNoches.length === 0)
              return;

            const modal = await this.modalController.create({
              component: NocheModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "number": this.availableNoches[0],
                "availableNodes": this.partition.nodos
              }
            });
        
            modal.present();
        
            const {data} = await modal.onWillDismiss();

            if (data) {
              /**
               * El modal retorna number, name y selectedNodes
               */
              
              // Se crea la automatización y una vez creada, se vuelve a pedir el dispositivo
              const loading = await this.loadingController.create({
                keyboardClose: true,
                cssClass: 'custom-loading',
              });
          
              loading.present();

              this.deviceService.newAutomation(
                this.deviceService.currentDeviceComId,
                this.partition.numero,
                data.number,
                data.name,
                "noche",
                data.selectedNodes
              ).subscribe(
                () => {
                  this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
                },
                () => {
                  console.log("error al crear el modo noche");
                  loading.dismiss();
                }
              );
            }
          }
        },
        {
          text: "Foto-timer",
          cssClass: 'custom-action-sheet',
          handler: async () => {
            this.actionSheetController.dismiss();

            if (this.availableFotoTimers.length === 0)
              return;

            const modal = await this.modalController.create({
              component: FototimerModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "number": this.availableFotoTimers[0],
                "availableNodes": this.partition.nodos,
                "hours": 5
              }
            });
        
            modal.present();
        
            const {data} = await modal.onWillDismiss();

            console.log(data);

            if (data) {
              /**
               * El modal retorna number, name, selectedNodes y hours
               */
              
              // Se crea la automatización y una vez creada, se vuelve a pedir el dispositivo
              const loading = await this.loadingController.create({
                keyboardClose: true,
                cssClass: 'custom-loading',
              });
          
              loading.present();

              this.deviceService.newAutomation(
                this.deviceService.currentDeviceComId,
                this.partition.numero,
                data.number,
                data.name,
                "fototimer",
                data.selectedNodes,
                null,
                null,
                data.hours
              ).subscribe(
                () => {
                  this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
                },
                () => {
                  console.log("error al crear el fototimer");
                  loading.dismiss();
                }
              );
            }
          }
        },
        {
          text: "Programación horaria",
          cssClass: 'custom-action-sheet',
          handler: async () => {
            console.log("programación horaria")
          }
        },
        {
          text: "Simulador",
          cssClass: 'custom-action-sheet',
          handler: async () => {
            console.log("simulador")
          }
        }
      ]
    });

    await actionSheet.present();
  }

  onNodeMore(node: NodoAutomatizacion, event: Event) {
    this.showNodeMoreActionSheet(node);

    // Se evita que se propague el evento de click al item
    event.stopPropagation();
    return false;
  }

  private async showNodeMoreActionSheet(node: NodoAutomatizacion) {

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

    let actionEditNode = {
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
    };

    let actionDeleteNode = {
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
    };


    let actionEditAutomation = {
      text: "Editar automatización",
      handler: async () => {
        this.actionSheetController.dismiss();

        let modal;
        switch(node.tipo) {
          case "noche":
            modal = await this.modalController.create({
              component: NocheModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "number": node.numero,
                "name": node.nombre,
                "availableNodes": this.partition.nodos,
                "nodes": node.nodos
              }
            });

            break;

          case "fototimer":
            modal = await this.modalController.create({
              component: FototimerModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "number": node.numero,
                "name": node.nombre,
                "availableNodes": this.partition.nodos,
                "nodes": node.nodos,
                "hours": node.horas
              }
            });

            break;

          case "programacion_horaria":
              modal = await this.modalController.create({
                component: FototimerModalPage,
                cssClass: 'auto-height',
                handle: false,
                componentProps: {
                  "number": node.numero,
                  "name": node.nombre,
                  "availableNodes": this.partition.nodos,
                  "nodes": node.nodos,
                  "hours": node.horas,
                  "timeStart": node.horaInicio,
                  "timeEnd": node.horaFin
                }
              });
  
              break;

          case "simulador":
            modal = await this.modalController.create({
              component: FototimerModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "number": node.numero,
                "name": node.nombre,
                "availableNodes": this.partition.nodos,
                "nodes": node.nodos,
                "hours": node.horas,
                "timeStart": node.horaInicio,
                "timeEnd": node.horaFin
              }
            });

            break;
        }
        
    
        modal.present();
    
        const {data} = await modal.onWillDismiss();

        console.log(data);
        
        if (data) {
          /**
           * El modal retorna number, name, selectedNodes, type y [timeStart], [timeEnd] y [hours]
           */
          
          // Se modifica la automatización y una vez modificada, se vuelve a pedir el dispositivo
          const loading = await this.loadingController.create({
            keyboardClose: true,
            cssClass: 'custom-loading',
          });
      
          loading.present();

          this.deviceService.updateAutomation(
            this.deviceService.currentDeviceComId,
            this.partition.numero,
            data.number,
            data.name,
            data.type,
            data.selectedNodes,
            data.timeStart,
            data.timeEnd,
            data.hours
          ).subscribe(
            () => {
              this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
            },
            () => {
              console.log("error al modificar la automatización");
              loading.dismiss();
            }
          );
        }
      }
    };


    let actionDeleteAutomation = {
      text: "Eliminar automatización",
      handler: async () => {
        this.actionSheetController.dismiss();

        const modal = await this.modalController.create({
          component: YesNoModalPage,
          cssClass: 'auto-height',
          handle: false,
          componentProps: {
            "message": `¿Deseás eliminar la automatización ${node.nombre}?`,
            "yesText": "Eliminar",
            "noText": "Cancelar"
          }
        });
    
        modal.present();
    
        const {data} = await modal.onWillDismiss();
        
        if (data && data.result === "yes") {
          // Se elimina la automatizacion y una vez eliminada, se vuelve a pedir el dispositivo
          const loading = await this.loadingController.create({
            keyboardClose: true,
            cssClass: 'custom-loading',
          });
      
          loading.present();

          this.deviceService.removeAutomation(
            this.deviceService.currentDeviceComId,
            this.partition.numero,
            node.numero,
            node.tipo
          ).subscribe(
            () => {
              this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
            },
            () => {
              console.log("error al eliminar la automatización");
              loading.dismiss();
            }
          );
        }
      }
    };



    let buttons = [];

    if (node.tipo === "nodo") {
      buttons = [
        actionEditNode,
        actionDeleteNode,
        actionOnOff
      ];
    }
    else {
      buttons = [
        actionEditAutomation,
        actionDeleteAutomation
      ];
    }

    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      mode: "ios",
      buttons: buttons
    });

    await actionSheet.present();
  }
}
