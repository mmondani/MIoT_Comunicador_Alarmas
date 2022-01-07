import { Component, OnInit } from '@angular/core';

@Component({
  selector: 'app-advanced',
  templateUrl: './advanced.page.html',
  styleUrls: ['./advanced.page.scss'],
})
export class AdvancedPage implements OnInit {

  constructor() { }

  ngOnInit() {
  }

  onUsersClick() {
    console.log("Usuarios de la partición");
  }

  onConfiguationClick() {
    console.log("Configuración de la partición");
  }
}
