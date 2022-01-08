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
  @Input() name: string;
  form: FormGroup;

  constructor(private modalController: ModalController) { }

  ngOnInit() {
    console.log(`${this.number} - ${this.name}`);

    this.form = new FormGroup({
      number: new FormControl(this.number, {
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
      number: this.form.value.number,
      name: this.form.value.name
    });
  }

  onCancel() {
    this.modalController.dismiss();
  }
}
