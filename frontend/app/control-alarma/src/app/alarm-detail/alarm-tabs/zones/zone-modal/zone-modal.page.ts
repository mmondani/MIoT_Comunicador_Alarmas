import { Component, Input, OnInit } from '@angular/core';
import { ModalController } from '@ionic/angular';

@Component({
  selector: 'app-zone-modal',
  templateUrl: './zone-modal.page.html',
  styleUrls: ['./zone-modal.page.scss'],
})
export class ZoneModalPage implements OnInit {

  @Input() number: number;
  @Input() name: string;

  constructor(private modalController: ModalController) { }

  ngOnInit() {
    console.log(`${this.number} - ${this.name}`);
  }


  onClick() {
    this.modalController.dismiss({
      number: this.number,
      name: this.name
    });
  }
}
