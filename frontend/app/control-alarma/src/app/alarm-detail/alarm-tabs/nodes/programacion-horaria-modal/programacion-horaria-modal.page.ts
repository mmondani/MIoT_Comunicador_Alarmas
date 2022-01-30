import { Component, Input, OnInit } from '@angular/core';
import { Nodo } from '../../../../models/nodo.model';
import { FormGroup, FormControl, Validators } from '@angular/forms';
import { ModalController } from '@ionic/angular';
import { format, formatISO, parseISO } from 'date-fns';

@Component({
  selector: 'app-programacion-horaria-modal',
  templateUrl: './programacion-horaria-modal.page.html',
  styleUrls: ['./programacion-horaria-modal.page.scss'],
})
export class ProgramacionHorariaModalPage implements OnInit {

  @Input() number: number;
  @Input() name?: string;
  @Input() nodes?: number[];
  @Input() timeStart?: string;
  @Input() timeEnd?: string;
  @Input() availableNodes: Nodo[];
  form: FormGroup;
  timeStartISO: string;
  timeEndISO: string;

  selectedNodes: number[];
  noMoreNodes: boolean;

  constructor(
    private modalController: ModalController
  ) { }

  ngOnInit() {
    /**
     * Si se provee el number, significa que es una edición de un elemento.
     * Si no se lo provee es una creación.
     */
    this.form = new FormGroup({
      name: new FormControl(this.name, {
        updateOn: "change",
        validators: [Validators.required]
      })
    });

    if (!this.nodes)
      this.selectedNodes = [];
    else {
      this.selectedNodes = [...this.nodes];

      // Si en los nodos hay 255, se los borra
      this.selectedNodes = this.selectedNodes.filter(node => node != 255);
    }
      

    // Se convierten las horas al formato ISO
    let timeStartElements = this.timeStart.split(":");
    this.timeStartISO = formatISO(new Date(2022, 1, 1, parseInt(timeStartElements[0]), parseInt(timeStartElements[1])));

    let timeEndElements = this.timeEnd.split(":");
    this.timeEndISO = formatISO(new Date(2022, 1, 1, parseInt(timeEndElements[0]), parseInt(timeEndElements[1])));

    console.log(this.timeStartISO);
  }


  onNodeCheboxClick(node: Nodo, event) {

    // Solo puede haber un máximo de 5 nodos seleccionados a la vez

    if (event.currentTarget.checked) {
      if (this.selectedNodes.length < 5)
        this.selectedNodes.push(node.numero);
      else
        event.currentTarget.checked = false;
    }
    else {
      let index = this.selectedNodes.indexOf(node.numero);

      if (index >= 0)
        this.selectedNodes.splice(index, 1);
    }
  }


  onOk() {
    this.modalController.dismiss({
      number: this.number,
      name: this.form.value.name,
      selectedNodes: this.selectedNodes,
      timeStart: this.timeStart,
      timeEnd: this.timeEnd,
      type: "programacion_horaria"
    });
  }

  onCancel() {
    this.modalController.dismiss();
  }

  formatTimeStart(value: string) {
    this.timeStartISO = value;
    this.timeStart = format(parseISO(value), 'HH:mm'); 
  }

  formatTimeEnd(value: string) {
    this.timeEndISO = value;
    this.timeEnd = format(parseISO(value), 'HH:mm'); 
  }
}
