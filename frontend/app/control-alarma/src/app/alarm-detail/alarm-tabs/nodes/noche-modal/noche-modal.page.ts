import { Component, Input, OnInit } from '@angular/core';
import { FormGroup, FormControl, Validators } from '@angular/forms';
import { ModalController } from '@ionic/angular';
import { Nodo } from '../../../../models/nodo.model';

@Component({
  selector: 'app-noche-modal',
  templateUrl: './noche-modal.page.html',
  styleUrls: ['./noche-modal.page.scss'],
})
export class NocheModalPage implements OnInit {
  
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
      type: "noche"
    });
  }

  onCancel() {
    this.modalController.dismiss();
  }
}
