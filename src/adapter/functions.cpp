/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <climits>
#include <LPC15xx.h>
#include <lstring.h>
#include <algorithms.h>
#include "adaptertypes.h"

using namespace std;
using namespace util;

/**
 * Do Binary/ASCII conversion for CAN identifier
 * @param[in]  num The number to convert
 * @param[out] str The output string
 * @param[in]  extended CAN 29 bit flag
 * @param[in]  spaces Use spaces flag
 */
void CanIDToString(uint32_t num, string& str, bool extended, bool spaces)
{
    Spacer spacer(str, spaces);
    IntAggregate value(num);

    if (!extended) { // 11 bit standard CAN identifier
        str += to_ascii(value.bvalue[1] & 0x0F);
        str += to_ascii(value.bvalue[0] >> 4);
        str += to_ascii(value.bvalue[0] & 0x0F);
    }
    else { // 29 bit extended CAN identifier
        str += to_ascii(value.bvalue[3] >> 4);
        str += to_ascii(value.bvalue[3] & 0x0F);
        spacer.space();
        str += to_ascii(value.bvalue[2] >> 4);
        str += to_ascii(value.bvalue[2] & 0x0F);
        spacer.space();
        str += to_ascii(value.bvalue[1] >> 4);
        str += to_ascii(value.bvalue[1] & 0x0F);
        spacer.space();
        str += to_ascii(value.bvalue[0] >> 4);
        str += to_ascii(value.bvalue[0] & 0x0F);
    }
}

/**
 * Do Binary/ASCII conversion for CAN identifier
 * @param[in]  num The number to convert
 * @param[out] str The output string
 * @param[in]  extended CAN 29 bit flag
 */
void CanIDToString(uint32_t num, string& str, bool extended)
{
    bool useSpaces = AdapterConfig::instance()->getBoolProperty(PAR_SPACES);
    CanIDToString(num, str, extended, useSpaces);
}

/**
 * Delay for number of milliseconds using SysTick timer
 * @param[in] value The number of millisecond to delay
 */
void Delay1ms(uint32_t value)
{
    if (value == 0) return;
    
    // Use the SysTick to generate the timeout in msecs
    SysTick->LOAD = value * (SystemCoreClock / 1000);
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    while (!(SysTick->CTRL & 0x10000)) {
        ;
    }
}

/**
 * Delay for number of microseconds using SysTick timer
 * @param[in] value The number of microseconds to delay
 */
void Delay1us(uint32_t value)
{
    const uint32_t AdjustValue = 50;
    // Use the SysTick to generate the timeout in us
    SysTick->LOAD = value * (SystemCoreClock / 1000000) - AdjustValue;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    while (!(SysTick->CTRL & 0x10000)) {
        ;
    }
}

/**
 * Binary/ASCII ISO 9141/14230 key words conversion
 * @param[in] kw Keyword byte to convert
 * @param[out] str The output string
 */
void KWordsToString(const uint8_t* kw, string& str)
{
    const char pattern[] = "1:-- 2:--";
    str = pattern;
    if (kw[0]) {
        str[2] = to_ascii(kw[0] >> 4);
        str[3] = to_ascii(kw[0] & 0x0F);
        str[7] = to_ascii(kw[1] >> 4);
        str[8] = to_ascii(kw[1] & 0x0F);
    }    
}

/**
 * Generic string to binary conversion function.
 * @param[in] str String to convert
 * @param[out] bytes The result as sequence of bytes
 * @return The length of output
 **/
uint32_t to_bytes(const string& str, uint8_t* bytes)
{
    int len = str.length();
    
    if ((len % 2) != 0) {
        len--; // Skip the last digit
    }
    
    int j = 0;
    for (int i = 0; i < len / 2; i++) {
        uint32_t hexValue = stoul(str.substr(j, 2), 0, 16);
        if (hexValue == ULONG_MAX)
            return 0;
        bytes[i] = hexValue;
        j += 2;
    }
    return len / 2;
}

/**
 * Generic binary to string conversion function.
 * @param[in] bytes The byte array to convert
 * @param[in] length The buffer length
 * @param[out] str The output string
 **/
void to_ascii(const uint8_t* bytes, uint32_t length, string& str)
{
    Spacer spacer(str);
    for (int i = 0; i < length; i++) {
        str += to_ascii(bytes[i] >> 4);
        str += to_ascii(bytes[i] & 0x0F);
        spacer.space();
    }
    if (spacer.isSpaces() && str.length() > 0) {
        str.resize(str.length() - 1); // Truncate the last space
    }
}

static uint32_t reverse2bytes(uint32_t val) 
{
    return (val & 0xFF) << 8 | (val & 0xFF00) >> 8;
}

static uint32_t reverse4bytes(uint32_t val) 
{
    return (val & 0xFF) << 24 | (val & 0xFF00) << 8 
         | (val & 0xFF0000) >> 8 | (val & 0xFF000000) >> 24;
}

/**
 * Parse command for ARCRA, set the mask and filter value 
 * @param[in] The command line to parse
 * @param[out] filter CAN filter value
 * @param[out] mask CAN mask value
 **/
void AutoReceiveParse(const string& str, uint32_t& filter_, uint32_t& mask_) 
{
    uint32_t filter = 0, mask = 0;
    
    for (int i = 0; i < str.length(); i++) {
        if (str[i] == 'X') {
            mask   &= 0xFFFFFFF0;
            filter &= 0xFFFFFFF0;
        }
        else {
            mask &= 0xFFFFFFF0;
            mask |= 0x0F;
            filter &= 0xFFFFFFF0;
            filter |= stoul(str.substr(i, 1), 0, 16);
        }
        if (i < str.length() - 1) {
            mask <<= 4;
            filter <<= 4;
        }
    }
    if (str.length() == 3) {
        mask &= 0x7FF;
        mask_ = reverse2bytes(mask);
        filter_= reverse2bytes(filter);
    }
    else if (str.length() == 8) {
        mask &= 0x1FFFFFFF;
        mask_ = reverse4bytes(mask);
        filter_= reverse4bytes(filter);
    }
}

/**
 * Generic byte order reversal algorithm
 * @param[in] bytes The byte array to revert
 * @param[in] length The buffer length
 **/
void ReverseBytes(uint8_t* bytes, uint32_t length)
{
    int end = length - 1;
    for (int i = 0; i < end; i++) {
        uint8_t tmp = bytes[i];
        bytes[i] = bytes[end];
        bytes[end--] = tmp;
    }
}
