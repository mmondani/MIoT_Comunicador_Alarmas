import { Component, OnInit } from '@angular/core';
import { Device } from '../models/device.model';
import { DeviceService } from '../alarm-list/device.service';
import { ActivatedRoute } from '@angular/router';
import { NavController } from '@ionic/angular';

@Component({
  selector: 'app-alarm-detail',
  templateUrl: './alarm-detail.page.html',
  styleUrls: ['./alarm-detail.page.scss'],
})
export class AlarmDetailPage implements OnInit {

  deviceComId: string;
  partitionNumber: number;
  currentPartitionIndex: number;
  deviceName: string;
  partitionNumbers: number[];
  currentPartitionName: string;

  constructor(
    private activatedRoute: ActivatedRoute,
    private deviceService: DeviceService,
    private navController: NavController
  ) { }

  ngOnInit() {

  }

  ionViewDidEnter() {
    this.deviceComId = this.activatedRoute.snapshot.params["id"];
    this.partitionNumber = parseInt(this.activatedRoute.snapshot.params["partition"]);

    this.deviceService.currentDeviceComId = this.deviceComId;
    this.deviceService.currentPartitionNumber = this.partitionNumber;

    this.deviceService.getDeviceName(this.deviceComId)
      .subscribe(deviceName => {
        this.deviceName = deviceName;
      })

    this.deviceService.getDevicePartitionNumbers(this.deviceComId)
      .subscribe(partitionNumbers => {
        this.partitionNumbers = partitionNumbers;
        this.currentPartitionIndex = this.getCurrentPartitionIndex();
      });

    this.deviceService.getDevicePartitionName(this.deviceComId, this.partitionNumber)
      .subscribe(partitionName => {
        this.currentPartitionName = partitionName;
      })
  }


  hasPrevious() {
    if (this.currentPartitionIndex > 0)
      return true;
    else
      return false;
  }

  hasNext() {
    if (this.currentPartitionIndex < (this.partitionNumbers.length - 1))
      return true;
    else
      return false;
  }

  onPreviousClick() {
    this.currentPartitionIndex --;
    this.navController.navigateForward(['alarm-detail', this.deviceComId, this.partitionNumbers[this.currentPartitionIndex]], {animated: false})
  }

  onNextClick() {
    this.currentPartitionIndex ++;
    this.navController.navigateForward(['alarm-detail', this.deviceComId, this.partitionNumbers[this.currentPartitionIndex]], {animated: false})
  }


  private getCurrentPartitionIndex() {
    let index = 0;

    for (let i = 0; i < this.partitionNumbers.length; i++) {
      if (this.partitionNumbers[i] === this.partitionNumber)
        break;
      else 
        index ++;
    }

    return index;
  }
}
