#include <Arduino.h>
#include <Helper.h>
#include <FaultLED.h>
#include <main.h>

#include "esp_log.h"

#define I2C_CFG_SDA 7
#define I2C_CFG_SCL 8
#define I2C_CFG_CLOCK 100000
#define I2C_CFG_INA_ADDR 0x40

#define LED_RED 3
#define LED_GREEN 4
#define LED_BLUE 5

#define RETRY_CONNECTION_MAX 10

SemaphoreHandle_t x_maxMeasSemp = xSemaphoreCreateBinary();
t_sensorMaxConfig x_sensorMaxConfig = m_INIT_SENSOR_MAX_CONFIG;
QueueHandle_t x_sensorDataQueue = xQueueCreate(4, sizeof(INA226::t_messageSensor));;

WifiConnect* WifiConnect::WifiConnectPtr = NULL;
WifiConnect &wifi_connect = *WifiConnect::getInstance();

AppInterface app_interface;

INA226 *sensor;

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
t_faultSource x_inaFault = {.o_faultLED = LED_BLUE};
t_faultSource *ax_faultConfig[] = {&x_wifiFault, &x_inaFault};

TaskHandle_t x_readSensor;
TaskHandle_t x_userInput;

void setup() 
{
    pinMode(6, OUTPUT);
    Serial.begin(115200);

    esp_log_level_set("wifi", ESP_LOG_ERROR);      // enable WARN logs from WiFi stack
    esp_log_level_set("dhcpc", ESP_LOG_ERROR);

    x_faultLED = new FaultLED(ax_faultConfig, m_SIZE_OF(ax_faultConfig));

    wifi_connect.set_app_ptr(&app_interface);

    EEPROM.begin(100);

    /**
     * @brief Initialize sensor
     */ 
    sensor = new INA226(I2C_CFG_SDA, 
                        I2C_CFG_SCL, 
                        I2C_CFG_CLOCK, 
                        I2C_CFG_INA_ADDR);
    initSensor(sensor);

    /**
     * @brief Connect to a WiFi STA
     */ 
    initWifi(WifiConnect::getInstance());

}

void loop() {
    static TickType_t xLastWakeTime = xTaskGetTickCount();
    static TickType_t xFrequency = 100 / portTICK_PERIOD_MS;

    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    int8_t wifi_status = WiFi.status();
    if(wifi_status == WL_CONNECTED)
    {
        x_wifiFault.o_faultFlag = 0;

        INA226::t_messageSensor x_sensorData;

        if (xQueueReceive(x_sensorDataQueue, (void *)&x_sensorData, 50 / portTICK_PERIOD_MS) == pdTRUE) 
        {
            app_interface.setMeasurementPayload(x_sensorData.current, x_sensorData.voltage);
            int16_t UDP_code = app_interface.UDP_measPayload();
            return;
        }
    }
    else
    {
        x_wifiFault.o_faultFlag=1 ;
    }
}

void start_sensor_task(INA226 *px_sensor);
void read_sensor_task(void *param);

/**
 * @brief Configure and calibration routine INAxxx sensor
 */ 
void initSensor(INA226 *px_sensor)
{
      
    x_inaFault.o_faultFlag = 1;
    if(px_sensor->configure(   INA226::E_AVG_16, 
                                INA226::E_VCT_1100us, 
                                INA226::E_VCT_1100us,
                                INA226::E_MODE_CNT_SHUNT_BUS) != 0)
    {
        while(true)
        {
            delay(1000);
        }
    }

    x_inaFault.o_faultFlag = 3;
    if(px_sensor->calibrate(0.004, x_sensorMaxConfig.max_current) != 0
    && px_sensor->setPowerLimit(x_sensorMaxConfig.max_power) != 0)
    {
        while(true)
        {
            delay(1000);
        }
    }
    
    x_inaFault.o_faultFlag = 0;
    start_sensor_task(px_sensor);
}

void start_sensor_task(INA226 *px_sensor)
{
    if(px_sensor->e_state < INA226::CALIBRATED)
    {
        return;
    }

    xTaskCreatePinnedToCore(read_sensor_task, "read_sensor_task", 4000, 
                                    (void*)px_sensor, 0, &x_readSensor, 1);
}

void read_sensor_task(void *param)
{
    INA226 *sensor = (INA226*)param;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xFrequency = 20 / portTICK_PERIOD_MS;

    INA226::t_messageSensor x_sensorData;
    t_sensorMaxConfig x_sensorMaxMeas = m_INIT_SENSOR_MAX_CONFIG;

    for(;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if(xSemaphoreTake(x_maxMeasSemp, 0) == pdTRUE)
        {
            x_sensorMaxMeas = x_sensorMaxConfig;
            sensor->calibrate(0.004, x_sensorMaxMeas.max_current);
            sensor->setPowerLimit(x_sensorMaxMeas.max_power);
            log_e("%f", x_sensorMaxMeas.max_voltage);
        }

        if(sensor->read_voltage() == -1
        || sensor->read_current() == -1)
        {
            continue;
            x_inaFault.o_faultFlag = 2;
        }
        
        x_sensorData.voltage = sensor->voltage;
        x_sensorData.current = sensor->current;

        
        if(sensor->voltage > x_sensorMaxMeas.max_voltage
        || sensor->current > x_sensorMaxMeas.max_current)
        {
            log_e("MAX!");
            m_SEND_STOP_SIGNAL;
            x_inaFault.o_faultFlag = 4;
        }

        xQueueSend(x_sensorDataQueue, (void*)&x_sensorData, 10 / portTICK_PERIOD_MS);
    }
}

