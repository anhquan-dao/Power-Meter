void send_data_task(void)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xFrequency = 100 / portTICK_PERIOD_MS;
    for(;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        int8_t wifi_status = WiFi.status();
        if(wifi_status == WL_CONNECTED)
        {
            x_wifiFault.o_faultFlag = 0;

            /** Check if the queue is available to take,
             *  and publish the value to 255.255.255.255 
             */
            INA226::t_messageSensor x_sensorData;
            if (xQueueReceive(x_sensorDataQueue, (void *)&x_sensorData, 50 / portTICK_PERIOD_MS) == pdTRUE) 
            {
                app_interface.setMeasurementPayload(x_sensorData.current, x_sensorData.voltage);
                int16_t UDP_code = app_interface.UDP_measPayload();
                return;
            }
        }
        
        x_wifiFault.o_faultFlag=1 ;
    }
}