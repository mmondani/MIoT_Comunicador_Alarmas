import { Component, Input, OnInit } from '@angular/core';
import { Nodo } from '../../../../models/nodo.model';
import { FormGroup, Validators, FormControl } from '@angular/forms';
import { ModalController } from '@ionic/angular';

@Component({
  selector: 'app-simulador-modal',
  templateUrl: './simulador-modal.page.html',
  styleUrls: ['./simulador-modal.page.scss'],
})
export class SimuladorModalPage implements OnInit {

  @Input() number: number;
  @Input() name?: string;
  @Input() nodes?: number[];
  @Input() availableNodes: Nodo[];
  form: FormGroup;

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
      }),
    });

    if (!this.nodes)
      this.selectedNodes = [];
    else {
      this.selectedNodes = [...this.nodes];
      
      // Si en los nodos hay 255, se los borra
      this.selectedNodes = this.selectedNodes.filter(node => node != 255);
    }
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
      type: "simulador"
    });
  }

  onCancel() {
    this.modalController.dismiss();
  }
}
