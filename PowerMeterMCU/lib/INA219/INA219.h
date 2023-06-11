#pragma once
#include <Arduino.h>
#include <Wire.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/************************************************************************************************
** Declare constants used in the class                                                         **
************************************************************************************************/
#ifndef INA_I2C_MODES                             // I2C related constants
#define INA_I2C_MODES                             ///< Guard code to prevent multiple defs
const uint32_t INA_I2C_STANDARD_MODE{100000};     ///< Default normal I2C 100KHz speed
const uint32_t INA_I2C_FAST_MODE{400000};         ///< Fast mode
const uint32_t INA_I2C_FAST_MODE_PLUS{1000000};   ///< Really fast mode
const uint32_t INA_I2C_HIGH_SPEED_MODE{3400000};  ///< Turbo mode
#endif
const uint8_t  INA_CONFIGURATION_REGISTER{0};       ///< Configuration Register address
const uint8_t  INA_BUS_VOLTAGE_REGISTER{2};         ///< Bus Voltage Register address
const uint8_t  INA_POWER_REGISTER{3};               ///< Power Register address
const uint8_t  INA_CALIBRATION_REGISTER{5};         ///< Calibration Register address
const uint8_t  INA_MASK_ENABLE_REGISTER{6};         ///< Mask enable Register (some devices)
const uint8_t  INA_ALERT_LIMIT_REGISTER{7};         ///< Alert Limit Register (some devices)
const uint8_t  INA_MANUFACTURER_ID_REGISTER{0xFE};  ///< Mfgr ID Register (some devices)
const uint8_t  INA_DIE_ID_REGISTER{0xFF};           ///< Die ID Register (some devices)
const uint16_t INA_RESET_DEVICE{0x8000};            ///< Write to config to reset device
const uint16_t INA_CONVERSION_READY_MASK{0x0080};   ///< Bit 4
const uint16_t INA_CONFIG_MODE_MASK{0x0007};        ///< Bits 0-3
const uint16_t INA_ALERT_MASK{0x03FF};              ///< Mask off bits 0-9
const uint8_t  INA_ALERT_SHUNT_OVER_VOLT_BIT{15};   ///< Register bit
const uint8_t  INA_ALERT_SHUNT_UNDER_VOLT_BIT{14};  ///< Register bit
const uint8_t  INA_ALERT_BUS_OVER_VOLT_BIT{13};     ///< Register bit
const uint8_t  INA_ALERT_BUS_UNDER_VOLT_BIT{12};    ///< Register bit
const uint8_t  INA_ALERT_POWER_OVER_WATT_BIT{11};   ///< Register bit
const uint8_t  INA_ALERT_CONVERSION_RDY_BIT{10};    ///< Register bit
const uint8_t  INA_DEFAULT_OPERATING_MODE{B111};    ///< Default continuous mode
const uint16_t INA219_BUS_VOLTAGE_LSB{400};         ///< INA219 LSB in uV *100 4.00mV
const uint16_t INA219_SHUNT_VOLTAGE_LSB{100};       ///< INA219 LSB in uV *10  10.0uV
const uint16_t INA219_CONFIG_AVG_MASK{0x07F8};      ///< INA219 Bits 3-6, 7-10
const uint16_t INA219_CONFIG_PG_MASK{0xE7FF};       ///< INA219 Bits 11-12 masked
const uint16_t INA219_CONFIG_BADC_MASK{0x0780};     ///< INA219 Bits 7-10  masked
const uint16_t INA219_CONFIG_SADC_MASK{0x0038};     ///< INA219 Bits 3-5
const uint8_t  INA219_BRNG_BIT{13};                 ///< INA219 Bit for BRNG in config reg
const uint8_t  INA219_PG_FIRST_BIT{11};             ///< INA219 1st bit of Programmable Gain
const uint16_t INA219_ENABLE_AVG_BIT{0x0402};

#define INA_CONFIGURATION_REGISTER          0x0000  // Configuration Register address
#define INA_SHUNT_VOLTAGE_REGISTER          0x0001  // Shunt Voltage Register address
#define INA_BUS_VOLTAGE_REGISTER            0x0002  // Bus Voltage Register address
#define INA_POWER_REGISTER                  0x0003  // Power Register adress
#define INA_CURRENT_REGISTER                0x0004  // Current Register address
#define INA_CALIBRATION_REGISTER            0x0005  // Calibration Register address

