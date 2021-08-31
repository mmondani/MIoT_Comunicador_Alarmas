#ifndef WIFIMANAGER_H_
#define WIFIMANAGER_H_

#include "asf.h"
#include "common/include/nm_common.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"
#include "debounce.h"
#include "basicDefinitions.h"


#define WIFI_MANAGER_WIFI_DISCONNECTED		M2M_WIFI_DISCONNECTED
#define WIFI_MANAGER_WIFI_CONNECTED			M2M_WIFI_CONNECTED


void wifiManager_init (debouncePin_t* button);
void wifiManager_deinit (void);
uint32_t wifiManager_isWiFiConnected (void);
uint32_t wifiManager_isProvisioningEnable (void);
void wifiManager_initWps (u8 wpsTrigger, const u8* pinNumber);
void wifiManager_stopWps (void);
void wifiManager_ProvisioningFsmHandler (void);
void wifiManager_provideProvisioningInfo (uint8_t* ssid, uint8_t* pass);


#endif /* WIFIMANAGER_H_ */