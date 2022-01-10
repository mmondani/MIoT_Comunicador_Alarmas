import { Injectable } from '@angular/core';
import Amplify from 'aws-amplify';
import { AWSIoTProvider } from '@aws-amplify/pubsub/lib/Providers'; 
import awsexports from '../common/aws-exports' 
import { Subject, Subscription } from 'rxjs';
import { DeviceService } from '../alarm-list/device.service';

@Injectable({
  providedIn: 'root'
})
export class MqttService {

  private deviceNewsSubscriptions = {};
  private deviceNewsTimers = {};

  constructor(
    private deviceService: DeviceService
  ) { 
    Amplify.configure(awsexports);
    Amplify.addPluggable(new AWSIoTProvider({  
      aws_pubsub_region: 'sa-east-1',  
      aws_pubsub_endpoint: 'wss://broker.com38-cloud.com/mqtt',
      clientId: `app-${Math.floor((Math.random() * 1000000) + 1)}`
    }));

  }

  subscribeToDeviceNews (comId: string) {
    // Solo se suscribe si no hay una suscripción anterior asociada a ese comID
    if (!this.deviceNewsSubscriptions[comId]) {
      this.deviceNewsSubscriptions[comId] = Amplify.PubSub.subscribe(`${comId}/new`).subscribe({    
        next: data => {  
          console.log(`[MqttService] mensaje recibido (${comId}):`);                     
          //console.log(JSON.stringify(data));

          /**
           * Si llega una novedad se vuelve a pedir la información al dispositivo
           * con un tiempo de integración de eventos para evitar múltiples pedidos
           */
          if (this.deviceNewsTimers[comId])
            clearTimeout(this.deviceNewsTimers[comId]);
          
          this.deviceNewsTimers[comId] = setTimeout(() => {
            this.deviceService.getDevice(comId).subscribe(()=>{});
          }, 4000);
        },    
        error: error => console.error("[MqttService]" + error),    
        close: () => console.log('[MqttService] close'),  
      });
    }
  }

  unsubscribeToDeviceNews (comId: string) {
    if (this.deviceNewsSubscriptions[comId]) {
      this.deviceNewsSubscriptions[comId].unsubscribe();
      delete this.deviceNewsSubscriptions[comId];
      delete this.deviceNewsTimers[comId];
    }
    
  }
}