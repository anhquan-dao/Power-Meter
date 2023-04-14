#pragma once

#include <EEPROM.h>

class DeviceCfg
{
    public:
        DeviceCfg(){}
        ~DeviceCfg(){}

        void printCfg();

        void readMemory(uint16_t addr, int8_t *buffer, uint16_t buffer_size);
        void writeMemory(uint16_t addr, int8_t *buffer, uint16_t buffer_size);

        enum data_enum
        {
            WIFISSID,
            WIFIPASS,
            HOSTIP,
            DATA_COUNT
        };

        struct memory_id_t
        {
            data_enum id_enum;
            uint16_t addr;
            uint16_t size;
            int8_t (DeviceCfg::*plausibility_func)(memory_id_t*, int8_t*, uint16_t);
            char     id[50];     
        };
        
        void readMemoryById(data_enum data_id, int8_t *buffer, uint16_t buffer_size);
        int8_t writeMemoryById(data_enum data_id, int8_t *buffer, uint16_t buffer_size);

        int8_t defaultPlausibility(memory_id_t *data, int8_t *buffer, uint16_t buffer_size)
        {
            if(buffer_size > data->size)
            {
                return -1;
            }
            return 0;
        }

        int8_t checkHostIP(memory_id_t *data, int8_t *buffer, uint16_t buffer_size);

        struct memory_id_t memory_id[DATA_COUNT]
        {
            {WIFISSID, 0, 32, &DeviceCfg::defaultPlausibility, "WifiSSID"},
            {WIFIPASS, 32, 32, &DeviceCfg::defaultPlausibility, "WifiPASS"},
            {HOSTIP, 64, 15, &DeviceCfg::checkHostIP, "HostIP"}
        };

        char wifi_ssid[32];
        char wifi_pass[32];
        char host_ip[15];
};

extern DeviceCfg device_cfg;