// u-blox 9 positioning receivers configuration library test program
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
// https://oinkzwurgl.org/hacking/ubloxcfg
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program.
// If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

#include "ubloxcfg.h"

#define NUMOF(array)    (sizeof(array)/sizeof(*(array)))
#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

static int gVerbosity = 0;

// Hex dump
void HEXDUMP(const char *descr, const void *pkData, int size)
{
    if (gVerbosity <= 0)
    {
        return;
    }
    const char i2hex[] = "0123456789abcdef";
    const char *data = pkData;
    for (int ix = 0; ix < size; )
    {
        char buf[128];
        memset(buf, ' ', sizeof(buf));
        for (int ix2 = 0; (ix2 < 16) && ((ix + ix2) < size); ix2++)
        {
            const uint8_t c = data[ix + ix2];
            buf[3 * ix2    ] = i2hex[ (c >> 4) & 0xf ];
            buf[3 * ix2 + 1] = i2hex[  c       & 0xf ];
            buf[3 * 16 + 2 + ix2] = isprint((int)c) ? c : '.';
            buf[3 * 16 + 3 + ix2] = '\0';
        }
        printf("%s 0x%04x  %s\n", descr ? descr : "", ix, buf);
        ix += 16;
    }
}

// Assertion with result printing
#define TEST(descr, predicate) do { numTests++; \
        if (predicate) \
        { \
            numPass++; \
            if (gVerbosity > 0) { printf("%03d PASS %s: %s [%s:%d]\n", numTests, descr, # predicate, __FILE__, __LINE__); } \
        } \
        else \
        { \
            numFail++; \
            printf("%03d FAIL %s: %s [%s:%d]\n", numTests, descr, # predicate, __FILE__, __LINE__); \
        } \
    } while (0)

int main(int argc, char **argv)
{
    for (int ix = 0; ix < argc; ix++)
    {
        if (strcmp(argv[ix], "-v") == 0)
        {
            gVerbosity++;
        }
    }

    int numTests = 0;
    int numPass = 0;
    int numFail = 0;

    // Data types
    {
        //TEST("bool size is 1",     sizeof(bool) == 1);
        //TEST("int size is 4",      sizeof(int) == 4);
        TEST("float size is 4",    sizeof(float) == 4);
        TEST("double size is 8",   sizeof(double) == 8);
        TEST("int8_t size is 1",   sizeof(int8_t) == 1);
        TEST("int16_t size is 2",  sizeof(int16_t) == 2);
        TEST("int32_t size is 4",  sizeof(int32_t) == 4);
        TEST("int64_t size is 8",  sizeof(int64_t) == 8);
        TEST("uint8_t size is 1",  sizeof(uint8_t) == 1);
        TEST("uint16_t size is 2", sizeof(uint16_t) == 2);
        TEST("uint32_t size is 4", sizeof(uint32_t) == 4);
        TEST("uint64_t size is 8", sizeof(uint64_t) == 8);
        TEST("UBLOXCFG_VALUE_t size is 8", sizeof(UBLOXCFG_VALUE_t) == 8);
    }

    // We (curently) need little endian
    {
        volatile uint32_t test = 0xdeadbeef;
        volatile uint8_t *byteorder = (volatile uint8_t *)&test;
        TEST("byte order is little-endian", (byteorder[0] == 0xef) && (byteorder[1] == 0xbe) && (byteorder[2] == 0xad) && (byteorder[3] == 0xde) );
    }

    // Lookup item by name
    {
        const UBLOXCFG_ITEM_t *item = ubloxcfg_getItemByName(UBLOXCFG_CFG_UBLOXCFGTEST_U1_STR);
        TEST("lookup item by name", (item != NULL) && (item->id == UBLOXCFG_CFG_UBLOXCFGTEST_U1_ID));
        const UBLOXCFG_ITEM_t *fail = ubloxcfg_getItemByName("nope-nope-nope");
        TEST("lookup item by name", (fail == NULL));
        const UBLOXCFG_ITEM_t *item2 = ubloxcfg_getItemByName(STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_U2_ID));
        TEST("lookup item by hex id", (item2 != NULL) && (item2->id == UBLOXCFG_CFG_UBLOXCFGTEST_U2_ID));
    }

    // Lookup item by ID
    {
        const UBLOXCFG_ITEM_t *item = ubloxcfg_getItemById(UBLOXCFG_CFG_UBLOXCFGTEST_U1_ID);
        TEST("lookup item by ID", (item != NULL) && (item->id == UBLOXCFG_CFG_UBLOXCFGTEST_U1_ID) );
        const UBLOXCFG_ITEM_t *fail = ubloxcfg_getItemById(0xffffffff);
        TEST("lookup item by ID", (fail == NULL));
    }

    // Message output rate configs
    {
        const UBLOXCFG_MSGRATE_t *rates = ubloxcfg_getMsgRateCfg(UBLOXCFG_UBX_NAV_PVT_STR);
        TEST("lookup msg rate cfg by name", (rates != NULL));
        if (rates != NULL)
        {
            TEST("msgrate cfg UART1", (rates->itemUart1->id == UBLOXCFG_CFG_MSGOUT_UBX_NAV_PVT_UART1_ID));
            TEST("msgrate cfg UART2", (rates->itemUart2->id == UBLOXCFG_CFG_MSGOUT_UBX_NAV_PVT_UART2_ID));
            TEST("msgrate cfg SPI", (rates->itemSpi->id == UBLOXCFG_CFG_MSGOUT_UBX_NAV_PVT_SPI_ID));
            TEST("msgrate cfg I2C", (rates->itemI2c->id == UBLOXCFG_CFG_MSGOUT_UBX_NAV_PVT_I2C_ID));
            TEST("msgrate cfg USB", (rates->itemUsb->id == UBLOXCFG_CFG_MSGOUT_UBX_NAV_PVT_USB_ID));
        }
    }

    // Test vectors for the encode/decode data tests below
    const UBLOXCFG_KEYVAL_t testKeyVal[] =
    {
        UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_L,    true       ),  // L  0x10fe0001  0x01
        UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_U1,    42        ),  // U1 0x20fe0011  0x2a
        UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_I2,   -42        ),  // I2 0x30fe0022  0xffd6
        UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X4,   0xdeadbeef ),  // X4 0x40fe0033  0xdeadbeef
        UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X8,   0xdeadbeef ),  // X8 0x50fe0034  0x00000000deadbeef
        UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,   1.0f/3.0f  ),  // R4 0x40fe0041 0x3eaaaaab
        UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,   1e-3/3.0   ),  // R8 0x50fe0042 0xef35d867c3ece2a5
        UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E1,   ONE        ),  // E1 0x20fe0041 0x01
        UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E2,   MINUS_ONE  ),  // E2 0x30fe0042 0xffff
        UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E4,   FOUR_HEX   ),  // E4 0x40fe0043 0x00000004
    };
    //HEXDUMP("testKeyVal", testKeyVal, sizeof(testKeyVal));
    const uint8_t testData[4+1 + 4+1 + 4+2 + 4+4 + 4+8 + 4+4 + 4+8 + 4+1 + 4+2 + 4+4] =
    {
        0x01, 0x00, 0xfe, 0x10,   0x01,
        0x11, 0x00, 0xfe, 0x20,   0x2a,
        0x22, 0x00, 0xfe, 0x30,   0xd6, 0xff,
        0x33, 0x00, 0xfe, 0x40,   0xef, 0xbe, 0xad, 0xde,
        0x34, 0x00, 0xfe, 0x50,   0xef, 0xbe, 0xad, 0xde, 0x00, 0x00, 0x00, 0x00,
        0x41, 0x00, 0xfe, 0x40,   0xab, 0xaa, 0xaa, 0x3e,
        0x42, 0x00, 0xfe, 0x50,   0xa5, 0xe2, 0xec, 0xc3, 0x67, 0xd8, 0x35, 0x3f,
        0x41, 0x00, 0xfe, 0x20,   0x01,
        0x42, 0x00, 0xfe, 0x30,   0xff, 0xff,
        0x43, 0x00, 0xfe, 0x40,   0x04, 0x00, 0x00, 0x00
    };
    //HEXDUMP("testData", testData, sizeof(testData));

    // Encode key-value pairs to data
    {
        int dataSize = 0;
        uint8_t data[NUMOF(testKeyVal) * 8];
        const bool makeDataRes = ubloxcfg_makeData(data, sizeof(data), testKeyVal, NUMOF(testKeyVal), &dataSize);
        TEST("encode config data", (makeDataRes) && (dataSize == sizeof(testData)) && (memcmp(testData, data, sizeof(testData)) == 0) );
        //HEXDUMP("data", testData, sizeof(testData));
        //HEXDUMP("test", data, dataSize);
    }

    // Decode data to key-value pairs
    {
        int nKeyVal = 0;
        UBLOXCFG_KEYVAL_t keyVal[NUMOF(testKeyVal)];
        const bool parseDataRes = ubloxcfg_parseData(testData, sizeof(testData), keyVal, NUMOF(keyVal), &nKeyVal);
        TEST("decode config data", (parseDataRes) && (nKeyVal == NUMOF(testKeyVal)) && (memcmp(testKeyVal, keyVal, sizeof(testKeyVal)) == 0) );
        //HEXDUMP("data", testKeyVal, sizeof(testKeyVal));
        //HEXDUMP("test", keyVal, sizeof(keyVal));
    }

    // Stringify values
    {
        typedef struct TEST_VAL_STR_s
        {
            const UBLOXCFG_KEYVAL_t keyVal;
            const char             *expTypeStr;
            const char             *expValStr;
            const bool              expRes;
        } TEST_VAL_STR_t;
        const TEST_VAL_STR_t valStr[] =
        {
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_L,     false      ), .expValStr = "0 (false)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_L_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_L,     true       ), .expValStr = "1 (true)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_L_TYPE), .expRes = true },

            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_U1,    0          ), .expValStr = "0",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_U1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_U1,    UINT8_MAX  ), .expValStr = "255",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_U1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_U2,    0          ), .expValStr = "0",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_U2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_U2,    UINT16_MAX ), .expValStr = "65535",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_U2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_U4,    0          ), .expValStr = "0",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_U4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_U4,    UINT32_MAX ), .expValStr = "4294967295",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_U4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_U8,    0),           .expValStr = "0",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_U8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_U8,   UINT64_MAX ),  .expValStr = "18446744073709551615",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_U8_TYPE), .expRes = true },

            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_I1,    INT8_MIN   ), .expValStr = "-128",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_I1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_I1,    INT8_MAX   ), .expValStr = "127",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_I1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_I2,    INT16_MIN  ), .expValStr = "-32768",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_I2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_I2,    INT16_MAX  ), .expValStr = "32767",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_I2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_I4,    INT32_MIN  ), .expValStr = "-2147483648",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_I4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_I4,    INT32_MAX  ), .expValStr = "2147483647",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_I4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_I8,    INT64_MIN  ), .expValStr = "-9223372036854775808",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_I8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_I8,    INT64_MAX  ), .expValStr = "9223372036854775807",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_I8_TYPE), .expRes = true },

            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    -FLT_MIN   ), .expValStr = NULL, // .expValStr = "-1.17549435082228750796874e-38",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    FLT_MAX    ), .expValStr = NULL, // .expValStr = "3.40282346638528859811704e+38",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    FLT_EPSILON), .expValStr = NULL, // .expValStr = "1.1920928955078125e-07",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    0.0        ), .expValStr = NULL, // .expValStr = "0",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    0.5        ), .expValStr = NULL, // .expValStr = "0.5",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    1.0        ), .expValStr = NULL, // .expValStr = "1",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    1.0/3.0    ), .expValStr = NULL, // .expValStr = "0.333333343267440795898438",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    1e-1/3.0   ), .expValStr = NULL, // .expValStr = "0.0333333350718021392822266",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    1e-2/3.0   ), .expValStr = NULL, // .expValStr = "0.0033333334140479564666748",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    1e-3/3.0   ), .expValStr = NULL, // .expValStr = "0.000333333329763263463973999",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    1e-4/3.0   ), .expValStr = NULL, // .expValStr = "3.33333337039221078157425e-05",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    1e-5/3.0   ), .expValStr = NULL, // .expValStr = "3.33333332491747569292784e-06",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    1e-6/3.0   ), .expValStr = NULL, // .expValStr = "3.33333332491747569292784e-07",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R4,    1e-12/3.0  ), .expValStr = NULL, // .expValStr = "3.33333322966380962704136e-13",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R4_TYPE), .expRes = true },

            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    -DBL_MIN   ), .expValStr = NULL, // .expValStr = "-2.22507385850720138309023271733240406421921598046233183e-308",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    DBL_MAX    ), .expValStr = NULL, // .expValStr = "1.79769313486231570814527423731704356798070567525844997e+308",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    DBL_EPSILON), .expValStr = NULL, // .expValStr = "2.220446049250313080847263336181640625e-16",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    0.0        ), .expValStr = NULL, // .expValStr = "0",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    0.5        ), .expValStr = NULL, // .expValStr = "0.5",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    1.0        ), .expValStr = NULL, // .expValStr = "1",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    1.0/3.0    ), .expValStr = NULL, // .expValStr = "0.333333333333333314829616256247390992939472198486328125",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    1e-1/3.0   ), .expValStr = NULL, // .expValStr = "0.0333333333333333328707404064061847748234868049621582031",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    1e-2/3.0   ), .expValStr = NULL, // .expValStr = "0.00333333333333333354728256203713954164413735270500183105",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    1e-3/3.0   ), .expValStr = NULL, // .expValStr = "0.000333333333333333322202191029148821144190151244401931763",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    1e-4/3.0   ), .expValStr = NULL, // .expValStr = "3.33333333333333349307245341286431994376471266150474548e-05",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    1e-5/3.0   ), .expValStr = NULL, // .expValStr = "3.33333333333333332366586396200425213010021252557635307e-06",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    1e-6/3.0   ), .expValStr = NULL, // .expValStr = "3.33333333333333353542410077557933689718083769548684359e-07",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_R8,    1e-12/3.0  ), .expValStr = NULL, // .expValStr = "3.33333333333333343457915187800123637867823894742613788e-13",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_R8_TYPE), .expRes = true },

            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E1,    ONE       ), .expValStr = "1 (ONE)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E1,    TWO       ), .expValStr = "2 (TWO)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E1,    THREE     ), .expValStr = "3 (THREE)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E1,    MINUS_ONE ), .expValStr = "-1 (MINUS_ONE)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E1,    FOUR_HEX  ), .expValStr = "0x04 (FOUR_HEX)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY(  CFG_UBLOXCFGTEST_E1,    42       ), .expValStr = "42 (n/a)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E1_TYPE), .expRes = true },

            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E2,    ONE       ), .expValStr = "1 (ONE)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E2,    TWO       ), .expValStr = "2 (TWO)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E2,    THREE     ), .expValStr = "3 (THREE)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E2,    MINUS_ONE ), .expValStr = "-1 (MINUS_ONE)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E2,    FOUR_HEX  ), .expValStr = "0x0004 (FOUR_HEX)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY(  CFG_UBLOXCFGTEST_E2,    42       ), .expValStr = "42 (n/a)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E2_TYPE), .expRes = true },

            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E4,    ONE       ), .expValStr = "1 (ONE)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E4,    TWO       ), .expValStr = "2 (TWO)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E4,    THREE     ), .expValStr = "3 (THREE)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E4,    MINUS_ONE ), .expValStr = "-1 (MINUS_ONE)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ENU( CFG_UBLOXCFGTEST_E4,    FOUR_HEX  ), .expValStr = "0x00000004 (FOUR_HEX)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY(  CFG_UBLOXCFGTEST_E4,    42       ), .expValStr = "42 (n/a)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_E4_TYPE), .expRes = true },

            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X1,    UBLOXCFG_CFG_UBLOXCFGTEST_X1_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X1_LAST ), .expValStr = "0x81 (FIRST|LAST)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X1,    0xff               ), .expValStr = "0xff (FIRST|SECOND|LAST|0x7c)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X1,    0x7c               ), .expValStr = "0x7c (n/a)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X1_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X1,    0x00               ), .expValStr = "0x00 (n/a)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X1_TYPE), .expRes = true },

            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X2,    UBLOXCFG_CFG_UBLOXCFGTEST_X2_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X2_LAST ), .expValStr = "0x8001 (FIRST|LAST)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X2,    0xffff             ), .expValStr = "0xffff (FIRST|SECOND|LAST|0x7ffc)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X2,    0x7ffc             ), .expValStr = "0x7ffc (n/a)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X2_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X2,    0x0000             ), .expValStr = "0x0000 (n/a)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X2_TYPE), .expRes = true },

            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X4,    UBLOXCFG_CFG_UBLOXCFGTEST_X4_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X4_LAST ), .expValStr = "0x80000001 (FIRST|LAST)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X4,    0xffffffff         ), .expValStr = "0xffffffff (FIRST|SECOND|LAST|0x7ffffffc)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X4,    0x7ffffffc         ), .expValStr = "0x7ffffffc (n/a)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X4_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X4,    0x00000000         ), .expValStr = "0x00000000 (n/a)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X4_TYPE), .expRes = true },

            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X8,    UBLOXCFG_CFG_UBLOXCFGTEST_X8_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X8_LAST ), .expValStr = "0x8000000000000001 (FIRST|LAST)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X8,    0xffffffffffffffff ), .expValStr = "0xffffffffffffffff (FIRST|SECOND|LAST|0x7ffffffffffffffc)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X8,    0x7ffffffffffffffc ), .expValStr = "0x7ffffffffffffffc (n/a)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X8_TYPE), .expRes = true },
            { .keyVal = UBLOXCFG_KEYVAL_ANY( CFG_UBLOXCFGTEST_X8,    0x0000000000000000 ), .expValStr = "0x0000000000000000 (n/a)",
              .expTypeStr = STRINGIFY(UBLOXCFG_CFG_UBLOXCFGTEST_X8_TYPE), .expRes = true }
        };
        for (int ix = 0; ix < (int)NUMOF(valStr); ix++)
        {
            char valueStr[200];
            const UBLOXCFG_ITEM_t *item = ubloxcfg_getItemById(valStr[ix].keyVal.id);
            TEST("must not fail", (item != NULL));
            if (item != NULL)
            {
                const bool stringifyRes = ubloxcfg_stringifyValue(valueStr, sizeof(valueStr), item->type, item, &valStr[ix].keyVal.val);
                const char *typeStr = ubloxcfg_typeStr(item->type);

                char descr[100];
                snprintf(descr, sizeof(descr), "stringify item %d (%s)", ix + 1, item->name);
                TEST(descr, (stringifyRes == valStr[ix].expRes));
                if (valStr[ix].expValStr != NULL)
                {
                    TEST(descr, (strcmp(valStr[ix].expValStr, valueStr) == 0));
                }
                TEST(descr, (typeStr != NULL) && (strcmp(valStr[ix].expTypeStr, typeStr) == 0));
            }
        }
    }

    // Stringify unknown item
    {
        const struct { UBLOXCFG_KEYVAL_t kv; const char *str; } kvStr[] =
        {
            { .kv = { .id = 0x10fe0ff1, .val = { .L  = true               } }, .str = "CFG-?-? (0x10fe0ff1, ?0) = 0x1" },
            { .kv = { .id = 0x20fe0ff2, .val = { .X1 = 0x12               } }, .str = "CFG-?-? (0x20fe0ff2, ?1) = 0x12" },
            { .kv = { .id = 0x30fe0ff3, .val = { .X2 = 0x1234             } }, .str = "CFG-?-? (0x30fe0ff3, ?2) = 0x1234" },
            { .kv = { .id = 0x40fe0ff4, .val = { .X4 = 0x12345678         } }, .str = "CFG-?-? (0x40fe0ff4, ?4) = 0x12345678" },
            { .kv = { .id = 0x50fe0ff5, .val = { .X8 = 0x1234567890abcdef } }, .str = "CFG-?-? (0x50fe0ff5, ?8) = 0x1234567890abcdef" },
        };
        for (int ix = 0; ix < (int)NUMOF(kvStr); ix++)
        {
            char str[200];
            const bool res = ubloxcfg_stringifyKeyVal(str, sizeof(str), &kvStr[ix].kv);
            TEST("stringify unkn kv res", (res));
            TEST("stringify unkn kv str", (strcmp(kvStr[ix].str, str) == 0));
        }
    }

    // Split stringified value
    {
        char str[100];
        char *vStr;
        char *pStr;
        strcpy(str, "value (pretty)");
        TEST("splitValueStr: value (pretty)", ubloxcfg_splitValueStr(str, &vStr, &pStr) && (strcmp(vStr, "value") == 0) && (strcmp(pStr, "pretty") == 0));
        strcpy(str, "value");
        TEST("splitValueStr: value", ubloxcfg_splitValueStr(str, &vStr, &pStr) && (strcmp(vStr, "value") == 0) && (pStr == NULL));
        strcpy(str, "value (n/a)");
        TEST("splitValueStr: value (n/a)", ubloxcfg_splitValueStr(str, &vStr, &pStr) && (strcmp(vStr, "value") == 0) && (pStr == NULL));
    }

    // Output message rate config aliases
    {
        const UBLOXCFG_KEYVAL_t keyVal[] =
        {
            UBLOXCFG_KEYVAL_ANY( UBX_NAV_PVT_UART1,  1 ),
            UBLOXCFG_KEYVAL_ANY( UBX_NAV_PVT_UART2,  2 ),
            UBLOXCFG_KEYVAL_ANY( UBX_NAV_PVT_SPI,    3 ),
            UBLOXCFG_KEYVAL_ANY( UBX_NAV_PVT_I2C,    4 ),
            UBLOXCFG_KEYVAL_ANY( UBX_NAV_PVT_USB,    5 ),
        };
        const uint8_t expected[5 * (4+1)] =
        {
            0x07, 0x00, 0x91, 0x20,   0x01,
            0x08, 0x00, 0x91, 0x20,   0x02,
            0x0a, 0x00, 0x91, 0x20,   0x03,
            0x06, 0x00, 0x91, 0x20,   0x04,
            0x09, 0x00, 0x91, 0x20,   0x05
        };
        uint8_t data[100];
        int dataSize = 0;
        const bool makeDataRes = ubloxcfg_makeData(data, sizeof(data), keyVal, NUMOF(keyVal), &dataSize);
        TEST("msg rate config data", (makeDataRes) && (dataSize == sizeof(expected)) && (memcmp(data, expected, sizeof(expected)) == 0) );
    }

    // Output message rate config aliases and helper
    {
        const UBLOXCFG_KEYVAL_t keyVal[] =
        {
            UBLOXCFG_KEYVAL_MSG( UBX_NAV_PVT, UART1, 1 ),
            UBLOXCFG_KEYVAL_MSG( UBX_NAV_PVT, UART2, 2 ),
            UBLOXCFG_KEYVAL_MSG( UBX_NAV_PVT, SPI, 3 ),
            UBLOXCFG_KEYVAL_MSG( UBX_NAV_PVT, I2C, 4 ),
            UBLOXCFG_KEYVAL_MSG( UBX_NAV_PVT, USB, 5 ),
        };
        const uint8_t expected[5 * (4+1)] =
        {
            0x07, 0x00, 0x91, 0x20,   0x01,
            0x08, 0x00, 0x91, 0x20,   0x02,
            0x0a, 0x00, 0x91, 0x20,   0x03,
            0x06, 0x00, 0x91, 0x20,   0x04,
            0x09, 0x00, 0x91, 0x20,   0x05
        };
        uint8_t data[100];
        int dataSize = 0;
        const bool makeDataRes = ubloxcfg_makeData(data, sizeof(data), keyVal, NUMOF(keyVal), &dataSize);
        TEST("msg rate config data", (makeDataRes) && (dataSize == sizeof(expected)) && (memcmp(data, expected, sizeof(expected)) == 0) );
    }

    // String to value
    {
        typedef struct TEST_STR_VAL_s
        {
            const char            *str;
            const UBLOXCFG_TYPE_t  type;
            const UBLOXCFG_VALUE_t val;
            const bool             expectedRes;
            const uint32_t         id;
        } TEST_STR_VAL_t;
        const TEST_STR_VAL_t strVal[] =
        {
            { .str = "0",                    .type = UBLOXCFG_TYPE_L,  .val = { .L = false           }, .expectedRes = true  },
            { .str = "0x0",                  .type = UBLOXCFG_TYPE_L,  .val = { .L = false           }, .expectedRes = true  },
            { .str = "1",                    .type = UBLOXCFG_TYPE_L,  .val = { .L = true            }, .expectedRes = true  },
            { .str = "01",                   .type = UBLOXCFG_TYPE_L,  .val = { .L = true            }, .expectedRes = true  },
            { .str = "0x1",                  .type = UBLOXCFG_TYPE_L,  .val = { .L = true            }, .expectedRes = true  },
            { .str = "0x2",                  .type = UBLOXCFG_TYPE_L,  .val = { ._raw = 0            }, .expectedRes = false },
            { .str = "42",                   .type = UBLOXCFG_TYPE_L,  .val = { ._raw = 0            }, .expectedRes = false },

            { .str = "42",                   .type = UBLOXCFG_TYPE_U1, .val = { .U1 = 42             }, .expectedRes = true  },
            { .str = "255",                  .type = UBLOXCFG_TYPE_U1, .val = { .U1 = UINT8_MAX      }, .expectedRes = true  },
            { .str = "0xff",                 .type = UBLOXCFG_TYPE_U1, .val = { .U1 = UINT8_MAX      }, .expectedRes = true  },
            { .str = "256",                  .type = UBLOXCFG_TYPE_U1, .val = { ._raw = 0            }, .expectedRes = false },

            { .str = "42",                   .type = UBLOXCFG_TYPE_U2, .val = { .U2 = 42             }, .expectedRes = true  },
            { .str = "65534",                .type = UBLOXCFG_TYPE_U2, .val = { .U2 = (UINT16_MAX-1) }, .expectedRes = true  },
            { .str = "65535",                .type = UBLOXCFG_TYPE_U2, .val = { .U2 = UINT16_MAX     }, .expectedRes = true  },
            { .str = "0xffff",               .type = UBLOXCFG_TYPE_U2, .val = { .U2 = UINT16_MAX     }, .expectedRes = true  },
            { .str = "65536",                .type = UBLOXCFG_TYPE_U2, .val = { ._raw = 0            }, .expectedRes = false },

            { .str = "42",                   .type = UBLOXCFG_TYPE_U4, .val = { .U4 = 42             }, .expectedRes = true  },
            { .str = "4294967294",           .type = UBLOXCFG_TYPE_U4, .val = { .U4 = (UINT32_MAX-1) }, .expectedRes = true  },
            { .str = "4294967295",           .type = UBLOXCFG_TYPE_U4, .val = { .U4 = UINT32_MAX     }, .expectedRes = true  },
            { .str = "0xffffffff",           .type = UBLOXCFG_TYPE_U4, .val = { .U4 = UINT32_MAX     }, .expectedRes = true  },
            { .str = "4294967296",           .type = UBLOXCFG_TYPE_U4, .val = { ._raw = 0            }, .expectedRes = false },

            { .str = "42",                   .type = UBLOXCFG_TYPE_U8, .val = { .U8 = 42             }, .expectedRes = true  },
            { .str = "18446744073709551614", .type = UBLOXCFG_TYPE_U8, .val = { .U8 = (UINT64_MAX-1) }, .expectedRes = true  },
            { .str = "18446744073709551615", .type = UBLOXCFG_TYPE_U8, .val = { .U8 = UINT64_MAX     }, .expectedRes = true  },
            { .str = "0xffffffffffffffff",   .type = UBLOXCFG_TYPE_U8, .val = { .U8 = UINT64_MAX     }, .expectedRes = true  },
          //{ .str = "18446744073709551616", .type = UBLOXCFG_TYPE_U8, .val = { ._raw = 0            }, .expectedRes = false }, // FIXME: why does this test fail?!

            { .str = "-128",                 .type = UBLOXCFG_TYPE_I1, .val = { .I1 = INT8_MIN       }, .expectedRes = true  },
            { .str = "127",                  .type = UBLOXCFG_TYPE_I1, .val = { .I1 = INT8_MAX       }, .expectedRes = true  },
            { .str = "+126",                 .type = UBLOXCFG_TYPE_I1, .val = { .I1 = (INT8_MAX-1)   }, .expectedRes = true  },
            { .str = "128",                  .type = UBLOXCFG_TYPE_I1, .val = { ._raw = 0            }, .expectedRes = false },
            { .str = "0xff",                 .type = UBLOXCFG_TYPE_I1, .val = { .I1 = -1             }, .expectedRes = true  },

            { .str = "-32768",               .type = UBLOXCFG_TYPE_I2, .val = { .I2 = INT16_MIN      }, .expectedRes = true  },
            { .str = "32767",                .type = UBLOXCFG_TYPE_I2, .val = { .I2 = INT16_MAX      }, .expectedRes = true  },
            { .str = "+32766",               .type = UBLOXCFG_TYPE_I2, .val = { .I2 = (INT16_MAX-1)  }, .expectedRes = true  },
            { .str = "32768",                .type = UBLOXCFG_TYPE_I2, .val = { ._raw = 0            }, .expectedRes = false },
            { .str = "0xffff",               .type = UBLOXCFG_TYPE_I2, .val = { .I2 = -1             }, .expectedRes = true  },

            { .str = "-2147483648",          .type = UBLOXCFG_TYPE_I4, .val = { .I4 = INT32_MIN      }, .expectedRes = true  },
            { .str = "2147483647",           .type = UBLOXCFG_TYPE_I4, .val = { .I4 = INT32_MAX      }, .expectedRes = true  },
            { .str = "+2147483646",          .type = UBLOXCFG_TYPE_I4, .val = { .I4 = (INT32_MAX-1)  }, .expectedRes = true  },
            { .str = "2147483648",           .type = UBLOXCFG_TYPE_I4, .val = { ._raw = 0            }, .expectedRes = false },
            { .str = "0xffffffff",           .type = UBLOXCFG_TYPE_I4, .val = { .I4 = -1             }, .expectedRes = true  },

            { .str = "-9223372036854775808", .type = UBLOXCFG_TYPE_I8, .val = { .I8 = INT64_MIN      }, .expectedRes = true  },
            { .str = "9223372036854775807",  .type = UBLOXCFG_TYPE_I8, .val = { .I8 = INT64_MAX      }, .expectedRes = true  },
            { .str = "+9223372036854775806", .type = UBLOXCFG_TYPE_I8, .val = { .I8 = (INT64_MAX-1)  }, .expectedRes = true  },
          //{ .str = "9223372036854775808",  .type = UBLOXCFG_TYPE_I8, .val = { ._raw = 0            }, .expectedRes = false }, // FIXME: why does this test fail?
            { .str = "0xffffffffffffffff",   .type = UBLOXCFG_TYPE_I8, .val = { .I8 = -1             }, .expectedRes = true  },

            { .str = "0",                                                             .type = UBLOXCFG_TYPE_R4, .val = { .R4 = 0.0         }, .expectedRes = true  },
            { .str = "1.0",                                                           .type = UBLOXCFG_TYPE_R4, .val = { .R4 = 1.0         }, .expectedRes = true  },
            { .str = "-1.17549435082228750796874e-38",                                .type = UBLOXCFG_TYPE_R4, .val = { .R4 = -FLT_MIN    }, .expectedRes = true  },
            { .str = "3.40282346638528859811704e+38",                                 .type = UBLOXCFG_TYPE_R4, .val = { .R4 = FLT_MAX     }, .expectedRes = true  },
            { .str = "1.1920928955078125e-07",                                        .type = UBLOXCFG_TYPE_R4, .val = { .R4 = FLT_EPSILON }, .expectedRes = true  },
            { .str = "0.333333343267440795898438",                                    .type = UBLOXCFG_TYPE_R4, .val = { .R4 = (1.0/3.0)   }, .expectedRes = true  },
            // FIXME: what about +inf/-inf/nan etc.?

            { .str = "0",                                                             .type = UBLOXCFG_TYPE_R8, .val = { .R8 = 0.0         }, .expectedRes = true  },
            { .str = "1.0",                                                           .type = UBLOXCFG_TYPE_R8, .val = { .R8 = 1.0         }, .expectedRes = true  },
            { .str = "-2.22507385850720138309023271733240406421921598046233183e-308", .type = UBLOXCFG_TYPE_R8, .val = { .R8 = -DBL_MIN    }, .expectedRes = true  },
            { .str = "1.79769313486231570814527423731704356798070567525844997e+308",  .type = UBLOXCFG_TYPE_R8, .val = { .R8 = DBL_MAX     }, .expectedRes = true  },
            { .str = "2.220446049250313080847263336181640625e-16",                    .type = UBLOXCFG_TYPE_R8, .val = { .R8 = DBL_EPSILON }, .expectedRes = true  },
            { .str = "0.333333333333333314829616256247390992939472198486328125",      .type = UBLOXCFG_TYPE_R8, .val = { .R8 = (1.0/3.0)   }, .expectedRes = true  },

            { .str = "42",    .type = UBLOXCFG_TYPE_E1, .val = { .E1 = 42       }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_E1_ID, .expectedRes = true  },
            { .str = "ONE",   .type = UBLOXCFG_TYPE_E1, .val = { .E1 = 1        }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_E1_ID, .expectedRes = true  },
            { .str = "NOPE",  .type = UBLOXCFG_TYPE_E1, .val = { ._raw = 0      }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_E1_ID, .expectedRes = false },

            { .str = "42",    .type = UBLOXCFG_TYPE_E2, .val = { .E2 = 42       }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_E2_ID, .expectedRes = true  },
            { .str = "ONE",   .type = UBLOXCFG_TYPE_E2, .val = { .E2 = 1        }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_E2_ID, .expectedRes = true  },
            { .str = "NOPE",  .type = UBLOXCFG_TYPE_E2, .val = { ._raw = 0      }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_E2_ID, .expectedRes = false },

            { .str = "42",    .type = UBLOXCFG_TYPE_E4, .val = { .E4 = 42       }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_E4_ID, .expectedRes = true  },
            { .str = "ONE",   .type = UBLOXCFG_TYPE_E4, .val = { .E4 = 1        }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_E4_ID, .expectedRes = true  },
            { .str = "NOPE",  .type = UBLOXCFG_TYPE_E4, .val = { ._raw = 0      }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_E4_ID, .expectedRes = false },

            { .str = "FIRST|LAST",                           .type = UBLOXCFG_TYPE_X1, .val = { .X1 = UBLOXCFG_CFG_UBLOXCFGTEST_X1_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X1_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X1_ID, .expectedRes = true  },
            { .str = "0x81",                                 .type = UBLOXCFG_TYPE_X1, .val = { .X1 = UBLOXCFG_CFG_UBLOXCFGTEST_X1_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X1_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X1_ID, .expectedRes = true  },
            { .str = "0x01|LAST",                            .type = UBLOXCFG_TYPE_X1, .val = { .X1 = UBLOXCFG_CFG_UBLOXCFGTEST_X1_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X1_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X1_ID, .expectedRes = true  },
            { .str = "FIRST|SECOND|LAST|0x7c",               .type = UBLOXCFG_TYPE_X1, .val = { .X1 = 0xff                                                                 }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X1_ID, .expectedRes = true  },
            { .str = "SECOND|0x7c|FIRST|LAST",               .type = UBLOXCFG_TYPE_X1, .val = { .X1 = 0xff                                                                 }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X1_ID, .expectedRes = true  },
            { .str = "255",                                  .type = UBLOXCFG_TYPE_X1, .val = { .X1 = 0xff                                                                 }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X1_ID, .expectedRes = true  },

            { .str = "FIRST|LAST",                           .type = UBLOXCFG_TYPE_X2, .val = { .X2 = UBLOXCFG_CFG_UBLOXCFGTEST_X2_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X2_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X2_ID, .expectedRes = true  },
            { .str = "0x8001",                               .type = UBLOXCFG_TYPE_X2, .val = { .X2 = UBLOXCFG_CFG_UBLOXCFGTEST_X2_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X2_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X2_ID, .expectedRes = true  },
            { .str = "0x0001|LAST",                          .type = UBLOXCFG_TYPE_X2, .val = { .X2 = UBLOXCFG_CFG_UBLOXCFGTEST_X2_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X2_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X2_ID, .expectedRes = true  },
            { .str = "FIRST|SECOND|LAST|0x7ffc",             .type = UBLOXCFG_TYPE_X2, .val = { .X2 = 0xffff                                                               }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X2_ID, .expectedRes = true  },
            { .str = "SECOND|0x7ffc|FIRST|LAST",             .type = UBLOXCFG_TYPE_X2, .val = { .X2 = 0xffff                                                               }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X2_ID, .expectedRes = true  },
            { .str = "65535",                                .type = UBLOXCFG_TYPE_X2, .val = { .X2 = 0xffff                                                               }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X2_ID, .expectedRes = true  },

            { .str = "FIRST|LAST",                           .type = UBLOXCFG_TYPE_X4, .val = { .X4 = UBLOXCFG_CFG_UBLOXCFGTEST_X4_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X4_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X4_ID, .expectedRes = true  },
            { .str = "0x80000001",                           .type = UBLOXCFG_TYPE_X4, .val = { .X4 = UBLOXCFG_CFG_UBLOXCFGTEST_X4_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X4_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X4_ID, .expectedRes = true  },
            { .str = "0x00000001|LAST",                      .type = UBLOXCFG_TYPE_X4, .val = { .X4 = UBLOXCFG_CFG_UBLOXCFGTEST_X4_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X4_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X4_ID, .expectedRes = true  },
            { .str = "FIRST|SECOND|LAST|0x7ffffffc",         .type = UBLOXCFG_TYPE_X4, .val = { .X4 = 0xffffffff                                                           }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X4_ID, .expectedRes = true  },
            { .str = "SECOND|0x7ffffffc|FIRST|LAST",         .type = UBLOXCFG_TYPE_X4, .val = { .X4 = 0xffffffff                                                           }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X4_ID, .expectedRes = true  },
            { .str = "4294967295",                           .type = UBLOXCFG_TYPE_X4, .val = { .X4 = 0xffffffff                                                           }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X4_ID, .expectedRes = true  },


            { .str = "FIRST|LAST",                           .type = UBLOXCFG_TYPE_X8, .val = { .X8 = UBLOXCFG_CFG_UBLOXCFGTEST_X8_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X8_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X8_ID, .expectedRes = true  },
            { .str = "0x8000000000000001",                   .type = UBLOXCFG_TYPE_X8, .val = { .X8 = UBLOXCFG_CFG_UBLOXCFGTEST_X8_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X8_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X8_ID, .expectedRes = true  },
            { .str = "0x0000000000000001|LAST",              .type = UBLOXCFG_TYPE_X8, .val = { .X8 = UBLOXCFG_CFG_UBLOXCFGTEST_X8_FIRST|UBLOXCFG_CFG_UBLOXCFGTEST_X8_LAST }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X8_ID, .expectedRes = true  },
            { .str = "FIRST|SECOND|LAST|0x7ffffffffffffffc", .type = UBLOXCFG_TYPE_X8, .val = { .X8 = 0xffffffffffffffff                                                   }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X8_ID, .expectedRes = true  },
            { .str = "SECOND|0x7ffffffffffffffc|FIRST|LAST", .type = UBLOXCFG_TYPE_X8, .val = { .X8 = 0xffffffffffffffff                                                   }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X8_ID, .expectedRes = true  },
            { .str = "18446744073709551615",                 .type = UBLOXCFG_TYPE_X8, .val = { .X8 = 0xffffffffffffffff                                                   }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X8_ID, .expectedRes = true  },

            // Bad input
            { .str = "NOPE",                                 .type = UBLOXCFG_TYPE_E4, .val = { ._raw = 0 }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_E4_ID, .expectedRes = false   },

            { .str = "666 ",                                 .type = UBLOXCFG_TYPE_I4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = " 666",                                 .type = UBLOXCFG_TYPE_I4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = " 666 ",                                .type = UBLOXCFG_TYPE_I4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = "bad666",                               .type = UBLOXCFG_TYPE_I4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = "666bad",                               .type = UBLOXCFG_TYPE_I4, .val = { ._raw = 0 }, .expectedRes = false   },

            { .str = "666 ",                                 .type = UBLOXCFG_TYPE_U4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = " 666",                                 .type = UBLOXCFG_TYPE_U4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = " 666 ",                                .type = UBLOXCFG_TYPE_U4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = "bad666",                               .type = UBLOXCFG_TYPE_U4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = "666bad",                               .type = UBLOXCFG_TYPE_U4, .val = { ._raw = 0 }, .expectedRes = false   },

            { .str = "666 ",                                 .type = UBLOXCFG_TYPE_X4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = " 666",                                 .type = UBLOXCFG_TYPE_X4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = " 666 ",                                .type = UBLOXCFG_TYPE_X4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = "bad666",                               .type = UBLOXCFG_TYPE_X4, .val = { ._raw = 0 }, .expectedRes = false   },
            { .str = "666bad",                               .type = UBLOXCFG_TYPE_X4, .val = { ._raw = 0 }, .expectedRes = false   },

            { .str = "0xbadhex00",                           .type = UBLOXCFG_TYPE_I4, .val = { ._raw = 0 }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_I4_ID, .expectedRes = false   },
            { .str = "0xbadhex00",                           .type = UBLOXCFG_TYPE_U4, .val = { ._raw = 0 }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_U4_ID, .expectedRes = false   },
            { .str = "0xbadhex00",                           .type = UBLOXCFG_TYPE_X4, .val = { ._raw = 0 }, .id = UBLOXCFG_CFG_UBLOXCFGTEST_X4_ID, .expectedRes = false   }
        };
        for (int ix = 0; ix < (int)NUMOF(strVal); ix++)
        {
            UBLOXCFG_VALUE_t val = { ._raw = 0 };
            const UBLOXCFG_ITEM_t *item = strVal[ix].id != 0 ? ubloxcfg_getItemById(strVal[ix].id) : NULL;
            const bool res = ubloxcfg_valueFromString(strVal[ix].str, strVal[ix].type, item, &val);
            char descr[100];
            snprintf(descr, sizeof(descr), "str to value %d (%s)", ix + 1, strVal[ix].str);
            TEST(descr, (res == strVal[ix].expectedRes));
            TEST(descr, (val._raw == strVal[ix].val._raw));
        }
    }

    // Configuration layers
    {
        TEST("layer name RAM",     strcmp(ubloxcfg_layerName(UBLOXCFG_LAYER_RAM),     "RAM")     == 0);
        TEST("layer name BBR",     strcmp(ubloxcfg_layerName(UBLOXCFG_LAYER_BBR),     "BBR")     == 0);
        TEST("layer name Flash",   strcmp(ubloxcfg_layerName(UBLOXCFG_LAYER_FLASH),   "Flash")   == 0);
        TEST("layer name Default", strcmp(ubloxcfg_layerName(UBLOXCFG_LAYER_DEFAULT), "Default") == 0);
        TEST("bad layer name", strcmp(ubloxcfg_layerName((UBLOXCFG_LAYER_t)99), "?") == 0);

        UBLOXCFG_LAYER_t layer;
        TEST("layer val 'RAM'",     ubloxcfg_layerFromName("RAM",     &layer) && (layer == UBLOXCFG_LAYER_RAM));
        TEST("layer val 'ram'",     ubloxcfg_layerFromName("ram",     &layer) && (layer == UBLOXCFG_LAYER_RAM));
        TEST("layer val 'Ram'",     ubloxcfg_layerFromName("Ram",     &layer) && (layer == UBLOXCFG_LAYER_RAM));
        TEST("layer val 'rAm'",     ubloxcfg_layerFromName("rAm",     &layer) && (layer == UBLOXCFG_LAYER_RAM));
        TEST("layer val 'BBR'",     ubloxcfg_layerFromName("BBR",     &layer) && (layer == UBLOXCFG_LAYER_BBR));
        TEST("layer val 'Flash'",   ubloxcfg_layerFromName("Flash",   &layer) && (layer == UBLOXCFG_LAYER_FLASH));
        TEST("layer val 'Default'", ubloxcfg_layerFromName("Default", &layer) && (layer == UBLOXCFG_LAYER_DEFAULT));
        TEST("layer val 'meier'",  !ubloxcfg_layerFromName("meier", &layer));
    }

    // Examples from ubloxcfg.h
    {
        const UBLOXCFG_KEYVAL_t keyVal[] =
        {
            UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_INIFIX3D,     true ),
            UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_WKNROLLOVER,  2099 ),
            UBLOXCFG_KEYVAL_ENU( CFG_NAVSPG_FIXMODE,      AUTO ),
            UBLOXCFG_KEYVAL_MSG( UBX_NAV_PVT,   UART1,    1),
            UBLOXCFG_KEYVAL_MSG( UBX_MON_COMMS, UART1,    5)
        };
        const int numKeyVal = sizeof(keyVal) / sizeof(*keyVal);
        TEST("makeData example numKeys", (numKeyVal == 5));
        uint8_t data[100];
        int size;
        const bool res = ubloxcfg_makeData(data, sizeof(data), keyVal, numKeyVal, &size);
        TEST("makeData example size", (res) && (size == 26));
    }
    {
        const uint8_t data[26] =
        {
            0x13, 0x00, 0x11, 0x10,   0x01,       // CFG-NAVSPG-INIFIX3D            = 1 (true)
            0x17, 0x00, 0x11, 0x30,   0x33, 0x08, // CFG-NAVSPG-WKNROLLOVER         = 2099 (0x0833)
            0x11, 0x00, 0x11, 0x20,   0x03,       // CFG-NAVSPG-FIXMODE             = 3 (AUTO)
            0x07, 0x00, 0x91, 0x20,   0x01,       // CFG-MSGOUT-UBX_NAV_PVT_UART1   = 1
            0x50, 0x03, 0x91, 0x20,   0x05        // CFG-MSGOUT-UBX_MON_COMMS_UART1 = 1
        };
        int numKeyVal = 0;
        UBLOXCFG_KEYVAL_t keyVal[20];
        const bool res = ubloxcfg_parseData(data, sizeof(data), keyVal, sizeof(keyVal)/sizeof(*keyVal), &numKeyVal);
        TEST("parseData example numKeyVal", (res) && (numKeyVal == 5));
        TEST("parseData example keyVal[0]", (keyVal[0].id == UBLOXCFG_CFG_NAVSPG_INIFIX3D_ID) && (keyVal[0].val.L));
        TEST("parseData example keyVal[1]", (keyVal[1].id == UBLOXCFG_CFG_NAVSPG_WKNROLLOVER_ID) && (keyVal[1].val.U2 == 2099));
    }
    {
        const UBLOXCFG_KEYVAL_t keyVal[] =
        {
            UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_INIFIX3D,      true ),
            UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_WKNROLLOVER,   2099 ),
            UBLOXCFG_KEYVAL_ENU( CFG_NAVSPG_FIXMODE,       AUTO ),
            UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_INFIL_MINCNO,  30   ),
            UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_OUTFIL_PDOP,   20   ),
            UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_CONSTR_ALT,    234  ),
            UBLOXCFG_KEYVAL_MSG( UBX_NAV_PVT, UART1,       1    ),
            { .id = 0x30fe0ff3, .val = { .U2 = 48879 } }
        };
        const int numKeyVal = sizeof(keyVal) / sizeof(*keyVal);
        TEST("stringifyKeyVal exmaple numKeyVal", (numKeyVal == 8));
        for (int ix = 0; ix < numKeyVal; ix++)
        {
            char str[200];
            const bool res = ubloxcfg_stringifyKeyVal(str, sizeof(str), &keyVal[ix]);
            TEST("stringifyKeyVal example", (res));
        }
    }

    // Analyse results
    printf("%d tests: %d passed, %d failed\n", numTests, numPass, numFail);
    if (numFail != 0)
    {
        printf("%d/%d tests failed!\n", numFail, numTests);
        return(EXIT_FAILURE);
    }
    else
    {
        return(EXIT_SUCCESS);
    }
}