void start_app_task(WifiConnect *px_app);
void check_user_input_task(void *param);

void initWifi(WifiConnect *px_wifiConnect)
{
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
    px_wifiConnect->set_STA_parameter(ssid, pass);
    px_wifiConnect->start_connection();
    
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
        
        while(true)
        {
            delay(1000);
        }
    }
    x_wifiFault.o_faultFlag = 0;
    start_app_task(px_wifiConnect);
}

void start_app_task(WifiConnect *px_app)
{
    xTaskCreatePinnedToCore(check_user_input_task, "check_user_input_task", 4000, 
                                    (void*)px_app, 1, &x_userInput, 1);
}
/**
 * @brief Async task checking user HTTP request.
 */
void check_user_input_task(void *param)
{
    WifiConnect *_this = WifiConnect::getInstance();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xFrequency = 100 / portTICK_PERIOD_MS;

    String header;
    String route;
    String resource_buffer;

    bool client_connected = false;
    uint16_t client_request_size;
    uint16_t client_timeout = 200;
    uint32_t client_curr_time = millis();
    uint32_t client_prev_time = client_curr_time;

    String client_IP;

    for(;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if(client_connected == false)
        {
            client_prev_time = millis();
            client_curr_time = client_prev_time;
            _this->client = _this->server.available();  // Listen for incoming clients
            if(!_this->client)
            {
                continue;
            }
            client_IP = _this->client.localIP().toString();
        }   

        if(_this->client.connected() && (client_curr_time - client_prev_time) < client_timeout)
        {
            client_curr_time = millis();
            client_connected = true;
            client_request_size = _this->client.available();
            if(client_request_size)
            {
                log_i("Incoming client %s is connected with request size %d", 
                    client_IP, client_request_size);
                for(int i = 0; i < client_request_size; i++)
                {
                   char c = _this->client.read();
                   header += c;
                }  

                int8_t request_type = HTTPHelper::handleHTTP(header, &route, _this->request_buffer, 255);
                switch(request_type)
                {
                    case(HTTPHelper::POST):
                    {   
                        resource_buffer = _this->request_buffer;
                        log_e("POST resource: %s", _this->request_buffer);
                        char resource[5][32];
                        uint16_t resource_idx[5];
                        uint8_t resource_length[5];

                        if(route == "wifi_info")
                        {
                            resource_idx[0] = resource_buffer.indexOf("=") + 1;
                            resource_idx[1] = resource_buffer.indexOf("=", resource_idx[0]) + 1;

                            resource_length[0] = resource_idx[1] - resource_idx[0] - 6;
                            
                            memset(resource[0], 0, 32);
                            resource_buffer.substring(resource_idx[0], resource_idx[0] + resource_length[0]).toCharArray(resource[0], 32);
                            HTTPHelper::decode(resource[0]);

                            memset(resource[1], 0, 32);
                            resource_buffer.substring(resource_idx[1]).toCharArray(resource[1], 32);
                            HTTPHelper::decode(resource[1]);

                            device_cfg.writeMemoryById(DeviceCfg::WIFISSID, (int8_t*)resource[0], 32);
                            device_cfg.writeMemoryById(DeviceCfg::WIFIPASS, (int8_t*)resource[1], 32);
                            log_e("Get STA params SSID: %s PASS: %s", resource[0], resource[1]);
                            _this->send_OKresp();
                        }
                        else if(route == "sensor_config")
                        {
                            resource_idx[0] = 0;
                            log_e("Get new sensor config");
                            for(int i=0; i<3; i++)
                            {
                                resource_idx[i] = resource_buffer.indexOf("=", resource_idx[i]) + 1;
                                resource_length[i] = resource_buffer.indexOf("&", resource_idx[i]) - resource_idx[i];
                                
                                memset(resource[i], 0, 32);
                                if(i == 2)
                                {
                                    resource_buffer.substring(resource_idx[i]).toCharArray(resource[i], 32);
                                }
                                else
                                {
                                    resource_buffer.substring(resource_idx[i], resource_idx[i] + resource_length[i]).toCharArray(resource[i], 32);
                                }
                                _this->app->max_measurement[i] = atof(resource[i]);
                                resource_idx[i+1] = resource_idx[i];
                            }
                            _this->send_OKresp();

                            if(xSemaphoreGive(x_maxMeasSemp) == pdTRUE)
                            {
                                x_sensorMaxConfig.max_current = _this->app->max_current;
                                x_sensorMaxConfig.max_voltage = _this->app->max_voltage;
                                x_sensorMaxConfig.max_power = _this->app->max_power;
                                log_e("Update new sensor config");
                            }
                        }
                        else if(route = "output")
                        {
                            digitalWrite(6, HIGH);
                            delay(100);
                            digitalWrite(6, LOW);
                            _this->send_OKresp();
                        }
                        break;
                    }
                    
                    case(HTTPHelper::GET):
                    {
                        if(_this->request_buffer[0] == '\0')
                        {
                            _this->send_homepage();
                        }
                        else
                        {
                            String requested_resource;
                            requested_resource += _this->request_buffer;
                            log_i("%s", requested_resource);
                            if(requested_resource == "reset")
                            {
                                _this->send_OKresp();
                                _this->client.println();

                                EEPROM.commit();
                                delay(1000);
                                ESP.restart();
                            }
                        }
                        break;
                    }
                };                     
            }
        }
        else
        {
            uint16_t client_connect_time = (client_curr_time - client_prev_time);
            log_i("Disconnect client %s. Connection time: %d ms", 
                    client_IP, client_connect_time);

            client_connected = false;
            // Clear the header variable
            header = "";

            _this->client.stop();
        }  
    }
}