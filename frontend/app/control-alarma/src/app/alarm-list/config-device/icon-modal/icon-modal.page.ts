import { Component, OnInit } from '@angular/core';
import { ModalController } from '@ionic/angular';

@Component({
  selector: 'app-icon-modal',
  templateUrl: './icon-modal.page.html',
  styleUrls: ['./icon-modal.page.scss'],
})
export class IconModalPage implements OnInit {

  icons = [
    "alarm1",
    "alarm2",
    "alarm3",
    "alarm4",
    "alarm5",
    "alarm6"
  ]

  constructor(
    private modalController: ModalController
  ) { }

  ngOnInit() {
  }

  closeModal() {
    this.modalController.dismiss();
  }

  onIconClicked(icon: string) {
    this.modalController.dismiss({
      icon: icon
    });
  }
}
