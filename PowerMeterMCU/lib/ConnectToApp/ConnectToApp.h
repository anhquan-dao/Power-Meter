#pragma once

#include <sstream>

#include <WiFi.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include <ESP32Ping.h>
#include <Helper.h>
#include <DeviceCfg.h>

#define ServerTask "ServerTask"

class AppInterface;
class STAInfo
{
    public:
        char STA_ssid[100];
        uint8_t STA_ssid_length = 0;
        char STA_pass[100];
        uint8_t STA_pass_length = 0;

        void setSSID(const char *ssid);
        uint8_t getSSID(char *buffer, uint8_t buffer_size);

        void setPass(const char *pass);
        uint8_t getPass(char *buffer, uint8_t buffer_size);

};
class WifiConnect
{
    public:

        // Create Singleton class
        static WifiConnect* WifiConnectPtr;
        WifiConnect() {}

        /**
         * @brief Connect to STA
         */
        int8_t configure_STA();

        /**
         * @brief Send OK response
         */ 
        void send_OKresp();

        /**
         * @brief Send homepage to configure wifi and host IP
         */
        void send_homepage();

        WiFiServer server;
        WiFiClient client;

        AppInterface *app;
        
        WifiConnect(const WifiConnect& obj) = delete;

        /**
         * @brief return instance poiter of WifiConnect
         */
        static WifiConnect* getInstance();

        /**
         * @brief Set the parameters of the STA
         * @param ssid
         * @param pass
         */
        void set_STA_parameter(const char *ssid, const char *pass);

        /**
         * @brief Start connection to the STA
         * 
         * @return -1 if Error
         */
        int8_t start_connection();

        /**
         * @brief 
         */
        void set_app_ptr(AppInterface *app_);
        
        STAInfo STA_info;

        char request_buffer[255];

        //Response for wifiStatus request
        struct wifiStatus_t
        {
            wl_status_t status;
            char name[32];
        };

        struct wifiStatus_t wifiStatus_struct[7]
        {
            {WL_IDLE_STATUS, "WL_IDLE_STATUS"},
            {WL_NO_SSID_AVAIL, "WL_NO_SSID_AVAIL"},
            {WL_SCAN_COMPLETED, "WL_SCAN_COMPLETED"},
            {WL_CONNECTED, "WL_CONNECTED"},
            {WL_CONNECT_FAILED, "WL_CONNECT_FAILED"},
            {WL_CONNECTION_LOST, "WL_CONNECTION_LOST"},
            {WL_DISCONNECTED, "WL_DISCONNECTED"}
        };

};

class AppInterface
{
    public:
        AppInterface();
        ~AppInterface(){}

        int16_t UDP_measPayload();

        /**
         * @brief
         */
        int8_t setMeasurementPayload(float current_, float voltage_);
        typedef struct MeasurementPayload_t{
            char buf[34] = "current=00.00000&voltage=00.00000";
            float current;
            float voltage;
        } MeasurementPayload_t;

        MeasurementPayload_t meas_payload;

        HTTPClient *http;
        int16_t httpResponseCode;
        String POST_uri;

        volatile bool   update_max = false;
        volatile float  max_measurement[3];
        volatile float  &max_current = max_measurement[0];
        volatile float  &max_voltage = max_measurement[1];
        volatile float  &max_power   = max_measurement[2];

    friend WifiConnect;
};
