#pragma once

#include <sstream>

#include <WiFi.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

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
    private:

        // Create Singleton class
        static WifiConnect* WifiConnectPtr;
        WifiConnect();

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

        TaskHandle_t *status_check;
        TaskHandle_t *user_input;

        typedef struct StatusMessage
        {	
            wl_status_t status;
            uint8_t localIP[4];
        } StatusMessage;

        StatusMessage *status_msg;
        QueueHandle_t *status_queue;

        WiFiServer server;
        WiFiClient client;

        AppInterface *app;

        const char host_name[10] = "PowerNode";
        

    public:
        
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
         * @brief Async task checking the connection status of the device
         */
        static void check_status_task(void *param);

        /**
         * @brief Get connection status of the device
         * 
         * @return -1: Error | 0-6: Connect status according to wl_status_t
         */
        int8_t get_status();

        /**
         * @brief Get local IP of the device
         * @param buffer
         * @param buffer_size
         * 
         * @return -1 if `buffer_size` < 4 
         */
        int8_t get_local_IP(uint8_t *buffer, uint8_t buffer_size);

        // 
        // Handle user's input from self-host server.
        //

        /**
         * @brief Async task checking user input on the device's self server
         */
        static void check_user_input_task(void *param);

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
        
        /**
         * @brief Set IP address of the host
         * @param hostIP
         * @return -1 if error
         */
        int8_t setHost(uint8_t *hostIP_=NULL, uint16_t port_=8090);

        /**
         * @brief POST data to the host
         * @return -1 if error
         */
        int16_t POST(String route, char *data);

        int16_t POST(String route, String data);

        int16_t POST_measPayload(String route);

        /**
         * @brief
         */
        int8_t setMeasurementPayload(float current_, float voltage_);
        
        /**
         * @brief Check if host is alive
         * @return -1 if Error
         */
        int8_t checkHostAlive();

    private:
        typedef struct MeasurementPayload_t{
            char buf[34] = "current=00.00000&voltage=00.00000";
            float current;
            float voltage;
        } MeasurementPayload_t;

        MeasurementPayload_t meas_payload;

        IPAddress hostAddress;
        uint16_t port = 8090;
        int8_t host_alive;

        HTTPClient *http;
        int16_t httpResponseCode;
        String POST_uri;

    friend WifiConnect;
};
