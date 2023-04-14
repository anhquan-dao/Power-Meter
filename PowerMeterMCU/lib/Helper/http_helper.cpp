#include <Helper.h>

String url_str_container;
String decode_str_container;

int8_t HTTPHelper::handleHTTP(String http_request, String *route, char *buffer, uint16_t buffer_size)
{
    Request_t request_type;
    http_request.indexOf("POST /")>=0 ? request_type = Request_t::POST :
    (http_request.indexOf("GET /")>=0 ? request_type = Request_t::GET : NONE);

    switch(request_type)
    {
        case POST:
            HTTPHelper::handlePOST(http_request, route, buffer, buffer_size);
            break;

        case GET:
            HTTPHelper::handleGET(http_request, route, buffer, buffer_size);
            break;

        case NONE:
            HTTPHelper::handleResponse(http_request, route, buffer, buffer_size);
            break;
    };

    return request_type;
}

void HTTPHelper::handlePOST(String http_request, String *route, char *buffer, uint16_t buffer_size)
{
    // Parse content length
    // uint16_t content_length_idx = http_request.indexOf("Content-Length:") + 16;
    // uint16_t content_length = (http_request[content_length_idx] - 0x0030)*10;
    // content_length += http_request[content_length_idx+1] - 0x0030;

    uint16_t content_idx = http_request.indexOf("\r\n\r\n") + 4;

    memset(buffer, 0, buffer_size);
    if(http_request.indexOf(" HTTP/1.1") == 6)
    {
        return;
    }

    *route = http_request.substring(6, http_request.indexOf(" HTTP/1.1"));
    http_request.toCharArray(buffer, buffer_size-1, content_idx);
}

void HTTPHelper::handleGET(String http_request, String *route, char *buffer, uint16_t buffer_size)
{
    String requested_resource;
    memset(buffer, 0, buffer_size);
    uint8_t protocol;
    if(http_request.indexOf(" HTTP/1.1") == 5)
    {
        return;
    }
    requested_resource = http_request.substring(5, http_request.indexOf(" HTTP/1.1"));
    requested_resource.toCharArray(buffer, buffer_size-1);
}

void HTTPHelper::handleResponse(String http_request, String *route, char *buffer, uint16_t buffer_size)
{
    
}

void HTTPHelper::decodeURLString(const String url_string, String *decode_string)
{   
    url_str_container = url_string;
    decode(decode_string);    
}

void HTTPHelper::decodeURLString(const char *url_string, String *decode_string)
{
    url_str_container = url_string;
    decode(decode_string);
}

void HTTPHelper::decodeURLString(const String url_string, char *decode_string)
{
    url_str_container = url_string;
    decode(decode_string);
}

void HTTPHelper::decodeURLString(const char *url_string, char *decode_string)
{
    url_str_container = url_string;
    decode(decode_string);
}

void HTTPHelper::decode(char *decode_string)
{
    decode_str_container = decode_string;
    decode(&decode_str_container);

    decode_str_container.toCharArray(decode_string, decode_str_container.length()+1);
    // Add null character at the end
    *(decode_string + decode_str_container.length()) = '\0';
}

void HTTPHelper::decode(String *decode_string)
{
    if(url_str_container.isEmpty())
    {
        url_str_container.concat(*decode_string);
    }

    decode_string->clear();

    char encode_hex[2];

    for(int i=0; i < url_str_container.length(); i++)
    {   
        if(url_str_container[i] == '%')
        {
            encode_hex[0] = url_str_container[i+1];
            encode_hex[1] = url_str_container[i+2];

            //Gonky way to decode special characters in an URL string
            encode_hex[0] >= 'a' ? encode_hex[0] -= 'a' - 'A' : 0;
            encode_hex[0] >= 'A' ? encode_hex[0] -= 'A' - 10 : 
                                    encode_hex[0] -= '0';

            encode_hex[1] >= 'a' ? encode_hex[1] -= 'a' - 'A' : 0;
            encode_hex[1] >= 'A' ? encode_hex[1] -= 'A' - 10 : 
                                    encode_hex[1] -= '0';

            (*decode_string) +=  (char)(16*encode_hex[0] + encode_hex[1]);
            i += 2;
        }
        else if (url_str_container[i] == '+')
        {
            (*decode_string) += ' ';
        }
        else
        {
            (*decode_string) += url_str_container[i];
        }
    }

    url_str_container.clear();
}