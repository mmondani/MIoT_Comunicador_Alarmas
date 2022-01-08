import { Component, Input, OnInit } from '@angular/core';
import { ModalController } from '@ionic/angular';

@Component({
  selector: 'app-yes-no-modal',
  templateUrl: './yes-no-modal.page.html',
  styleUrls: ['./yes-no-modal.page.scss'],
})
export class YesNoModalPage implements OnInit {

  @Input() message: string;
  @Input() yesText: string;
  @Input() noText: string;

  constructor(private modalController: ModalController) { }

  ngOnInit() {
  }


  onYes() {
    this.modalController.dismiss({
      result: "yes"
    });
  }

  onNo() {
    this.modalController.dismiss({
      result: "no"
    });
  }

}
