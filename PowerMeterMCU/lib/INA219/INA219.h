#pragma once
#include <Arduino.h>
#include <Wire.h>

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
const uint8_t  INA219_SHUNT_VOLTAGE_REGISTER{1};    ///< INA219 Shunt Voltage Register
const uint8_t  INA219_CURRENT_REGISTER{4};          ///< INA219 Current Register
const uint16_t INA219_BUS_VOLTAGE_LSB{400};         ///< INA219 LSB in uV *100 4.00mV
const uint16_t INA219_SHUNT_VOLTAGE_LSB{100};       ///< INA219 LSB in uV *10  10.0uV
const uint16_t INA219_CONFIG_AVG_MASK{0x07F8};      ///< INA219 Bits 3-6, 7-10
const uint16_t INA219_CONFIG_PG_MASK{0xE7FF};       ///< INA219 Bits 11-12 masked
const uint16_t INA219_CONFIG_BADC_MASK{0x0780};     ///< INA219 Bits 7-10  masked
const uint16_t INA219_CONFIG_SADC_MASK{0x0038};     ///< INA219 Bits 3-5
const uint8_t  INA219_BRNG_BIT{13};                 ///< INA219 Bit for BRNG in config reg
const uint8_t  INA219_PG_FIRST_BIT{11};             ///< INA219 1st bit of Programmable Gain
const uint16_t INA219_ENABLE_AVG_BIT{0x0402};

class INA219
{
    public:
        INA219() = delete;

        INA219(int sda, int scl, int i2c_clock, int addr)
        {
            i2c_addr = addr;
            Wire.begin(sda, scl, i2c_clock);
            Wire.beginTransmission(addr);
            error = Wire.endTransmission();

            writeMessage();

            while(error != 0)
            {
                if(Serial)
                {
                    state = INIT;
                    writeMessage();
                }
                error = Wire.endTransmission();
                delay(10);
            }
            state = NORMAL;
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

        void configure(reset_t reset, brng_t brng, pga_t pga, badc_t badc, sadc_t sadc, mode_t mode)
        {   
            config = 0;
            config = reset << 15 | brng << 13 | pga << 11 | badc << 7 | sadc << 3 | mode;
            buffer[0] = config >> 8;
            buffer[1] = config;
            i2c_write(INA_CONFIGURATION_REGISTER, buffer, 2);

#ifdef SERIAL_DEBUG
            Serial.println("Configuration to write to INA219:");
            Serial.print("0x");
            // if(config < 0x1000) Serial.print("0");
            Serial.println(config, HEX);

            Serial.println("Configuration stored in INA219");
            i2c_read(INA_CONFIGURATION_REGISTER, 2);
            Serial.print("0x");
            if(rx_buffer[0] < 16) Serial.print("0");
            Serial.print(rx_buffer[0], HEX);
            if(rx_buffer[1] < 16) Serial.print("0");
            Serial.print(rx_buffer[1], HEX);
#endif
        }

        void calibrate(float shunt_val, float v_shunt_max, float i_max_expected)
        {
            r_shunt = shunt_val;

            current_lsb = i_max_expected / 32767;

            /* From datasheet: This value was selected to be a round number near the Minimum_LSB.
            * This selection allows for good resolution with a rounded LSB.
            * eg. 0.000610 -> 0.000700
            */
            uint16_t digits = 0;
            while( current_lsb > 0.0 ){//If zero there is something weird...
                if( (uint16_t)current_lsb / 1){
                    current_lsb = (uint16_t) current_lsb + 1;
                    current_lsb /= pow(10,digits);
                    break;
                }
                else{
                    digits++;
                    current_lsb *= 10.0;
                }
            };

            float swap = (0.04096)/(current_lsb*r_shunt);
            cal_value = (uint16_t) swap;
            power_lsb = current_lsb * 20;

            buffer[0] = cal_value >> 8;
            buffer[1] = cal_value;

            i2c_write(INA_CALIBRATION_REGISTER, buffer, 2);

        }

        void i2c_write(uint8_t reg_, uint8_t *buffer_, uint8_t size)
        {
            Wire.beginTransmission(i2c_addr);
            Wire.write(reg_);
            if(size !=0)
                Wire.write(buffer_, size);
            error = Wire.endTransmission();
            delay(10);
        }
        
        void i2c_write_null(uint8_t reg_, uint8_t size)
        {
            if(size!=0)
                memset(buffer, 0, size);
            
            i2c_write(reg_, buffer, size);
        }

        void i2c_read(uint8_t reg_, uint8_t size)
        {
            i2c_write_null(reg_, 0);

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

            for(int i=0; i<size; i++)
            {
                rx_buffer[i] = Wire.read();
            }
        }

        void read_voltage()
        {
            i2c_read(INA_BUS_VOLTAGE_REGISTER, 2);
            voltage = (((rx_buffer[0] << 8) | rx_buffer[1]) >> 3) * 0.004;
        }

        void read_shunt_voltage()
        {
            i2c_read(INA219_SHUNT_VOLTAGE_REGISTER, 2);
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
            i2c_read(INA219_CURRENT_REGISTER, 2);
            int16_t current_reg = (rx_buffer[0] << 8) | rx_buffer[1];
            current = current_reg * current_lsb;
            capacity += current * 0.1 / 3600;
#ifdef SERIAL_DEBUG
            Serial.print("Current LSB: ");
            Serial.print(current_reg); Serial.print(" ");
            Serial.println(current_lsb, 12);
#endif     
        }
        
        void writeMessage()
        {
            tx_msg_cnt++;
            writeHeader();
            writeByte(state);
            writeByte(tx_msg_cnt);
            writeByte(error);
            writeFloat(voltage);
        }
        void writeHeader()
        {
            Serial.write(header, 2);
        }
        void writeByte(uint8_t data)
        {
            Serial.write(data);
        }
        void writeFloat(float data)
        {
            uint8_t float_to_byte_arr[4];
            memcpy(float_to_byte_arr, &data, 4);
            Serial.write(float_to_byte_arr, 4);
        }

        uint8_t tx_msg_cnt;
        uint8_t rx_msg_cnt;
        uint8_t error_cnt;
        uint8_t error;

        uint8_t i2c_addr;
        uint8_t buffer[100];
        uint8_t rx_buffer[100];

        uint16_t config;

        uint8_t state;
        enum state_t{
            INIT,
            NORMAL,
            SHUTDOWN
        };

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
};
