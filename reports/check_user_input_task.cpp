void check_user_input_task(void *param)
{
    WifiConnect *_this = WifiConnect::getInstance();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xFrequency = 100 / portTICK_PERIOD_MS;
    for(;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        /** Check if any client sends a HTTP request to the device */
        _this->client = _this->server.available(); 
        if(!_this->client) continue;

        /** Check if the request size is larger than 0 */
        uint16_t client_request_size = _this->client.available();
        if(!client_request_size) continue;
        
        /** Parse the request type, route and the data of the HTTP request */
        String header = _this->client.read(client_request_size);        
        String route, resource_buffer;
        int8_t request_type = HTTPHelper::parseHTTP(header, &route, &resource_buffer, 255);
        switch(request_type)
        {
            case(HTTPHelper::POST):
                if(route == "sensor_config")
                {
                    parse_sensor_config(resource_buffer, _this);
                    if(xSemaphoreGive(x_maxMeasSemp) == pdTRUE)
                    {
                        update_sensor_config(_this, &x_sensorMaxConfig);
                    }
                }
                else if(route == "output")
                {
                    m_SEND_SWITCH_OFF_SIGNAL;
                }
                _this->send_OKresp();
                break;

            case(HTTPHelper::GET):
                if(requested_resource == "reset")
                {
                    _this->send_OKresp();
                    ESP.restart();
                }
                break;
        };                     
    } 
}