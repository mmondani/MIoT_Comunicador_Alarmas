import { Component, Input, OnInit } from '@angular/core';
import { ModalController } from '@ionic/angular';

@Component({
  selector: 'app-qr-modal',
  templateUrl: './qr-modal.page.html',
  styleUrls: ['./qr-modal.page.scss'],
})
export class QrModalPage implements OnInit {

  @Input() comId: string;

  constructor(
    private modalController: ModalController
  ) { }

  ngOnInit() {
  }


  onClick() {
    this.modalController.dismiss();
  }
}
