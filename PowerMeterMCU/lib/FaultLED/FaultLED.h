#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct s_faultSource
{
    uint8_t o_faultFlag;
    const uint8_t o_faultLED;
}t_faultSource;

class FaultLED
{
    public:
        /** @name FaultLED_Func FaultLED Member Method */
        /**@{*/ 
        FaultLED() = delete;

        FaultLED(t_faultSource *ax_faultConfig[], uint8_t o_faultCnt_);

        ~FaultLED()
        {
            delete[] pax_faultConfig;
        }

        static void statusBlinkTask(void *param);
        
        /**@}*/ 

        /** @name FaultLED_Var FaultLED Member Variable */
        /**@{*/ 
        uint8_t o_tickCnt;
        uint8_t o_faultCnt;
        t_faultSource **pax_faultConfig;
        TaskHandle_t *px_statusLED;
        /**@}*/ 



};