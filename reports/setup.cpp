/** Class for configuring the WiFi connection */
WifiConnect* WifiConnect::WifiConnectPtr = NULL;
WifiConnect &wifi_connect = *WifiConnect::getInstance();

/** Class for sending data and receiving user's inputs */
AppInterface app_interface;

/** Class for configuring and reading the sensor board INA226 */
INA226 *sensor;

/** Struct for holding WiFi fault code */
t_faultSource x_wifiFault = {.o_faultLED = LED_GREEN};
/** Struct for holding INA fault code */
t_faultSource x_inaFault = {.o_faultLED = LED_BLUE};

t_faultSource *ax_faultConfig[] = {&x_wifiFault, &x_inaFault};
/** Class for reading error from each sources and blink the correct LED */
FaultLED x_faultLED(ax_faultConfig, m_SIZE_OF(ax_faultConfig));

void setup() 
{
    pinMode(MCU_CTRL_OUT, OUTPUT);
    Serial.begin(115200);

    start_led_task(&x_faultLED);

    /** Limit the log from other components */
    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_level_set("dhcpc", ESP_LOG_ERROR);

    wifi_connect.set_app_ptr(&app_interface);

    /** Initialize sensor */ 
    sensor = new INA226(I2C_CFG_SDA, 
                        I2C_CFG_SCL, 
                        I2C_CFG_CLOCK, 
                        I2C_CFG_INA_ADDR);
    initSensor(sensor);
    start_sensor_task(sensor);

    /** Connect to a WiFi STA */ 
    initWifi(WifiConnect::getInstance());
    start_app_task(WifiConnect::getInstance());

    start_send_data_task();
}