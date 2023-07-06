#include "INA226.h"

/**
 * @brief   Reset INA226 by setting the first bit of the configuraiton register 00h
 */ 
void INA226::reset()
{   
    uint16_t reset_cmd = 0x8000;
    memcpy(buffer, &reset_cmd, INA_CONFIGURATION_REGISTER_LEN);
    i2c_write(INA_CONFIGURATION_REGISTER, buffer, INA_CONFIGURATION_REGISTER_LEN);
    
    delay(500);
    i2c_read(INA_CONFIGURATION_REGISTER, INA_CONFIGURATION_REGISTER_LEN);
    uint16_t default_config = 0x4127;
    if(memcmp(rx_buffer, &default_config, 2) != 0)
    {
        log_e("Fail to reset device");
        log_e("Configuration stored %02X%02X", rx_buffer[1], rx_buffer[0]);
    }
}

/**
 * @brief   Configure the operation of INA226 by writing to the configuration register 00h
 * @param   e_avg       Number of samples used for averaging.
 * @param   e_busCVT    Bus voltage conversion time.
 * @param   e_shuntCVT  Shunt voltage converstion time.
 * @param   e_mode      Operation mode.
 * 
 * @retval  0           If success.
 * @retval  -1          If register cannot be set.
 */
int8_t INA226::configure(e_AVG e_avg, e_VCT e_busCVT, e_VCT e_shuntVCT , e_MODE e_mode)
{ 
    // Reset the device
    reset();
    
    // Write configuration to the device
    config = (e_avg << 9) | (e_busCVT << 6) | (e_shuntVCT << 3) | e_mode;
    memcpy(buffer, &config, INA_CONFIGURATION_REGISTER_LEN);
    i2c_write(INA_CONFIGURATION_REGISTER, buffer, INA_CONFIGURATION_REGISTER_LEN);

    i2c_read(INA_CONFIGURATION_REGISTER, INA_CONFIGURATION_REGISTER_LEN);
    log_i("Configuration set %04X & stored %02X%02X.", config, rx_buffer[1], rx_buffer[0]);
    if(((rx_buffer[1]&0x0F)   != ((config >> 8)&0xFF))
    || (rx_buffer[0]        != (config&0xFF)))
    {
        log_e("Fail to configure INA sensor.");
        return -1;
    }

    /**
     *  Enable alert function of the device
     *  Alert function: Power Over-limit
     *  Alert polarity: Active High
     *  => Alert mask = 0x0802;
     */
    uint16_t alert_mask = 0x0802;
    memcpy(buffer, &alert_mask, INA_MARK_ENABLE_REGISTER_LEN);
    i2c_write(INA_MARK_ENABLE_REGISTER, buffer, INA_MARK_ENABLE_REGISTER_LEN);

    i2c_read(INA_MARK_ENABLE_REGISTER, INA_MARK_ENABLE_REGISTER_LEN);
    log_e("Alert mask set %04X & stored %02X%02X", alert_mask, rx_buffer[1], rx_buffer[0]);
    if(rx_buffer[1] == buffer[1] 
    && rx_buffer[0]&0x03 == buffer[0])
    {
        log_e("Fail to configure alert mask.");
        return -1;
    }

    e_state = CONFIGURED;
    return 0;
}

/**
 * @brief   Write the calibration parameter to the Calibration register 05h
 */ 
int8_t INA226::calibrate(float shunt_val, float i_max_expected)
{
    if(e_state < CONFIGURED)
    {
        return -1;
    }

    r_shunt = shunt_val;
    current_lsb = i_max_expected / 32768.0;

    /**
     * From datasheet: This value was selected to be a round number near the Minimum_LSB.
     * This selection allows for good resolution with a rounded LSB.
     * eg. 0.000610 -> 0.000700
     */
    uint16_t digits = 0;
    while( current_lsb > 0.0 ){//If zero there is something weird...
        if(current_lsb >= 1){
            current_lsb = (uint16_t)current_lsb + 1;
            current_lsb /= pow(10,digits);
            break;
        }
        else{
            digits++;
            current_lsb *= 10.0;
        }
    };

    /**
     * cal_value and power_lsb are calculated following the datasheet
     */
    cal_value = (uint16_t)((0.00512)/(current_lsb*r_shunt));
    power_lsb = current_lsb * 25.0;

    memcpy(buffer, &cal_value, INA_CALIBRATION_REGISTER_LEN);
    i2c_write(INA_CALIBRATION_REGISTER, buffer, INA_CALIBRATION_REGISTER_LEN);

    i2c_read(INA_CALIBRATION_REGISTER, INA_CALIBRATION_REGISTER_LEN);
    log_i("Calibration set %04X & stored %02X%02X", cal_value, rx_buffer[1], rx_buffer[0]);
    if(memcmp(rx_buffer, &cal_value, 2) != 0)
    {   
        log_e("Fail to calibrate INA sensor. Calibration set %04X & stored %02X%02X", cal_value, rx_buffer[1], rx_buffer[0]);
        return -1;
    }

    e_state = CALIBRATED;
    return 0;

}

int8_t INA226::setPowerLimit(float limit)
{
    uint16_t alert_limit = limit / power_lsb;
    memcpy(buffer, &alert_limit, INA_ALERT_LIMIT_REGISTER_LEN);
    i2c_write(INA_ALERT_LIMIT_REGISTER, buffer, INA_ALERT_LIMIT_REGISTER_LEN);

    i2c_read(INA_ALERT_LIMIT_REGISTER, INA_ALERT_LIMIT_REGISTER_LEN);
    log_e("Power limit set %04X & stored %02X%02X", alert_limit, rx_buffer[1], rx_buffer[0]);
    if(memcmp(rx_buffer, buffer, 2) != 0)
    {
        log_e("Fail to configure power limit.");
        return -1;
    }
    return 0;
}


/**
 * @brief   Read the value stored in Bus Voltage register 02h
 */ 
int8_t INA226::read_voltage()
{
    if(i2c_read(INA_BUS_VOLTAGE_REGISTER, INA_BUS_VOLTAGE_REGISTER_LEN) == -1)
    {
        return -1;
    }

    voltage = ((rx_buffer[1] << 8) | rx_buffer[0]) * INA226_BUS_VOLTAGE_LSB;
    return 0;
}

/**
 * @brief   Read the value stored in Shunt Voltage register 01h
 */ 
int8_t INA226::read_shunt_voltage()
{
    if(i2c_read(INA_SHUNT_VOLTAGE_REGISTER, INA_SHUNT_VOLTAGE_REGISTER_LEN) == -1)
    {
        return -1;
    }

    int16_t shunt_voltage_reg = (rx_buffer[1] << 8) | rx_buffer[0];
    shunt_voltage = shunt_voltage_reg * INA226_SHUNT_VOLTAGE_LSB;   
    return 0;
}

/**
 * @brief   Read the value stored in Current register 04h
 */
int8_t INA226::read_current()
{
    if(i2c_read(INA_CURRENT_REGISTER, INA_CURRENT_REGISTER_LEN) == -1)
    {
        return -1;
    }

    int16_t current_reg = (rx_buffer[1] << 8) | rx_buffer[0];
    current = current_reg * current_lsb;
    return 0; 
}

/**
 * @brief   Read the value stored in Power register 03h
 */
int8_t INA226::read_power()
{
    if(i2c_read(INA_POWER_REGISTER, INA_POWER_REGISTER_LEN) == -1)
    {
        return -1;
    }

    uint16_t power_reg = (rx_buffer[1] << 8) | rx_buffer[0];
    power_ = power_reg * power_lsb;
    return 0;    
}

