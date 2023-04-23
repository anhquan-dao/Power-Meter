#include <DeviceCfg.h>

void DeviceCfg::printCfg()
{
    // log_i("Wifi SSID: %s\nHost IP: %s", wifi_cfg.wifi, host_ip);
}

void DeviceCfg::readMemory(uint16_t addr, int8_t *buffer, uint16_t buffer_size)
{
    EEPROM.readBytes(addr, buffer, buffer_size);
}

void DeviceCfg::writeMemory(uint16_t addr, int8_t *buffer, uint16_t buffer_size)
{
    memset(buffer, 0, buffer_size);
    EEPROM.writeBytes(addr, buffer, buffer_size);
}

void DeviceCfg::readMemoryById(data_enum data_id, int8_t *buffer, uint16_t buffer_size)
{
    memory_id_t &data = memory_id[data_id];
    EEPROM.readBytes(data.addr, buffer, buffer_size < data.size ? buffer_size : data.size);
}

int8_t DeviceCfg::writeMemoryById(data_enum data_id, int8_t *buffer, uint16_t buffer_size)
{
    memory_id_t &data = memory_id[data_id];
    
    int8_t plausible = (this->*(data.plausibility_func))(&data, buffer, buffer_size);
    log_i("Plausibility check: %d", plausible);
    if(plausible != 0)
    {
        return plausible;
    }

    for(int i=0; i<data.size; i++)
    {
        EEPROM.writeByte(data.addr+i, 0);
    }

    EEPROM.writeBytes(data.addr, buffer, buffer_size);

    return 0;
}

int8_t DeviceCfg::checkHostIP(memory_id_t *data, int8_t *buffer, uint16_t buffer_size)
{
    int8_t dot_count = 0;
    if(buffer_size > data->size)
    {
        return -1;
    }

    for(int i=0; i<buffer_size-1; i++)
    {
        // Check if IP contains 3 dot
        if(*(buffer+i) == 0x2E)
        {
            dot_count++;
            continue;
        }

        // Check if IP contains only number
        if(*(buffer+i) < 0x30 || *(buffer+i) > 0x39)
        {
            return -3;
        }
    }

    if(dot_count != 3)
    {
        return -2;
    }

    return 0;
}

DeviceCfg device_cfg;