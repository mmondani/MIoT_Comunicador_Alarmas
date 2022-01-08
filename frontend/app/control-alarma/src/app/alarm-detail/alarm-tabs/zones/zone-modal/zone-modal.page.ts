import { Component, Input, OnInit } from '@angular/core';
import { ModalController } from '@ionic/angular';
import { FormGroup, FormControl, Validators } from '@angular/forms';

@Component({
  selector: 'app-zone-modal',
  templateUrl: './zone-modal.page.html',
  styleUrls: ['./zone-modal.page.scss'],
})
export class ZoneModalPage implements OnInit {

  @Input() number: number;
  @Input() name?: string;
  @Input() availableZones?: number[];
  form: FormGroup;

  constructor(private modalController: ModalController) { }

  ngOnInit() {
    /**
     * Si se provee el number, significa que es una edición de un elemento.
     * En este caso no se puede editar el número.
     * Si no se lo provee es una creación.
     */
    this.form = new FormGroup({
      number: new FormControl({
        value: this.number? this.number : this.availableZones[0],
        disabled: this.number? true : false
      }, {
        updateOn: "change",
        validators: [Validators.required]
      }),
      name: new FormControl(this.name, {
        updateOn: "change",
        validators: [Validators.required]
      }),
    });

  }


  onOk() {
    this.modalController.dismiss({
      number: this.number? this.number : this.form.value.number,
      name: this.form.value.name
    });
  }

  onCancel() {
    this.modalController.dismiss();
  }
}
