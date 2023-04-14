#include <ConnectToApp.h>
#include <index.h>

void STAInfo::setSSID(char *ssid)
{
    memset(STA_ssid, 0, 100);
    STA_ssid_length = TypeHelper::get_char_array_length(ssid);
    memcpy(STA_ssid, ssid, STA_ssid_length);
}

uint8_t STAInfo::getSSID(char *buffer, uint8_t buffer_size)
{
    if(buffer_size > STA_ssid_length) 
    {
        memcpy(buffer, STA_ssid, STA_ssid_length);
    }
    else
    {
        memcpy(buffer, STA_ssid, buffer_size);
    }

    return STA_ssid_length;
}

void STAInfo::setPass(char *pass)
{
    memset(STA_pass, 0, 100);
    STA_pass_length = TypeHelper::get_char_array_length(pass);
    memcpy(STA_pass, pass, STA_pass_length);
}

uint8_t STAInfo::getPass(char *buffer, uint8_t buffer_size)
{
    if(buffer_size > STA_pass_length) 
    {
        memcpy(buffer, STA_pass, STA_pass_length);
    }
    else
    {
        memcpy(buffer, STA_pass, buffer_size);
    }

    return STA_pass_length;
}

//======================================================
//            WifiConnect definition
//========================================================

WifiConnect::WifiConnect()
{

    // Create necessary resource for async task status_check_task()
    status_check = new TaskHandle_t;
    status_msg = new StatusMessage;

    status_queue = new QueueHandle_t;
    *status_queue = xQueueCreate(4, sizeof(StatusMessage));
}

WifiConnect* WifiConnect::getInstance()
{
    if (WifiConnectPtr == NULL)
    {
        WifiConnectPtr = new WifiConnect();
        return WifiConnectPtr;
    }

    return WifiConnectPtr;
}

int8_t WifiConnect::configure_STA()
{
    // Check if SSID has been defined
    if(STA_info.STA_ssid_length == 0)
    {
        return -1;
    }

    char STA_ssid_[STA_info.STA_ssid_length];
    memcpy(STA_ssid_, STA_info.STA_ssid, STA_info.STA_ssid_length);
    // Check if the STA requires password to connect
    if(STA_info.STA_pass_length == 0)
    {
        WiFi.begin(STA_ssid_, NULL);
    }
    else
    {   
        char STA_pass_[STA_info.STA_pass_length];
        memcpy(STA_pass_, STA_info.STA_pass, STA_info.STA_pass_length);
        WiFi.begin(STA_ssid_, STA_pass_);
    }

    return 0;
}

void WifiConnect::send_OKresp()
{
    client.println(OK_resp);
    client.println();
}   

void WifiConnect::send_homepage()
{
    send_OKresp();
    // Display the HTML web page
    client.println(MAIN_page);
    client.println();
}

void WifiConnect::set_STA_parameter(char *ssid, char *pass)
{
    STA_info.setSSID(ssid);
    STA_info.setPass(pass);
}

int8_t WifiConnect::start_connection()
{

    int8_t configure_ret;

    WiFi.mode(WIFI_AP_STA);

    // WiFi.disconnect();
    // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    // WiFi.setHostname(host_name); //define hostname
    // log_i("Set host name %s for device", WiFi.getHostname());

    // Setup the device as AP for configuring STA info
    IPAddress local_ip(192,168,1,1);
    IPAddress gateway(192,168,1,255);
    IPAddress subnet(255,255,255,0);

    server.begin(80);

    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP("ESP32Power", "", 10, false, 2);

    configure_ret = configure_STA();
    if(configure_ret != 0)
    {
        return configure_ret;
    }

    xTaskCreatePinnedToCore(check_status_task, "check_connect_task", 2000, 
                                    NULL, 0, status_check, 0);

    xTaskCreatePinnedToCore(check_user_input_task, "check_user_input_task", 4000, 
                                    NULL, 0, user_input, 1);

    return 0;
}

