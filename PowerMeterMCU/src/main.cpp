#include <Arduino.h>
#include <ConnectToApp.h>
#include <Helper.h>
#include <INA219.h>

#include "esp_log.h"

// #define ARDUHAL_LOG_LEVEL ESP_LOG_INFO

#define I2C_CFG_SDA 7
#define I2C_CFG_SCL 8
#define I2C_CFG_CLOCK 100000
#define I2C_CFG_INA_ADDR 0x40

#define LED_RED 3
#define LED_GREEN 4
#define LED_BLUE 5

#define RETRY_CONNECTION_MAX 10

#define m_SET_LED_RED digitalWrite(LED_RED, HIGH)
#define m_SET_LED_GREEN digitalWrite(LED_GREEN, HIGH)
#define m_SET_LED_BLUE digitalWrite(LED_BLUE, HIGH)

#define m_RESET_LED_RED digitalWrite(LED_RED, LOW)
#define m_RESET_LED_GREEN digitalWrite(LED_GREEN, LOW)
#define m_RESET_LED_BLUE digitalWrite(LED_BLUE, LOW)

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

enum e_Fault{
    FAULT_DEFAULT,
    FAULT_WIFI_NOT_CONNECTED,
    FAULT_HOST_NOT_REACHABLE,
    FAULT_COUNT
};

e_Fault currentFault = FAULT_DEFAULT;

void setup() 
{
    esp_log_level_set("wifi", ESP_LOG_ERROR);      // enable WARN logs from WiFi stack
    esp_log_level_set("dhcpc", ESP_LOG_ERROR);

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);

    wifi_connect.set_app_ptr(&app_interface);

    Serial.begin(115200);
    EEPROM.begin(100);

    currentState = STATE_STARTUP;

#ifndef INA219_ONLY_BUILD 
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
    while(WiFi.status() != WL_CONNECTED 
    && retry_count < RETRY_CONNECTION_MAX)
    {
        retry_count++;
        m_SET_LED_GREEN;
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
    m_RESET_LED_GREEN;

    IPAddress localIP = WiFi.localIP();
    log_i("Local IP: %d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);
    
    retry_count = 0;
    int8_t host_alive = -1;

    log_i("Try to reach host at %d.%d.%d.%d...", 
                                        hostIP[0], 
                                        hostIP[1], 
                                        hostIP[2], 
                                        hostIP[3]);
    while(host_alive == -1
    && retry_count < RETRY_CONNECTION_MAX)
    {
        retry_count++;
        m_SET_LED_RED;
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
    m_RESET_LED_RED;
#endif

#ifndef INA219_DISABLE
    sensor = new INA219(I2C_CFG_SDA, 
                        I2C_CFG_SCL, 
                        I2C_CFG_CLOCK, 
                        I2C_CFG_INA_ADDR);

    sensor->configure(INA219::NO_RESET, 
                    INA219::BRNG_32V, 
                    INA219::PGA_1, 
                    INA219::BADC_8AVG, 
                    INA219::SADC_8AVG, 
                    INA219::CNT_SHUNT_BUS);
    sensor->calibrate(0.01, 0.02, 2);
#endif
}

void loop() {
    currentState = STATE_NORMAL;

#ifndef INA219_DISABLE
    sensor->read_voltage();
    sensor->read_current();
    log_v("-------------------");
    log_v("Bus voltage:   %08.6f", sensor->voltage);
    log_v("Current:       %08.6f", sensor->current);
#endif

#ifndef INA219_ONLY_BUILD  
    int8_t wifi_status = WiFi.status();
    if(wifi_status == WL_CONNECTED)
    {
        m_RESET_LED_GREEN;

        int8_t host_alive = app_interface.checkHostAlive();

        if(host_alive == 0)
        {   
            m_RESET_LED_RED;
            
#ifndef INA219_DISABLE
            app_interface.setMeasurementPayload(sensor->current, sensor->voltage);
#else
            app_interface.setMeasurementPayload((esp_random()%11000)/1000.0, (esp_random()%37000)/1000.0);
#endif
            int16_t HTTP_code = app_interface.POST_measPayload("/esp32_post");
            log_e("Response code: %d", HTTP_code);
        }
        else
        {
            m_SET_LED_RED;
        }
    }
    else
    {
        m_SET_LED_GREEN;
    }
#endif


    delay(200);
}