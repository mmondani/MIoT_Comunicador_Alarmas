import { Component, Input, OnInit } from '@angular/core';
import { FormGroup, FormControl, Validators } from '@angular/forms';
import { ModalController, LoadingController } from '@ionic/angular';
import { IconModalPage } from './icon-modal/icon-modal.page';
import { Router, ActivatedRoute } from '@angular/router';
import { DeviceService } from '../device.service';
import { CommandsService } from '../../services/commands.service';

@Component({
  selector: 'app-config-device',
  templateUrl: './config-device.page.html',
  styleUrls: ['./config-device.page.scss'],
})
export class ConfigDevicePage implements OnInit {

  comId: string;
  icon: string;
  name: string;
  codeM: string;
  codeH: string;
  timeZone: number;
  timeZoneName: string;

  form: FormGroup;
  showCodeM = false;
  codeMToggleIcon = "eye";
  showCodeH = false;
  codeHToggleIcon = "eye";

  timeZones = [
    {code: 1, name: "Ciudad de Buenos Aires"},
    {code: 2, name: "Buenos Aires"},
    {code: 3, name: "Santa Fe"},
    {code: 4, name: "Córdoba"},
    {code: 5, name: "Entre Ríos"},
    {code: 6, name: "Corrientes"},
  ];

  constructor(
    private activatedRoute: ActivatedRoute,
    private router: Router,
    private modalController: ModalController,
    private deviceService: DeviceService,
    private commandsService: CommandsService,
    private loadingController: LoadingController
  ) { 
    /**
     * Se recuperan los parámetros que se pasan, si los hay
     */
    this.activatedRoute.queryParamMap.subscribe(params => {

      if (this.router.getCurrentNavigation().extras.state) {
        this.icon = this.router.getCurrentNavigation().extras.state.icon;
        this.name = this.router.getCurrentNavigation().extras.state.name;
        this.timeZone = this.router.getCurrentNavigation().extras.state.timeZone;
      }
    })
  }

  ngOnInit() {
    this.comId = this.activatedRoute.snapshot.params["id"];

    /**
     * Si se pasa como Input el time zone, se busca el nombre
     * Si no se pasa se carga uno por default
     */
    if (this.timeZone) {
      this.timeZones.forEach(tz => {
        if (tz.code === this.timeZone)
          this.timeZoneName = tz.name;
      })
    }
    else {
      this.timeZone = 1;
      this.timeZoneName = "Ciudad de Buenos Aires"
    }

    /**
     * Si no se pasa un ícono se carga un default
     */
    if (!this.icon)
      this.icon = "alarm1";


    this.form = new FormGroup({
      icon: new FormControl(this.icon, {
        updateOn: 'change',
        validators: [Validators.required]
      }),
      name: new FormControl(this.name? this.name : null, {
        updateOn: 'change',
        validators: [Validators.required]
      }),
      codeM: new FormControl(this.codeM? this.codeM : null, {
        updateOn: 'change',
      }),
      codeH: new FormControl(this.codeH? this.codeH : null, {
        updateOn: 'change',
      }),
      timeZone: new FormControl(this.timeZone? this.timeZone : null, {
        updateOn: 'change',
        validators: [Validators.required]
      })
    });
  }

  async onIconClicked() {
    const modal = await this.modalController.create({
      component: IconModalPage
    });

    modal.present();
        
    const {data} = await modal.onWillDismiss();


    if (data) {
      this.icon = data.icon;
      this.form.patchValue({icon: data.icon});
    }
  }

  onToggleCodeM () {
    this.showCodeM = !this.showCodeM;

    if (this.codeMToggleIcon === "eye")
      this.codeMToggleIcon = "eye-off";
    else
      this.codeMToggleIcon = "eye";
  }

  onToggleCodeH () {
    this.showCodeH = !this.showCodeH;

    if (this.codeHToggleIcon === "eye")
      this.codeHToggleIcon = "eye-off";
    else
      this.codeHToggleIcon = "eye";
  }

  async onSaveConfig() {
    const loading = await this.loadingController.create({
      keyboardClose: true,
      message: "Actualizando"
    });

    loading.present();

    this.deviceService.patchDevice(
      this.comId,
      this.form.value.name,
      this.form.value.icon,
      this.form.value.codeM? this.form.value.codeM : undefined,
      this.form.value.codeH? this.form.value.codeH : undefined
    ).subscribe(async () => {
      loading.dismiss();

      /**
       * Se pide el dispositivo al backend para traerlo actualizado
       */
      const loading2 = await this.loadingController.create({
        keyboardClose: true,
        message: "Actualizando"
      });

      loading2.present();

      this.deviceService.getDevice(this.comId).subscribe(() => {
        loading2.dismiss();

        this.router.navigateByUrl("/");
      }, () => {
        loading2.dismiss();
      });
    }, () => {
      loading.dismiss();
    });
  }

}
