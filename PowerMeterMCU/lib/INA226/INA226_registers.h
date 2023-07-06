#pragma once

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
const float    INA226_BUS_VOLTAGE_LSB = 1.25e-3;         ///< INA226 LSB in uV *100 4.00mV
const float    INA226_SHUNT_VOLTAGE_LSB = 2.5e-6;       ///< INA226 LSB in uV *10  10.0uV
const uint16_t INA226_CONFIG_AVG_MASK{0x07F8};      ///< INA226 Bits 3-6, 7-10
const uint16_t INA226_CONFIG_PG_MASK{0xE7FF};       ///< INA226 Bits 11-12 masked
const uint16_t INA226_CONFIG_BADC_MASK{0x0780};     ///< INA226 Bits 7-10  masked
const uint16_t INA226_CONFIG_SADC_MASK{0x0038};     ///< INA226 Bits 3-5
const uint8_t  INA226_BRNG_BIT{13};                 ///< INA226 Bit for BRNG in config reg
const uint8_t  INA226_PG_FIRST_BIT{11};             ///< INA226 1st bit of Programmable Gain
const uint16_t INA226_ENABLE_AVG_BIT{0x0402};

#define INA_CONFIGURATION_REGISTER          0x0000  // Configuration Register address
#define INA_SHUNT_VOLTAGE_REGISTER          0x0001  // Shunt Voltage Register address
#define INA_BUS_VOLTAGE_REGISTER            0x0002  // Bus Voltage Register address
#define INA_POWER_REGISTER                  0x0003  // Power Register adress
#define INA_CURRENT_REGISTER                0x0004  // Current Register address
#define INA_CALIBRATION_REGISTER            0x0005  // Calibration Register address
#define INA_MARK_ENABLE_REGISTER            0x0006
#define INA_ALERT_LIMIT_REGISTER            0x0007

#define INA_CONFIGURATION_REGISTER_LEN      0x02    // Configuration Register length
#define INA_SHUNT_VOLTAGE_REGISTER_LEN      0x02    // Shunt Voltage Register length
#define INA_BUS_VOLTAGE_REGISTER_LEN        0x02    // Bus Voltage Register length
#define INA_POWER_REGISTER_LEN              0x02    // Power Register length
#define INA_CURRENT_REGISTER_LEN            0x02    // Current Register length
#define INA_CALIBRATION_REGISTER_LEN        0x02    // Calibration Register length
#define INA_MARK_ENABLE_REGISTER_LEN        0x02
#define INA_ALERT_LIMIT_REGISTER_LEN        0x02

#define MAX_TX_BUFFER                       10
#define MAX_RX_BUFFER                       10