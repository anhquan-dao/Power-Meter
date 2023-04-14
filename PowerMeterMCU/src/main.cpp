#include <Arduino.h>
#include <ConnectToApp.h>
#include <Helper.h>

#include "esp_log.h"

// #define ARDUHAL_LOG_LEVEL ESP_LOG_INFO

// initializing WifiConnectPtr with NULL
WifiConnect* WifiConnect::WifiConnectPtr = NULL;
WifiConnect &wifi_connect = *WifiConnect::getInstance();

AppInterface app_interface;

// char ssid[32];
// char pass[32];

// Dorm network
char ssid[] = "Phong 318";
char pass[] = "conddixdduwjctelecom";

// Home network
// char ssid[32] = "Quan Khoa";
// char pass[32] = "Gau_Me_Beo";

// Lab network
// char ssid[] = "DT2000";
// char pass[] = "myPMS5003"; 

// Personal network
// char ssid[] = "Enso cafe1";
// char pass[] = "Enso12346789";


uint8_t localIP[4];
uint8_t hostIP[4] = {192, 168, 1, 8};
char host_IP[] = "192.168.1.8";

void setup() 
{   
    Serial.begin(115200);
    // EEPROM.begin(100);
    // device_cfg.readMemoryById(DeviceCfg::WIFISSID, (int8_t*)ssid, 32);
    // device_cfg.readMemoryById(DeviceCfg::WIFIPASS, (int8_t*)pass, 32);
    log_e("Attempt to connect to %s with pass %s", ssid, pass);
    wifi_connect.set_STA_parameter(ssid, pass);
    wifi_connect.start_connection();
}

void loop() {
    int8_t wifi_status = wifi_connect.get_status();
    
    if(wifi_status == WL_CONNECTED)
    {
        wifi_connect.get_local_IP(localIP, 4);
        log_i("Local IP: %d.%d.%d.%d\n", localIP[0], localIP[1], localIP[2], localIP[3]);
        
        int8_t host_alive = app_interface.setHost(hostIP);

        log_i("Set host IP: %d.%d.%d.%d. Result: %d\n", 
            hostIP[0], hostIP[1], hostIP[2], hostIP[3], 
            host_alive);
        
        app_interface.setMeasurementPayload((esp_random()%11000)/1000.0, (esp_random()%37000)/1000.0);

        if(host_alive >= 0)
        {
            // log_i(app_interface.POST_measPayload("/esp32_post"));
        }
    }
    else
    {
        log_e("Connection status: %s",wifi_connect.wifiStatus_struct[wifi_status].name);
    }
    
    
    delay(1000);
}