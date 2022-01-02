import { Component, OnInit } from '@angular/core';
import { Subscription } from 'rxjs';
import { DeviceService } from '../../../alarm-list/device.service';
import { Particion } from '../../../models/particion.model';

@Component({
  selector: 'app-nodes',
  templateUrl: './nodes.page.html',
  styleUrls: ['./nodes.page.scss'],
})
export class NodesPage implements OnInit {

  partition: Particion;
  private partitionSubscription: Subscription;

  constructor(private deviceService: DeviceService) { }

  ngOnInit() {
    this.partitionSubscription = this.deviceService.currentPartition.subscribe( partition => {
        this.partition = partition;
      }
    );
  }

  ngOnDestroy(): void {
    if (this.partitionSubscription)
      this.partitionSubscription.unsubscribe();
  }

}
