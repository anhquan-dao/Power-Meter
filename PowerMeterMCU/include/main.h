#pragma once

#include <INA226.h>
#include <ConnectToApp.h>

#define I2C_CFG_SDA 7
#define I2C_CFG_SCL 8
#define I2C_CFG_CLOCK 100000
#define I2C_CFG_INA_ADDR 0x40

#define LED_GREEN 4
#define LED_BLUE 5

#define MOSFET_CTRL 6

#define RETRY_CONNECTION_MAX 10

#define m_SIZE_OF(obj) sizeof(obj)/sizeof(obj[0])
#define m_SEND_STOP_SIGNAL \
{   digitalWrite(6, HIGH); \
    delay(100);            \
    digitalWrite(6, LOW); }

#define m_INIT_SENSOR_MAX_CONFIG    \
{   .max_current = 10.0,            \
    .max_voltage = 30.0,            \
    .max_power   = 100.0}

typedef struct s_sensorMaxConfig
{	
    float  max_current;
    float  max_voltage;
    float  max_power;
} t_sensorMaxConfig;

extern t_sensorMaxConfig x_sensorMaxConfig;

extern SemaphoreHandle_t x_maxMeasSemp;
extern QueueHandle_t x_sensorDataQueue;

void initSensor(INA226 *px_sensor);
void start_sensor_task(INA226 *px_sensor);
void read_sensor_task(void *param);

void initWifi(WifiConnect *px_wifiConnect);
void start_app_task(WifiConnect *px_app);
void check_user_input_task(void *param);