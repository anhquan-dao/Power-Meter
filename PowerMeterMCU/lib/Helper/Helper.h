#pragma once

#include "Arduino.h"

class TypeHelper
{   
    private:
        TypeHelper(){}
        ~TypeHelper(){}

    public:
        static uint8_t get_char_array_length(const char *input);
        static uint8_t get_char_array_length(const uint8_t *input);
};

class HTTPHelper
{
    private:
        HTTPHelper(){}
        ~HTTPHelper(){}

    public:
        static int8_t handleHTTP(String http_request, String *route, char *buffer, uint16_t buffer_size);
        static void handlePOST(String http_request, String *route, char *buffer, uint16_t buffer_size);
        static void handleGET(String http_request, String *route, char *buffer, uint16_t buffer_size);
        static void handleResponse(String http_reqest, String *route, char *buffer, uint16_t buffer_size);

        static void decodeURLString(const String url_string, String *decode_string);
        static void decodeURLString(const char *url_string, String *decode_string);
        static void decodeURLString(const String url_string, char *decode_string);
        static void decodeURLString(const char *url_string, char *decode_string);
        static void decode(char *decode_string);
        static void decode(String *decode_string);

        enum Request_t
        {
            GET,
            POST,
            NONE=-1
        };
};