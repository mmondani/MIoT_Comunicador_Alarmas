import { Injectable } from '@angular/core';
import Amplify from 'aws-amplify';
import { AWSIoTProvider } from '@aws-amplify/pubsub/lib/Providers'; 
import awsexports from '../common/aws-exports' 
import { Subject, Subscription } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class MqttService {

  private deviceNewsSubscriptions = {};

  constructor() { 
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
          console.log('[MqttService] mensaje recibido:')                       
          console.log(JSON.stringify(data));
        },    
        error: error => console.error(error),    
        close: () => console.log('[MqttService] close'),  
      });
    }
  }

  unsubscribeToDeviceNews (comId: string) {
    if (this.deviceNewsSubscriptions[comId]) {
      this.deviceNewsSubscriptions[comId].unsubscribe();
      delete this.deviceNewsSubscriptions[comId];
    }
    
  }
}