void read_sensor_task(void *param)
{
    INA226 *sensor = (INA226*)param;

    TickType_t xLastWakeTime    = xTaskGetTickCount();
    TickType_t xFrequency       = 20 / portTICK_PERIOD_MS;
    uint8_t o_100msCounter      = 0;

    INA226::t_messageSensor x_sensorData;
    t_sensorMaxConfig x_sensorMaxMeas = {   .max_current = 10.0,
                                            .max_voltage = 30.0,
                                            .max_power   = 100.0};

    for(;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        /** Recalibrate and set new power limit if a request is received */
        if(xSemaphoreTake(x_maxMeasSemp, 0) == pdTRUE)
        {
            x_sensorMaxMeas = x_sensorMaxConfig;
            sensor->calibrate(0.004, x_sensorMaxMeas.max_current);
            sensor->setPowerLimit(x_sensorMaxMeas.max_power);
            x_inaFault.o_faultFlag = 0;
        }

        /** Check if the sensor board can still be read,
         *  and set the error code for fault_led_task
         */
        if(sensor->read_voltage() == -1
        || sensor->read_current() == -1)
        {
            continue;
            x_inaFault.o_faultFlag = 2;
        }
        
        /** Compare the read values with set maximum value,
         *  and outputs a positive pulse accordingly
         */
        x_sensorData.voltage = sensor->voltage;
        x_sensorData.current = sensor->current;

        if(sensor->voltage > x_sensorMaxMeas.max_voltage
        || sensor->current > x_sensorMaxMeas.max_current)
        {
            m_SEND_SWITCH_OFF_SIGNAL;
            x_inaFault.o_faultFlag = 4;
        }

        /** Data is sent every 100ms to the queue */
        if(o_100msCounter < 5)
        {
            o_100msCounter++;
            continue;
        }
        o_100msCounter = 0;
        xQueueSend(x_sensorDataQueue, (void*)&x_sensorData, 10 / portTICK_PERIOD_MS);
    }
}