#define INA_CONFIGURATION_REGISTER_LEN      0x02    // Configuration Register length
#define INA_SHUNT_VOLTAGE_REGISTER_LEN      0x02    // Shunt Voltage Register length
#define INA_BUS_VOLTAGE_REGISTER_LEN        0x02    // Bus Voltage Register length
#define INA_POWER_REGISTER_LEN              0x02    // Power Register length
#define INA_CURRENT_REGISTER_LEN            0x02    // Current Register length
#define INA_CALIBRATION_REGISTER_LEN        0x02    // Calibration Register length

#define MAX_TX_BUFFER                       10
#define MAX_RX_BUFFER                       10

class INA219
{
    public:
        INA219() = delete;

        INA219(int sda, int scl, uint32_t i2c_clock, int addr)
        {
            i2c_addr = addr;
            Wire.begin(sda, scl, i2c_clock);
            Wire.beginTransmission(addr);
            error = Wire.endTransmission();

            // px_sensorDataQueue = new QueueHandle_t;
            // *px_sensorDataQueue = xQueueCreate(4, sizeof(t_messageSensor));

            // px_readSensor = new TaskHandle_t;

            e_state = INIT;
        }

        ~INA219()
        {

        }

        enum reset_t
        {
            NO_RESET,
            RESET,
        };
        enum brng_t
        {
            BRNG_16V,
            BRNG_32V
        };
        enum pga_t
        {
            PGA_1 = 0,
            PGA_2 = 1,
            PGA_4 = 2,
            PGA_8 = 3
        };

        enum badc_t
        {
            BADC_9BIT = 0,
            BADC_10BIT = 1,
            BADC_11BIT = 2,
            BADC_12BIT = 3,
            BADC_2AVG = 9,
            BADC_4AVG = 10,
            BADC_8AVG = 11,
            BADC_16AVG = 12,
            BADC_32AVG = 13,
            BADC_64AVG = 14,
            BADC_128AVG = 15
        };

        enum sadc_t
        {
            SADC_9BIT = 0,
            SADC_10BIT = 1,
            SADC_11BIT = 2,
            SADC_12BIT = 3,
            SADC_2AVG = 9,
            SADC_4AVG = 10,
            SADC_8AVG = 11,
            SADC_16AVG = 12,
            SADC_32AVG = 13,
            SADC_64AVG = 14,
            SADC_128AVG = 15
        };

        enum mode_t
        {
            PWR_DOWN = 0,
            TRG_SHUNT = 1,
            TRG_BUS = 2,
            TRG_SHUNT_BUS = 3,
            ADC_OFF = 4,
            CNT_SHUNT = 5,
            CNT_BUS = 6,
            CNT_SHUNT_BUS = 7
        };

        int8_t configure(reset_t reset, brng_t brng, pga_t pga, badc_t badc, sadc_t sadc, mode_t mode)
        {           
            config = 0x0000;   
            config = (reset << 15) | (brng << 13) | (pga << 12) | (badc << 10) | (sadc << 6) | mode;
            
            memcpy(buffer, &config, INA_CONFIGURATION_REGISTER_LEN);
            i2c_write(INA_CONFIGURATION_REGISTER, buffer, INA_CONFIGURATION_REGISTER_LEN);

            i2c_read(INA_CONFIGURATION_REGISTER, INA_CONFIGURATION_REGISTER_LEN);
            if(memcmp(rx_buffer, &config, 2) == 0)
            {
                log_e("Configuration set %04X & stored %02X%02X", config, rx_buffer[0], rx_buffer[1]);
                e_state = CONFIGURED;
                return 0;
            }
            log_e("Fail to configure INA sensor. Configuration set %04X & stored %02X%02X", config, rx_buffer[0], rx_buffer[1]);
            e_state = CONFIGURED;
            return 0;
        }

