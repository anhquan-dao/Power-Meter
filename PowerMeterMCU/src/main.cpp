#include <Arduino.h>
#include <ConnectToApp.h>
#include <Helper.h>
#include <INA219.h>
#include <FaultLED.h>

#include "esp_log.h"

#define I2C_CFG_SDA 7
#define I2C_CFG_SCL 8
#define I2C_CFG_CLOCK 100000
#define I2C_CFG_INA_ADDR 0x40

#define LED_RED 3
#define LED_GREEN 4
#define LED_BLUE 5

#define RETRY_CONNECTION_MAX 10

WifiConnect* WifiConnect::WifiConnectPtr = NULL;
WifiConnect &wifi_connect = *WifiConnect::getInstance();

AppInterface app_interface;

INA219 *sensor;

enum e_State{
    STATE_STARTUP,
    STATE_NORMAL,
    STATE_COUNT
};

e_State currentState = STATE_STARTUP;

typedef enum e_Fault{
    FAULT_DEFAULT,
    FAULT_WIFI_NOT_CONNECTED,
    FAULT_HOST_NOT_REACHABLE,
    FAULT_COUNT
}t_Fault;

t_Fault currentFault = FAULT_DEFAULT;

FaultLED *x_faultLED;
t_faultSource x_wifiFault = {.o_faultLED = LED_GREEN};
t_faultSource x_appFault = {.o_faultLED = LED_RED};
t_faultSource x_inaFault = {.o_faultLED = LED_BLUE};
t_faultSource *ax_faultConfig[] = {&x_wifiFault, &x_appFault, &x_inaFault};

void setup() 
{
    Serial.begin(115200);
    Serial.printf("Hello %d\n", CONFIG_DISABLE_HAL_LOCKS);

    esp_log_level_set("wifi", ESP_LOG_ERROR);      // enable WARN logs from WiFi stack
    esp_log_level_set("dhcpc", ESP_LOG_ERROR);

    x_faultLED = new FaultLED(ax_faultConfig, 3);

    wifi_connect.set_app_ptr(&app_interface);

    EEPROM.begin(100);

    /**
     * @brief Configure and calibration routine INAxxx sensor
     */ 
#ifndef INA219_DISABLE
    sensor = new INA219(I2C_CFG_SDA, 
                        I2C_CFG_SCL, 
                        I2C_CFG_CLOCK, 
                        I2C_CFG_INA_ADDR);

    x_inaFault.o_faultFlag = 1;
    if(sensor->configure(   INA219::NO_RESET, 
                            INA219::BRNG_32V, 
                            INA219::PGA_8, 
                            INA219::BADC_8AVG, 
                            INA219::SADC_8AVG, 
                            INA219::CNT_SHUNT_BUS) != 0)
    {
        while(true)
        {
            delay(1000);
        }
    }
    delay(1000);
    x_inaFault.o_faultFlag = 3;
    if(sensor->calibrate(0.02, 0.2, 10) != 0)
    {
        while(true)
        {
            delay(1000);
        }
    }
    delay(1000);
    x_inaFault.o_faultFlag = 0;
    // sensor->start();
#endif

    currentState = STATE_STARTUP;

    /**
     * @brief Connect to a WiFi STA
     */ 
    IPAddress host_address;
#ifdef HOST_IP
    host_address.fromString(HOST_IP);
#else
    String str_host_ip;
    device_cfg.readMemoryById(DeviceCfg::HOSTIP, (int8_t*)str_host_ip.begin(), 32);
    host_address.fromString(str_host_ip);
#endif

    uint8_t hostIP[4] = {host_address[0], 
                        host_address[1], 
                        host_address[2], 
                        host_address[3]};

    char ssid[32];
    char pass[32];

#if defined(STA_SSID) && defined(STA_PASS)
    memcpy(ssid, &STA_SSID, sizeof(STA_SSID)/sizeof(STA_SSID[0]));
    memcpy(pass, &STA_PASS, sizeof(STA_PASS)/sizeof(STA_PASS[0]));
#else
    device_cfg.readMemoryById(DeviceCfg::WIFISSID, (int8_t*)ssid, 32);
    device_cfg.readMemoryById(DeviceCfg::WIFIPASS, (int8_t*)pass, 32); 
#endif
    log_i("Attempt to connect to %s with pass %s", ssid, pass);
    wifi_connect.set_STA_parameter(ssid, pass);
    wifi_connect.start_connection();
    
    uint8_t retry_count=0;
    x_wifiFault.o_faultFlag = 1;
    while(WiFi.status() != WL_CONNECTED 
    && retry_count < RETRY_CONNECTION_MAX)
    {
        retry_count++;
        delay(1000);
        
    }
    if(retry_count >= RETRY_CONNECTION_MAX)
    {
        WiFi.mode(WIFI_AP);
        log_e("Failed to connect to %s", ssid);
        log_e("Disable STA mode. Please access the device AP to reconfigure STA information");
        currentFault = FAULT_WIFI_NOT_CONNECTED;
        
        while(true)
        {
            delay(1000);
        }
    }
    x_wifiFault.o_faultFlag = 0;

    /**
     * @brief Connect to the saved host compute IP
     */ 
    IPAddress localIP = WiFi.localIP();
    log_i("Local IP: %d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);
    
    retry_count = 0;
    int8_t host_alive = -1;

    log_i("Try to reach host at %d.%d.%d.%d...", 
                                        hostIP[0], 
                                        hostIP[1], 
                                        hostIP[2], 
                                        hostIP[3]);
    x_appFault.o_faultFlag = 1;
    while(host_alive == -1
    && retry_count < RETRY_CONNECTION_MAX)
    {
        retry_count++;
        host_alive = app_interface.setHost(hostIP, 80);
        delay(100);
    }
    if(retry_count >= RETRY_CONNECTION_MAX)
    {   
        log_e("Cannot reach host at %d.%d.%d.%d.", 
                                        hostIP[0], 
                                        hostIP[1], 
                                        hostIP[2], 
                                        hostIP[3]);
        log_e("Please access %s:80 to reconfigure Host information", localIP.toString());
        currentFault = FAULT_HOST_NOT_REACHABLE;
        
        while(true)
        {
            delay(1000);
        }
    }
    x_appFault.o_faultFlag = 0;

}

void loop() {
#ifndef INA219_ONLY_BUILD  
    int8_t wifi_status = WiFi.status();
    if(wifi_status == WL_CONNECTED)
    {
        x_wifiFault.o_faultFlag = 0;

#ifndef INA219_DISABLE
        INA219::t_messageSensor x_sensorData;
        sensor->read_current();
        sensor->read_voltage();
        x_sensorData.current = sensor->current;
        x_sensorData.voltage = sensor->voltage;
        app_interface.setMeasurementPayload(x_sensorData.current, x_sensorData.voltage);
#else
        app_interface.setMeasurementPayload((esp_random()%11000)/1000.0, (esp_random()%37000)/1000.0);
#endif 
        int16_t UDP_code = app_interface.UDP_measPayload();
        if(UDP_code != 1)
        {
            app_interface.checkHostAlive() == 0 ? x_appFault.o_faultFlag=0 : x_appFault.o_faultFlag=1;
        }
        else
        {
            x_appFault.o_faultFlag=0;
        }
     
    }
    else
    {
        x_wifiFault.o_faultFlag=1 ;
    }
    vTaskDelay(100);
#endif
}