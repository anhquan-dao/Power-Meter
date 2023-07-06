#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <INA226_registers.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class INA226
{
    public:
        INA226() = delete;

        INA226(int sda, int scl, uint32_t i2c_clock, int addr)
        {
            i2c_addr = addr;
            Wire.begin(sda, scl, i2c_clock);
            Wire.beginTransmission(addr);
            error = Wire.endTransmission();

            e_state = INIT;
        }

        ~INA226()
        {

        }

        enum e_AVG
        {
            E_AVG_1,
            E_AVG_4,
            E_AVG_16,
            E_AVG_64,
            E_AVG_128,
            E_AVG_256,
            E_AVG_512,
            E_AVG_1024
        };

        enum e_VCT
        {
            E_VCT_140us,
            E_VCT_204us,
            E_VCT_332us,
            E_VCT_588us,
            E_VCT_1100us,
            E_VCT_2116us,
            E_VCT_4156us,
            E_VCT_8244us
        };

        enum e_MODE
        {
            E_MODE_PWR_DOWN,
            E_MODE_TRG_SHUNT,
            E_MODE_TRG_BUS,
            E_MODE_TRG_SHUNT_BUS,
            E_MODE_ADC_OFF,
            E_MODE_CNT_SHUNT,
            E_MODE_CNT_BUS,
            E_MODE_CNT_SHUNT_BUS
        };

        void reset();
        int8_t configure(e_AVG e_avg, e_VCT e_busCVT, e_VCT e_shuntVCT , e_MODE e_mode);
        int8_t calibrate(float shunt_val, float i_max_expected);
        int8_t setPowerLimit(float limit);
        int8_t read_voltage();
        int8_t read_shunt_voltage();
        int8_t read_current();
        int8_t read_power();

        int8_t i2c_write(uint8_t reg_, uint8_t *buffer_, uint8_t size)
        {
            Wire.beginTransmission(i2c_addr);
            Wire.write(reg_);
            if(size !=0)
            {
                for(int i=size-1; i>=0; i--)
                {
                    Wire.write(*(buffer_+i));
                }
            }
                
            error = Wire.endTransmission();
            if(error != ESP_OK)
            {
                log_e("Fail to write to register %d", reg_);
                return -1;
            }

            delay(10);
            return 0;
        }
        
        int8_t i2c_write_null(uint8_t reg_, uint8_t size)
        {
            if(size!=0)
                memset(buffer, 0, size);
            
            return i2c_write(reg_, buffer, size);
        }

        int8_t i2c_read(uint8_t reg, uint8_t size)
        {
            if(i2c_write_null(reg, 0) == -1)
            {
                return -1;
            }
            
            uint8_t rx_byte_cnt = Wire.requestFrom(i2c_addr, size);
            if(rx_byte_cnt != size)
            {
                return -1;
            }

            memset(rx_buffer, 0, MAX_RX_BUFFER);
            for(int i=size-1; i>=0; i--)
            {
                rx_buffer[i] = Wire.read();
            }
            return 0;
        }

        static void read_sensor_task(void *param);

        uint8_t o_configure = false;
        uint8_t error;

        uint8_t i2c_addr;
        uint8_t buffer[MAX_TX_BUFFER];
        uint8_t rx_buffer[MAX_RX_BUFFER];

        uint16_t config;

        typedef enum n_state{
            INIT,
            CONFIGURED,
            CALIBRATED,
            NORMAL,
            SHUTDOWN,
            ERROR
        }t_state;
        t_state e_state;

        float voltage;
        float shunt_voltage;
        float current;
        float power_;
        boolean bus_voltage_ovf;

        float r_shunt;
        float current_lsb;
        uint16_t cal_value;
        float power_lsb;

        typedef struct s_messageSensor
        {	
            float voltage;
            float current;
        } t_messageSensor;
};
