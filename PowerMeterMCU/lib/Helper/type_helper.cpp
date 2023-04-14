#include <Helper.h>

uint8_t TypeHelper::get_char_array_length(char *input)
{
    uint8_t input_length = 0;
    while (input[input_length] != '\0') {
        input_length++;
    }
    input_length++;
    return input_length;
}

uint8_t TypeHelper::get_char_array_length(uint8_t *input)
{
    return get_char_array_length((char*)input);
}
