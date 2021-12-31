import { Component, OnInit } from '@angular/core';
import { Device } from '../models/device.model';
import { DeviceService } from '../alarm-list/device.service';
import { ActivatedRoute } from '@angular/router';

@Component({
  selector: 'app-alarm-detail',
  templateUrl: './alarm-detail.page.html',
  styleUrls: ['./alarm-detail.page.scss'],
})
export class AlarmDetailPage implements OnInit {

  device: Device;

  constructor(
    private activatedRoute: ActivatedRoute,
    private deviceService: DeviceService
  ) { }

  ngOnInit() {
    this.deviceService.getDeviceItem(this.activatedRoute.snapshot.params["id"])
      .subscribe(device => {
        this.device = device;
      })
  }

}
