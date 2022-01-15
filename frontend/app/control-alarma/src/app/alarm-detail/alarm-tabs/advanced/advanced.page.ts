import { Component, OnDestroy, OnInit } from '@angular/core';
import { Subscription, partition } from 'rxjs';
import { Particion } from '../../../models/particion.model';
import { DeviceService } from '../../../alarm-list/device.service';
import { UsuarioAlarma } from '../../../models/usuario-alarma.model';
import { ActionSheetController, ModalController, LoadingController } from '@ionic/angular';
import { YesNoModalPage } from '../../../yes-no-modal/yes-no-modal.page';
import { UserModalPage } from './user-modal/user-modal.page';
import { Storage } from '@capacitor/storage';

@Component({
  selector: 'app-advanced',
  templateUrl: './advanced.page.html',
  styleUrls: ['./advanced.page.scss'],
})
export class AdvancedPage implements OnInit, OnDestroy {

  showPassword = false;
  passwordToggleIcon = "eye";
  partition: Particion;
  alarmCode: string = "";
  private partitionSubscription: Subscription;
  private availableUsers: number[];


  constructor(
    private deviceService: DeviceService,
    private actionSheetController: ActionSheetController,
    private modalController: ModalController,
    private loadingController: LoadingController
  ) { }

  ngOnInit() {
    this.partitionSubscription = this.deviceService.currentPartition.subscribe( partition => {
      this.partition = partition;

      // El array availableUsers tiene los números de usuario que no están usados
      this.availableUsers = Array.from({length: 32}, (x, i) => i+1);

      // Se ordenan los usuarios por número de usuario
      this.partition.usuariosAlarma.sort((a, b) => {return a.numero-b.numero});

      // Se eliminan los números de usuario que no están disponibles
      this.partition.usuariosAlarma.forEach(usuarioAlarma => {
        let availableZoneIndex = this.availableUsers.findIndex((zoneNumber) => zoneNumber === usuarioAlarma.numero)
        this.availableUsers.splice(availableZoneIndex, 1);
      });
    });
  }

  ngOnDestroy(): void {
    if (this.partitionSubscription)
      this.partitionSubscription.unsubscribe();
  }


  async ionViewDidEnter() {
    // Se busca si hay un código de la alarma cargado para este equipo y esta partición
    let storedData = await Storage.get({key: this.deviceService.currentDeviceComId})
    let codes: {particion: number, codigo: string}[];

    if (storedData && storedData.value) {
      codes = JSON.parse(storedData.value);
      codes.forEach(code => {
        if (code.particion === this.deviceService.currentPartitionNumber) {
          this.alarmCode = code.codigo;
        }
      });
    }
  }


  onTogglePassword () {
    this.showPassword = !this.showPassword;

    if (this.passwordToggleIcon === "eye")
      this.passwordToggleIcon = "eye-off";
    else
      this.passwordToggleIcon = "eye";
  }

  async onAddUser() {
    const modal = await this.modalController.create({
      component: UserModalPage,
      cssClass: 'auto-height',
      handle: false,
      componentProps: {
        "availableUsers": this.availableUsers,
        "name": ""
      }
    });

    modal.present();

    const {data} = await modal.onWillDismiss();
    
    if (data) {
      // Se crea un nuevo usuario de la alarma y una vez creado, se vuelve a pedir el dispositivo
      const loading = await this.loadingController.create({
        keyboardClose: true,
        cssClass: 'custom-loading',
      });
  
      loading.present();

      this.deviceService.newAlarmUser(
        this.deviceService.currentDeviceComId,
        this.partition.numero,
        data.number,
        data.name
      ).subscribe(
        () => {
          this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
        },
        () => {
          console.log("error al crear el usuario de la alarma");
          loading.dismiss();
        }
      );
      
    }
  }

  onUserMore(user: UsuarioAlarma, event: Event) {
    this.showUserMoreActionSheet(user);

    // Se evita que se propague el evento de click a la card
    event.stopPropagation();
    return false;
  }

  private async showUserMoreActionSheet(user: UsuarioAlarma) {
    const actionSheet = await this.actionSheetController.create({
      cssClass: "action-sheet",
      mode: "ios",
      buttons: [
        {
          text: "Editar Usuario",
          handler: async () => {
            this.actionSheetController.dismiss();

            const modal = await this.modalController.create({
              component: UserModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "number": user.numero,
                "availableUsers": this.availableUsers,
                "name": user.nombre
              }
            });
        
            modal.present();
        
            const {data} = await modal.onWillDismiss()
            
            if (data) {
              // Se modifica el usuario de la alarma y una vez modificado, se vuelve a pedir el dispositivo
              const loading = await this.loadingController.create({
                keyboardClose: true,
                cssClass: 'custom-loading',
              });
          
              loading.present();

              this.deviceService.updateAlarmUser(
                this.deviceService.currentDeviceComId,
                this.partition.numero,
                data.number,
                data.name
              ).subscribe(
                () => {
                  this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
                },
                () => {
                  console.log("error al modificar el usuario de la alarma");
                  loading.dismiss();
                }
              );
              
            }
          }
        },
        {
          text: "Eliminar usuario",
          handler: async () => {
            this.actionSheetController.dismiss();

            const modal = await this.modalController.create({
              component: YesNoModalPage,
              cssClass: 'auto-height',
              handle: false,
              componentProps: {
                "message": `¿Deseás eliminar el usuario ${user.nombre}?`,
                "yesText": "Eliminar",
                "noText": "Cancelar"
              }
            });
        
            modal.present();
        
            const {data} = await modal.onWillDismiss()
            
            if (data && data.result === "yes") {
              // Se elimina el usuario de la alarma y una vez eliminado, se vuelve a pedir el dispositivo
              const loading = await this.loadingController.create({
                keyboardClose: true,
                cssClass: 'custom-loading',
              });
          
              loading.present();

              this.deviceService.removeAlarmUser(
                this.deviceService.currentDeviceComId,
                this.partition.numero,
                user.numero
              ).subscribe(
                () => {
                  this.deviceService.getDevice(this.deviceService.currentDeviceComId).subscribe(() => loading.dismiss());
                },
                () => {
                  console.log("error al eliminar el usuario de la alarma");
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

  async onSaveCodeClick() {
    /**
     * Se actualiza la información que hay en el localstorage
     * Se la guarda usando como key el ID del comunicador
     */
    
    let storedData = await Storage.get({key: this.deviceService.currentDeviceComId})
    let codes: {particion: number, codigo: string}[];

    if (!storedData || !storedData.value)
      codes = [];
    else
      codes = JSON.parse(storedData.value);
    
    let modified = false;
    codes.forEach(code => {
      if (code.particion === this.deviceService.currentPartitionNumber) {
        code.codigo = this.alarmCode;
        modified = true;
      }
    });

    if (!modified) {
      codes.push({
        particion: this.deviceService.currentPartitionNumber,
        codigo: this.alarmCode 
      });
    }

    console.log(JSON.stringify(codes));

    /**
     * Se guardan los códigos de este comunicador en el local storage
     */
    Storage.set({
      key: this.deviceService.currentDeviceComId,
      value: JSON.stringify(codes)
    })
  }
}