void WifiConnect::check_status_task(void *param)
{
    WifiConnect *_this = WifiConnect::getInstance();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xFrequency = 300 / portTICK_PERIOD_MS;

    WifiConnect::StatusMessage txMessage;
    bool connected = false;

    uint8_t scan_channel = 1;
    int16_t scan_status = 0;
    int8_t found_ssid = 0;
    
    // Every second check for Wifi.status()
    // If connected, store the local IP and set connected flag to true
    for(;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        txMessage.status = WiFi.status();        
        if (txMessage.status == WL_CONNECTED) 
        {   
            if(connected == false)
            {
                IPAddress localIP = WiFi.localIP();
                connected = true;
                txMessage.localIP[0] = localIP[0];
                txMessage.localIP[1] = localIP[1];
                txMessage.localIP[2] = localIP[2];
                txMessage.localIP[3] = localIP[3];
            }
        }
        else
        {
            connected = false;
        }
        xQueueSendToFront(*(_this->status_queue), (void *)&txMessage, 0);

        if(found_ssid == 0)
        {
            scan_channel >= 14 ? scan_channel = 1 : scan_channel++;

            scan_status = WiFi.scanNetworks();
            log_e("Channel %d. Scan status: %d", scan_channel, scan_status);
            if(scan_status < 0)
            {
                continue;
            }

            for(int i=0; i<scan_status; i++)
            {   
                log_e("%s", WiFi.SSID(i).c_str());
                if(WiFi.SSID(i) == _this->STA_info.STA_ssid)
                {
                    found_ssid = 1;
                    _this->configure_STA();
                    break;
                }
            }
            
            continue;
        }     
    }   
}

void WifiConnect::check_user_input_task(void *param)
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
                            if(requested_resource == "wifiStatus")
                            {
                                _this->send_OKresp();                                

                                String resp_text = "{\"connect\":\"";
                                resp_text += _this->wifiStatus_struct[_this->get_status()].name;
                                resp_text += "\",\"host_status\":\"1.1.1.1\"}";
                                _this->client.println(resp_text);
                                _this->client.println();
                            }
                            else if(requested_resource == "reset")
                            {
                                _this->send_OKresp();
                                _this->client.println();

                                EEPROM.commit();
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

int8_t WifiConnect::get_status()
{
    if(xQueuePeek(*status_queue, (void*)status_msg, 0) != pdTRUE)
    {
        return -1;
    }

    return status_msg->status;
}

int8_t WifiConnect::get_local_IP(uint8_t *buffer, uint8_t buffer_size)
{
    if(buffer_size < 4)
    {
        return -1;
    }

    memcpy(buffer, status_msg->localIP, 4);
    return 0;
}

//======================================================
//            AppInteface definition
//========================================================

AppInterface::AppInterface()
{
    http = new HTTPClient;
}

int8_t AppInterface::setHost(uint8_t *hostIP_, uint16_t port_)
{
    if(memcmp(&hostIP, hostIP, 4) != 0)
    {
        memcpy(&hostIP_, hostIP, 4);
    }
    
    port = port_;

    host_alive = checkHostAlive();

    return host_alive;
}

int16_t AppInterface::POST(String route, char *data)
{
    if(!http->begin("http://172.16.133.43:8090/esp32_post"))
    {
        return -1;
    }

    http->addHeader("Content-Type", "application/x-www-form-urlencoded");

    uint8_t data_length = TypeHelper::get_char_array_length(data);
    int16_t httpResponseCode = http->POST((uint8_t*)data, data_length-1);
    return httpResponseCode;
}

int16_t AppInterface::POST(String route, String data)
{
    char buf[data.length()+1];
    data.toCharArray(buf, data.length());
    return POST(route, buf);
}

int16_t AppInterface::POST_measPayload(String route)
{
    return POST(route, meas_payload.buf);
}

int8_t AppInterface::checkHostAlive()
{
    hostAddress[0] = hostIP[0];
    hostAddress[1] = hostIP[1];
    hostAddress[2] = hostIP[2];
    hostAddress[3] = hostIP[3];

    return (Ping.ping(hostAddress, 1) - 1);
}

int8_t AppInterface::setMeasurementPayload(float current_, float voltage_)
{
    meas_payload.current = current_;
    meas_payload.voltage = voltage_;

    sprintf(meas_payload.buf, "current=%06.3f&voltage=%06.3f", current_, voltage_);

    return 0;
}



