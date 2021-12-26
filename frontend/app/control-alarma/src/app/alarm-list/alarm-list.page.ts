import { Component, OnInit } from '@angular/core';
import { AuthService } from '../login/auth.service';

@Component({
  selector: 'app-alarm-list',
  templateUrl: './alarm-list.page.html',
  styleUrls: ['./alarm-list.page.scss'],
})
export class AlarmListPage implements OnInit {

  constructor(private authService: AuthService) { }

  ngOnInit() {
  }

  onLogout() {
    this.authService.logout();
  }
}
