#include "FaultLED.h"

FaultLED::FaultLED(t_faultSource *ax_faultConfig[], uint8_t o_faultCnt_)
{
    o_faultCnt = o_faultCnt_;
    pax_faultConfig = new t_faultSource*[o_faultCnt];
    for(int i = 0; i<o_faultCnt; i++)
    {
        pax_faultConfig[i] = ax_faultConfig[i];
        pinMode(pax_faultConfig[i]->o_faultLED, OUTPUT);
    }
    px_statusLED = new TaskHandle_t;
    xTaskCreatePinnedToCore(statusBlinkTask, "statusBlinkTask", 1000,
                            (void*)this, 1, px_statusLED, 1);
}

void FaultLED::statusBlinkTask(void *param)
{
    FaultLED *px_faultLED = (FaultLED*)param;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xFrequency = 250 / portTICK_PERIOD_MS;

    for(;;px_faultLED->o_tickCnt++)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency/2.0);
        uint8_t o_tickCnt_ = px_faultLED->o_tickCnt & 0x03;
        for(int i=0; i<px_faultLED->o_faultCnt; i++)
        {
            if(px_faultLED->pax_faultConfig[i]->o_faultFlag >= o_tickCnt_)
            {
                digitalWrite(px_faultLED->pax_faultConfig[i]->o_faultLED, HIGH);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency/2.0);
        for(int i=0; i<px_faultLED->o_faultCnt; i++)
        {
            digitalWrite(px_faultLED->pax_faultConfig[i]->o_faultLED, LOW);
        }
    }
    
}