        int8_t calibrate(float shunt_val, float v_shunt_max, float i_max_expected)
        {

            if(e_state < CONFIGURED)
            {
                return -1;
            }

            r_shunt = shunt_val;

            current_lsb = i_max_expected / 32767.0;

            /* From datasheet: This value was selected to be a round number near the Minimum_LSB.
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

            float trunc = (0.04096)/(current_lsb*r_shunt);
            cal_value = trunc;
            power_lsb = current_lsb * 20;

            memcpy(buffer, &cal_value, INA_CALIBRATION_REGISTER_LEN);
            i2c_write(INA_CALIBRATION_REGISTER, buffer, INA_CALIBRATION_REGISTER_LEN);

            i2c_read(INA_CALIBRATION_REGISTER, INA_CALIBRATION_REGISTER_LEN);
            if(memcmp(rx_buffer, &cal_value, 2) == 0)
            {
                e_state = CALIBRATED;
                log_e("Calibration set %04X & stored %02X%02X", cal_value, rx_buffer[1], rx_buffer[0]);
                return 0;
            }
            log_e("Fail to calibrate INA sensor. Calibration set %04X & stored %02X%02X", cal_value, rx_buffer[1], rx_buffer[0]);
            return -1;

        }

        void i2c_write(uint8_t reg_, uint8_t *buffer_, uint8_t size)
        {
            Wire.beginTransmission(i2c_addr);
            Wire.write(reg_);
            if(size !=0)
                Wire.write(buffer_, size);
            error = Wire.endTransmission();
            if(error != ESP_OK)
            {
                log_e("Fail to write to register %d", reg_);
            }
            delay(10);
        }
        
        void i2c_write_null(uint8_t reg_, uint8_t size)
        {
            if(size!=0)
                memset(buffer, 0, size);
            
            i2c_write(reg_, buffer, size);
        }

        void i2c_read(uint8_t reg, uint8_t size)
        {
            i2c_write_null(reg, 0);

            if(error != 0)
            {
                return;
            }
            
            uint8_t rx_byte_cnt = Wire.requestFrom(i2c_addr, size);
            if(rx_byte_cnt != size)
            {
                error = 6;
                return;
            }

            memset(rx_buffer, 0, MAX_RX_BUFFER);
            for(int i=0; i<size; i++)
            {
                rx_buffer[i] = Wire.read();
            }
        }

        void read_voltage()
        {
            i2c_read(INA_BUS_VOLTAGE_REGISTER, INA_BUS_VOLTAGE_REGISTER_LEN);
            voltage = (((rx_buffer[0] << 8) | rx_buffer[1]) >> 3) * 0.004;
        }

        void read_shunt_voltage()
        {
            i2c_read(INA_SHUNT_VOLTAGE_REGISTER, INA_SHUNT_VOLTAGE_REGISTER_LEN);
            uint16_t sign_mask = (uint16_t)(0xF000 << ((config & ~INA219_CONFIG_PG_MASK) >> 11));
            sign_mask &= 0x7FFF;

            int16_t shunt_volatage_reg = (rx_buffer[0] << 8) | rx_buffer[1];
            shunt_voltage = shunt_volatage_reg * 1.0e-5;

#ifdef SERIAL_DEBUG
            Serial.print("Shunt voltage register value: ");
            Serial.print("0x");
            Serial.print((uint16_t)shunt_volatage_reg, HEX);
            Serial.print(" 0x");
            Serial.println((uint16_t)(shunt_volatage_reg & (~sign_mask)), HEX);
#endif     
        }

        void read_current()
        {
            i2c_read(INA_CURRENT_REGISTER, INA_CURRENT_REGISTER_LEN);
            int16_t current_reg = (rx_buffer[0] << 8) | rx_buffer[1];
            current = current_reg * current_lsb;
            capacity += current * 0.1 / 3600;
#ifdef SERIAL_DEBUG
            Serial.print("Current LSB: ");
            Serial.print(current_reg); Serial.print(" ");
            Serial.println(current_lsb, 12);
#endif     
        }
        
        int8_t start()
        {
            if(e_state < CALIBRATED)
            {
                return -1;
            }

            xTaskCreatePinnedToCore(read_sensor_task, "read_sensor_task", 4000, 
                                    (void*)this, 0, px_readSensor, 1);

            e_state = NORMAL;
            return 0;
        }

        static void read_sensor_task(void *param)
        {
            INA219 *_this = (INA219*)param;

            TickType_t xLastWakeTime = xTaskGetTickCount();
            TickType_t xFrequency = 100 / portTICK_PERIOD_MS;

            for(;;)
            {
                vTaskDelayUntil(&xLastWakeTime, xFrequency);
                _this->read_voltage();
                _this->read_current();

                _this->x_sensorData.voltage = _this->voltage;
                _this->x_sensorData.current = _this->current;
                if(xQueueSend(*_this->px_sensorDataQueue, (void*)&(_this->x_sensorData), 50 / portTICK_PERIOD_MS) == pdTRUE)
                {

                }
            }
            
        }

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

        uint8_t header[2] = {0x79, 0x97};

        float voltage;
        float shunt_voltage;
        float current;
        float capacity = 0;
        boolean bus_voltage_ovf;

        float r_shunt;
        float current_lsb;
        uint16_t cal_value;
        float power_lsb;

        TaskHandle_t *px_readSensor;
        typedef struct s_messageSensor
        {	
            float voltage;
            float current;
        } t_messageSensor;

        t_messageSensor x_sensorData;
        QueueHandle_t *px_sensorDataQueue;
};
