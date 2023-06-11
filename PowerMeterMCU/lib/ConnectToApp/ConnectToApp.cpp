#include <ConnectToApp.h>
#include <index.h>

WiFiUDP udp_;

void STAInfo::setSSID(const char *ssid)
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

void STAInfo::setPass(const char *pass)
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
WifiConnect* WifiConnect::getInstance()
{
    if (WifiConnectPtr == NULL)
    {
        WifiConnectPtr = new WifiConnect();
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

void WifiConnect::set_STA_parameter(const char *ssid, const char *pass)
{
    STA_info.setSSID(ssid);
    STA_info.setPass(pass);
}

int8_t WifiConnect::start_connection()
{

    int8_t configure_ret;

    WiFi.mode(WIFI_AP_STA);

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

    px_userInput = new TaskHandle_t;

    xTaskCreatePinnedToCore(check_user_input_task, "check_user_input_task", 4000, 
                                    NULL, 1, px_userInput, 1);

    return 0;
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
                        else if(route == "server_info")
                        {
                            resource_idx[0] = resource_buffer.indexOf("=") + 1;

                            memset(resource[0], 0, 32);
                            resource_buffer.substring(resource_idx[0]).toCharArray(resource[0], 32);
                            HTTPHelper::decode(resource[0]);

                            resource_length[0] = TypeHelper::get_char_array_length(resource[0]);
                            log_e("Resource_length: %d", resource_length[0]);

                            int8_t plausibility_check = device_cfg.writeMemoryById(DeviceCfg::HOSTIP, (int8_t*)resource[0], resource_length[0]);
                            if(plausibility_check !=0)
                            {
                                log_e("Plausibility check result: %d", plausibility_check);
                            }
                            else
                            {
                                log_e("Get host ip: %s", resource[0]);
                            }
                            
                        }

                        _this->send_OKresp();
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
                                resp_text += _this->wifiStatus_struct[WiFi.status()].name;
                                resp_text += "\",\"host_status\":";
                                resp_text += _this->app->host_alive;
                                resp_text += "}\n";

                                _this->client.println(resp_text);
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

void WifiConnect::set_app_ptr(AppInterface *app_)
{
    app = app_;
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
    if(memcmp(&hostAddress[0], &hostIP_, 4) != 0)
    {
        hostAddress[0] = *hostIP_;
        hostAddress[1] = *(hostIP_+1);
        hostAddress[2] = *(hostIP_+2);
        hostAddress[3] = *(hostIP_+3);
        port = port_;
        host_alive = checkHostAlive();
        return host_alive;
    }

    return 0;
}

int16_t AppInterface::POST(String route, char *data)
{

    POST_uri = "http://" + hostAddress.toString() + ":8090/" + route;

    log_e("%s", POST_uri.c_str());

    if(!http->begin(POST_uri))
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

int16_t AppInterface::UDP_measPayload()
{
    udp_.beginPacket(hostAddress.toString().c_str(), 4444);
    log_e("%s", meas_payload.buf);
    udp_.write((uint8_t*)meas_payload.buf, 34);
    return udp_.endPacket();
}

int8_t AppInterface::checkHostAlive()
{
    host_alive = (Ping.ping(hostAddress, 1) - 1);
    return host_alive;
}

int8_t AppInterface::setMeasurementPayload(float current_, float voltage_)
{
    meas_payload.current = current_;
    meas_payload.voltage = voltage_;

    sprintf(meas_payload.buf, "current=%09.6f&voltage=%09.6f", current_, voltage_);

    return 0;
}



