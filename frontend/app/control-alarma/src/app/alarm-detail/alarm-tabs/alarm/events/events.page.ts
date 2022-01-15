import { Component, Input, OnDestroy, OnInit } from '@angular/core';
import { Subscription } from 'rxjs';
import { DeviceService } from '../../../../alarm-list/device.service';
import { Particion } from '../../../../models/particion.model';
import { EventoAlarma } from '../../../../models/evento-alarma.model';
import { ModalController } from '@ionic/angular';

@Component({
  selector: 'app-events',
  templateUrl: './events.page.html',
  styleUrls: ['./events.page.scss'],
})
export class EventsPage implements OnInit {

  @Input() events: EventoAlarma[];

  constructor(
    private modalController: ModalController
  ) { }

  ngOnInit() {
  }

  closeModal() {
    this.modalController.dismiss();
  }

}
