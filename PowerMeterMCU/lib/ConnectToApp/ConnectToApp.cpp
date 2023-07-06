#include <ConnectToApp.h>
#include <index.h>

WiFiUDP udp_;

void STAInfo::setSSID(const char *ssid)
{
    memset(STA_ssid, 0, 100);
    STA_ssid_length = TypeHelper::get_char_array_length(ssid);
    memcpy(STA_ssid, ssid, STA_ssid_length);
}

uint8_t STAInfo::getSSID(char *buffer, uint8_t buffer_size)
{
    if(buffer_size > STA_ssid_length) 
    {
        memcpy(buffer, STA_ssid, STA_ssid_length);
    }
    else
    {
        memcpy(buffer, STA_ssid, buffer_size);
    }

    return STA_ssid_length;
}

void STAInfo::setPass(const char *pass)
{
    memset(STA_pass, 0, 100);
    STA_pass_length = TypeHelper::get_char_array_length(pass);
    memcpy(STA_pass, pass, STA_pass_length);
}

uint8_t STAInfo::getPass(char *buffer, uint8_t buffer_size)
{
    if(buffer_size > STA_pass_length) 
    {
        memcpy(buffer, STA_pass, STA_pass_length);
    }
    else
    {
        memcpy(buffer, STA_pass, buffer_size);
    }

    return STA_pass_length;
}

//======================================================
//            WifiConnect definition
//========================================================
WifiConnect* WifiConnect::getInstance()
{
    if (WifiConnectPtr == NULL)
    {
        WifiConnectPtr = new WifiConnect();
    }
    return WifiConnectPtr;
}

int8_t WifiConnect::configure_STA()
{
    // Check if SSID has been defined
    if(STA_info.STA_ssid_length == 0)
    {
        return -1;
    }

    char STA_ssid_[STA_info.STA_ssid_length];
    memcpy(STA_ssid_, STA_info.STA_ssid, STA_info.STA_ssid_length);
    // Check if the STA requires password to connect
    if(STA_info.STA_pass_length == 0)
    {
        WiFi.begin(STA_ssid_, NULL);
    }
    else
    {   
        char STA_pass_[STA_info.STA_pass_length];
        memcpy(STA_pass_, STA_info.STA_pass, STA_info.STA_pass_length);
        WiFi.begin(STA_ssid_, STA_pass_);
    }

    return 0;
}

void WifiConnect::send_OKresp()
{
    client.println(OK_resp);
    client.println();
}   

void WifiConnect::send_homepage()
{
    send_OKresp();
    // Display the HTML web page
    client.println(MAIN_page);
    client.println();
}

void WifiConnect::set_STA_parameter(const char *ssid, const char *pass)
{
    STA_info.setSSID(ssid);
    STA_info.setPass(pass);
}

int8_t WifiConnect::start_connection()
{

    int8_t configure_ret;

    WiFi.mode(WIFI_AP_STA);

    // Setup the device as AP for configuring STA info
    IPAddress local_ip(192,168,1,1);
    IPAddress gateway(192,168,1,255);
    IPAddress subnet(255,255,255,0);

    server.begin(80);

    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP("ESP32Power", "", 10, false, 2);

    configure_ret = configure_STA();
    if(configure_ret != 0)
    {
        return configure_ret;
    }

    return 0;
}

void WifiConnect::set_app_ptr(AppInterface *app_)
{
    app = app_;
}

//======================================================
//            AppInteface definition
//========================================================

AppInterface::AppInterface()
{
    http = new HTTPClient;
}

int16_t AppInterface::UDP_measPayload()
{
    udp_.beginPacket("255.255.255.255", 4444);
    log_i("%s", meas_payload.buf);
    udp_.write((uint8_t*)meas_payload.buf, 34);
    return udp_.endPacket();
}

int8_t AppInterface::setMeasurementPayload(float current_, float voltage_)
{
    meas_payload.current = current_;
    meas_payload.voltage = voltage_;

    sprintf(meas_payload.buf, "current=%09.6f&voltage=%09.6f", current_, voltage_);

    return 0;
}



