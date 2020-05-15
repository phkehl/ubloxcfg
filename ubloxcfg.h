/*!
    \file
    \brief u-blox 9 positioning receivers configuration library

    - Copyright (c) 2020 Philippe Kehl (flipflip at oinkzwurgl dot org),
      https://oinkzwurgl.org/hacking/ubloxcfg

    This program is free software: you can redistribute it and/or modify it under the terms of the
    GNU Lesser General Public License as published by the Free Software Foundation, either version 3
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with this
    program. If not, see <https://www.gnu.org/licenses/>.

    \defgroup UBLOXCFG Receiver configuration
    @{
*/

#ifndef __UBLOXCFG_H__
#define __UBLOXCFG_H__

#include <stdint.h>
#include <stdbool.h>

#include "ubloxcfg_gen.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ********************************************************************************************** */

/*!
    \defgroup UBLOXCFG_DEFS Configuration definition
    @{
*/

//! Configuration item size
typedef enum UBLOXCFG_SIZE_e
{
    UBLOXCFG_SIZE_BIT   = 0x01, //!< One bit
    UBLOXCFG_SIZE_ONE   = 0x02, //!< One byte
    UBLOXCFG_SIZE_TWO   = 0x03, //!< Two bytes
    UBLOXCFG_SIZE_FOUR  = 0x04, //!< Four bytes
    UBLOXCFG_SIZE_EIGHT = 0x05  //!< Eight bytes
} UBLOXCFG_SIZE_t;

//! Get item size from item ID \hideinitializer
/*!
    \param[in] id  The item ID
    \returns The item size (#UBLOXCFG_SIZE_t)

    \note No range checks are performed. Bad input can return a bad #UBLOXCFG_SIZE_t.
*/
#define UBLOXCFG_ID2SIZE(id) (UBLOXCFG_SIZE_t)(((id) >> 28) & 0x0f)

//! Configuration item storage type (s.a. #UBLOXCFG_VALUE_t)
typedef enum UBLOXCFG_TYPE_e
{
    UBLOXCFG_TYPE_U1, //!< One byte unsigned, little-endian (uint8_t)
    UBLOXCFG_TYPE_U2, //!< Two bytes unsigned, little-endian (uint16_t)
    UBLOXCFG_TYPE_U4, //!< Four bytes unsigned, little-endian (uint32_t)
    UBLOXCFG_TYPE_U8, //!< Eight bytes unsigned, little-endian (uint64_t)
    UBLOXCFG_TYPE_I1, //!< One byte signed, little-endian (int8_t)
    UBLOXCFG_TYPE_I2, //!< Two bytes signed, little-endian (int16_t)
    UBLOXCFG_TYPE_I4, //!< Four bytes signed, little-endian (int32_t)
    UBLOXCFG_TYPE_I8, //!< Eight byte signed, little-endian (int64_t)
    UBLOXCFG_TYPE_X1, //!< One byte unsigned, little-endian (uint8_t)
    UBLOXCFG_TYPE_X2, //!< Two bytes unsigned, little-endian (uint16_t)
    UBLOXCFG_TYPE_X4, //!< Four bytes unsigned, little-endian (uint32_t)
    UBLOXCFG_TYPE_X8, //!< Eight bytes unsigned, little-endian (uint64_t)
    UBLOXCFG_TYPE_R4, //!< Four bytes IEEE754 single precision (float)
    UBLOXCFG_TYPE_R8, //!< Eight bytes IEEE754 double precision (double)
    UBLOXCFG_TYPE_E1, //!< One byte unsigned, little-endian (int8_t)
    UBLOXCFG_TYPE_E2, //!< Two bytes unsigned, little-endian (int16_t)
    UBLOXCFG_TYPE_E4, //!< Four bytes unsigned, little-endian (int32_t)
    UBLOXCFG_TYPE_L   //!< One bit logical (0 = false, 1 = true)
} UBLOXCFG_TYPE_t;

//! Constants for type E1/E2/E4 configuration items
typedef struct UBLOXCFG_CONST_s
{
    const char         *name;  //!< Name of the constant
    const char         *title; //!< Title
    const char         *value; //!< Value as string
    union
    {
        int32_t          E;   //!< E type value as number
        int64_t          X;   //!< X type value as number
    } val; //!< Value
} UBLOXCFG_CONST_t;

//! Configuration item
typedef struct UBLOXCFG_ITEM_s
{
    uint32_t                id;        //!< Item ID
    UBLOXCFG_TYPE_t         type;      //!< Storage type
    UBLOXCFG_SIZE_t         size;      //!< Item size
    const char             *name;      //!< Item name
    const char             *title;     //!< Title
    const char             *unit;      //!< Unit (or NULL)
    const char             *scale;     //!< Scale factor as string (or NULL)
    const UBLOXCFG_CONST_t *consts;    //!< Constants (or NULL if none)
    int                     nConsts;   //!< Number of constants (or 0 if none)
    double                  scalefact; //!< Scale factor as number
} UBLOXCFG_ITEM_t;

//! Configuration items for output message rate configuration
typedef struct UBLOXCFG_MSGRATE_s
{
    const char            *msgName;    //!< Message name
    const UBLOXCFG_ITEM_t *itemUart1;  //!< Item for output rate on UART1 port (or NULL)
    const UBLOXCFG_ITEM_t *itemUart2;  //!< Item for output rate on UART2 port (or NULL)
    const UBLOXCFG_ITEM_t *itemSpi;    //!< Item for output rate on SPI port (or NULL)
    const UBLOXCFG_ITEM_t *itemI2c;    //!< Item for output rate on I2C port (or NULL)
    const UBLOXCFG_ITEM_t *itemUsb;    //!< Item for output rate on USB port (or NULL)
} UBLOXCFG_MSGRATE_t;

//! Get configuration item info by name
/*!
    \param[in] name  Name of the configuration item (e.g. "CFG-NAVSPG-FIXMODE")
                     or hexadecimal string of the item ID (e.g. "0x20110011")

    \returns Configuration item if found, NULL otherwise
*/
const UBLOXCFG_ITEM_t *ubloxcfg_getItemByName(const char *name);

//! Get configuration item info by key ID
/*!
    \param[in] id  ID of the configuration item (e.g. 0x20110011)

    \returns The item if found, NULL otherwise
*/
const UBLOXCFG_ITEM_t *ubloxcfg_getItemById(const uint32_t id);

//! Get list of all items
/*!
    \param[out] num  Number of items returned

    \returns List of items
*/
const UBLOXCFG_ITEM_t **ubloxcfg_getAllItems(int *num);

//! Get configuration items for output message rate configuration
/*!
    \param[in] msgName   The name of the message (see \ref UBLOXCFG_MSGOUT)

    \return Configuration items if found, NULL otherwise
*/
const UBLOXCFG_MSGRATE_t *ubloxcfg_getMsgRateCfg(const char *msgName);

//! Get list of all output message rate configurations
/*!
    \param[out] num  Number of output message rate configurations returned

    \returns List of output message rate configurations
*/
const UBLOXCFG_MSGRATE_t **ubloxcfg_getAllMsgRateCfgs(int *num);

///@}

/* ********************************************************************************************** */

/*!
    \defgroup UBLOXCFG_DATA Configuration data
    @{
*/

//! Configuration value storage (s.a. #UBLOXCFG_TYPE_t)
typedef union UBLOXCFG_VALUE_u
{
    uint8_t   U1; //!< #UBLOXCFG_TYPE_U1 type value
    uint16_t  U2; //!< #UBLOXCFG_TYPE_U2 type value
    uint32_t  U4; //!< #UBLOXCFG_TYPE_U4 type value
    uint64_t  U8; //!< #UBLOXCFG_TYPE_U8 type value
    int8_t    I1; //!< #UBLOXCFG_TYPE_I1 type value
    int16_t   I2; //!< #UBLOXCFG_TYPE_I2 type value
    uint32_t  I4; //!< #UBLOXCFG_TYPE_I4 type value
    int64_t   I8; //!< #UBLOXCFG_TYPE_I8 type value
    uint8_t   X1; //!< #UBLOXCFG_TYPE_X1 type value
    uint16_t  X2; //!< #UBLOXCFG_TYPE_X2 type value
    uint32_t  X4; //!< #UBLOXCFG_TYPE_X4 type value
    uint64_t  X8; //!< #UBLOXCFG_TYPE_X8 type value
    float     R4; //!< #UBLOXCFG_TYPE_R4 type value
    double    R8; //!< #UBLOXCFG_TYPE_R8 type value
    int8_t    E1; //!< #UBLOXCFG_TYPE_E1 type value
    int16_t   E2; //!< #UBLOXCFG_TYPE_E2 type value
    int32_t   E4; //!< #UBLOXCFG_TYPE_E4 type value
    bool      L;  //!< #UBLOXCFG_TYPE_L type value
    uint8_t  _bytes[8]; //!< raw bytes
    uint64_t _raw;      //!< raw value
} UBLOXCFG_VALUE_t;

//! Key-value pair
typedef struct UBLOXCFG_KEYVAL_s
{
    uint32_t         id;       //!< Configuration item ID
    UBLOXCFG_VALUE_t val;      //!< Configuration item value
} UBLOXCFG_KEYVAL_t;

//! Initialiser for a #UBLOXCFG_KEYVAL_t for any type \hideinitializer
/*!
    \param[in]  name   The name of the configuration item (with underscores instead of minus, e.g. CFG_NAVSPG_FIXMODE)
    \param[in]  value  The value (of appropriate type, e.g. 1)

    \returns The #UBLOXCFG_KEYVAL_t initialiser

    \b Example
    \code{.c}
        const UBLOXCFG_KEYVAL_t keyVal[] =
        {
            UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_INIFIX3D, true ),   // Set CFG-NAVSPG-INIFIX3D to true
            UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_WKNROLLOVER, 2099 ) // Set CFG-NAVSPG-WKNROLLOVER to 2099
        };
    \endcode
*/
#define UBLOXCFG_KEYVAL_ANY(name, value) { .id = UBLOXCFG_ ## name ## _ID, .val = { .UBLOXCFG_ ## name ## _TYPE = (value) } }

//! Initialiser for a #UBLOXCFG_KEYVAL_t for enum types \hideinitializer
/*!
    \param[in]  name   The name of the configuration item (with underscores instead of minus, e.g. CFG_NAVSPG_FIXMODE)
    \param[in]  value  The name of the enum value (e.g. 2DONLY)

    \returns The #UBLOXCFG_KEYVAL_t initialiser

    \b Example
    \code{.c}
        const UBLOXCFG_KEYVAL_t keyVal[] =
        {
            UBLOXCFG_KEYVAL_ENU( CFG_NAVSPG_FIXMODE, AUTO ), // Set CFG-NAVSPG-FIXMODE to AUTO (= 3)
            //UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_FIXMODE, 3 )   // Set CFG-NAVSPG-FIXMODE to 3 (= AUTO)
        };
    \endcode
*/
#define UBLOXCFG_KEYVAL_ENU(name, value) { .id = UBLOXCFG_ ## name ## _ID, .val = { .UBLOXCFG_ ## name ## _TYPE = (UBLOXCFG_ ## name ## _ ## value) } }

//! Initialiser for #UBLOXCFG_KEYVAL_t for output message rate \hideinitializer
/*!
    \param[in]  msg    The name of the message (with underscores instead of minus, e.g. UBX_NAV_PVT)
    \param[in]  port   The port (UART1, UART2, SPI, I2C or USB)
    \param[in]  rate   The message rate (0..255)

    \returns The #UBLOXCFG_KEYVAL_t initialiser
    
    \b Example
    \code{.c}
        const UBLOXCFG_KEYVAL_t keyVal[] =
        {
            // Set UBX-NAV-PVT output rate to 1 on port UART1
            UBLOXCFG_KEYVAL_MSG( UBX_NAV_PVT, UART1, 1),
            // Set UBX-MON-COMMS output rate to 5 on port USB
            UBLOXCFG_KEYVAL_MSG( UBX_MON_COMMS, USB, 5)
            // Disable NMEA-STANDARD-RMC message on port UART2
            UBLOXCFG_KEYVAL_MSG( NMEA_STANDARD_RMC, UART2, 0 )
        };
    \endcode
*/
#define UBLOXCFG_KEYVAL_MSG(msg, port, rate)  UBLOXCFG_KEYVAL_ANY(msg ## _ ## port, rate)

//! Configuration data from key-value list
/*!
    Generates binary configuration data (e.g. for UBX-CFG-VALSET message) from a list of key-value pairs.

    \param[out] data      Buffer to write the binary configuration data to
    \param[in]  size      Buffer size (maximum size that can be used)
    \param[in]  keyVal    List of key-value pairs
    \param[in]  nKeyVal   Number of key-value pairs
    \param[out] dataSize  Actually used size in data buffer

    \returns true if \c data buffer was big enough to fit all key-value pairs, false otherwise
             (in which case neither \c data nor \c dataSize are valid)

    An empty list (\c nKeyVal = 0) is a valid input.

    \note This function does not validate the contents of the passed list of key-value pairs.

    \b Example
    \code{.c}
        const UBLOXCFG_KEYVAL_t keyVal[] =
        {
            UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_INIFIX3D,     true ),
            UBLOXCFG_KEYVAL_ANY( CFG_NAVSPG_WKNROLLOVER,  2099 ),
            UBLOXCFG_KEYVAL_ENU( CFG_NAVSPG_FIXMODE,      AUTO ),
            UBLOXCFG_KEYVAL_MSG( UBX_NAV_PVT,   UART1,    1),
            UBLOXCFG_KEYVAL_MSG( UBX_MON_COMMS, UART1,    5)
        };
        const int numKeyVal = sizeof(keyVal) / sizeof(*keyVal); // 5
        uint8_t data[100];
        int size;
        const bool res = ubloxcfg_makeData(data, sizeof(data), keyVal, numKeyVal, &size);
        if (res)
        {
            // size   = 26
            // data[] = { 0x13, 0x00, 0x11, 0x10,   0x01,
            //            0x17, 0x00, 0x11, 0x30,   0x33, 0x08,
            //            0x11, 0x00, 0x11, 0x20,   0x03, 
            //            0x07, 0x00, 0x91, 0x20,   0x01,
            //            0x50, 0x03, 0x91, 0x20,   0x05 }
        }
    \endcode
*/
bool ubloxcfg_makeData(uint8_t *data, const int size, const UBLOXCFG_KEYVAL_t *keyVal, const int nKeyVal, int *dataSize);

//! Key-value list from configuration data
/*!
    Parses binary configuration data (e.g. from a UBX-CFG-VALGET response) into a list of key-value pairs.

    \param[in]  data       Buffer to read the binary configuration data from
    \param[in]  size       Size of data buffer
    \param[out] keyVal     List of key-value pairs to populate
    \param[in]  maxKeyVal  Maximum number of key-value pairs (length of key-value list)
    \param[out] nKeyVal    Number of key-value pairs written to list

    \returns true if \c keyVal was big enough to fit all key-value pairs, false otherwise
             (in which case neither \c keyVal nor \c nKeyVal are valid)

    An empty data buffer (\c dataSize = 0) is a valid input.

    \note This function does not validate the extracted keys.

    \b Example
    \code{.c}
        const uint8_t data[26] =
        {
            0x13, 0x00, 0x11, 0x10,   0x01,       // CFG-NAVSPG-INIFIX3D            = 1 (true)
            0x17, 0x00, 0x11, 0x30,   0x33, 0x08, // CFG-NAVSPG-WKNROLLOVER         = 2099 (0x0833)
            0x11, 0x00, 0x11, 0x20,   0x03,       // CFG-NAVSPG-FIXMODE             = 3 (AUTO)
            0x07, 0x00, 0x91, 0x20,   0x01,       // CFG-MSGOUT-UBX_NAV_PVT_UART1   = 1
            0x50, 0x03, 0x91, 0x20,   0x05        // CFG-MSGOUT-UBX_MON_COMMS_UART1 = 1
        };
        int numKeyVal;
        UBLOXCFG_KEYVAL_t keyVal[20];
        const bool res = ubloxcfg_parseData(data, sizeof(data), keyVal, sizeof(keyVal)/sizeof(*keyVal), &numKeyVal);
        if (res)
        {
            // numKeyVal = 5
            // keyVal[0].id = UBLOXCFG_CFG_NAVSPG_INIFIX3D_ID,            keyVal[0].val.L  = true
            // keyVal[1].id = UBLOXCFG_CFG_NAVSPG_WKNROLLOVER_ID,         keyVal[1].val.U2 = 2099
            // keyVal[2].id = UBLOXCFG_CFG_NAVSPG_FIXMODE_ID,             keyVal[2].val.E1 = 3 (= AUTO)
            // keyVal[3].id = UBLOXCFG_CFG_MSGOUT_UBX_NAV_PVT_UART1_ID,   keyVal[3].val.U1 = 1
            // keyVal[4].id = UBLOXCFG_CFG_MSGOUT_UBX_MON_COMMS_UART1_ID, keyVal[4].val.U1 = 1
        }
    \endcode

*/
bool ubloxcfg_parseData(const uint8_t *data, const int size, UBLOXCFG_KEYVAL_t *keyVal, const int maxKeyVal, int *nKeyVal);

//! Stringify item type
/*!
    \param[in] type  Type

    \returns Returns a string with the item type ("L", "U1", etc.) resp. NULL if the \c type is none of #UBLOXCFG_TYPE_e.
*/
const char *ubloxcfg_typeStr(UBLOXCFG_TYPE_t type);

//! Stringify item value
/*!
    \param[out] str   String to format with the value
    \param[in]  size  Size of the \c str buffer
    \param[in]  type  Type
    \param[in]  item  Item (or NULL)
    \param[in]  val   Configuration item value

    \returns Returns true if the value was successfully formatted, false otherwise
             (\c size too small, ...)

    Constant names will be added for L type, and, where available, for X and E type if \c item is given.

    This function does not apply item scale factors nor does it add item units to the
    formatted string.

    The stringification is as follows:
    - L type: "0 (false)" or "1 (true)"
    - U types: "0", "42", "8190232132"
    - I types: "0", "-42", "3423443"
    - X types (FIRST=0x01, LAST=0x80): "0x81 (FIRST|LAST)", "0x7c (n/a)", "0xff (FIRST|LAST|0x7c)", "0x00 (n/a)"
    - E types (ONE=1, TWO=2): "1 (ONE)", "2 (TWO)", "3 (n/a)"
    - R types: "0", "1", "0.5", "1.25e-24"
    - Unknown items all stringify to X type, i.e. "0x04 (n/a)", "0x1234 (n/a)", etc.

    The \c str buffer should be reasonably big, esp. for stringification of E and X types.
*/
bool ubloxcfg_stringifyValue(char *str, const int size, const UBLOXCFG_TYPE_t type, const UBLOXCFG_ITEM_t *item, const UBLOXCFG_VALUE_t *val);

//! Stringify key-value pair
/*!
    \param[out] str     String to format
    \param[in]  size    Size of the \c str buffer
    \param[in]  keyVal  Key-value pair

    \returns Returns true if the value was successfully formatted, false otherwise
             (\c size too small, ...)

    The \c str buffer should be reasonably big (> #UBLOXCFG_MAX_KEYVAL_STR_SIZE),
    esp. for stringification of E and X types.

    \b Example
    \code{.c}
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
        const int numKeyVal = sizeof(keyVal) / sizeof(*keyVal); // 7
        for (int ix = 0; ix < numKeyVal; ix++)
        {
            char str[200];
            const bool res = ubloxcfg_stringifyKeyVal(str, sizeof(str), &keyVal[ix]);
            if (res)
            {
                printf("keyVal[%d]: %s\n", ix, str);
                // keyVal[0]: CFG-NAVSPG-INIFIX3D (0x10110013, L) = 1 (true)
                // keyVal[1]: CFG-NAVSPG-WKNROLLOVER (0x30110017, U2) = 2099
                // keyVal[2]: CFG-NAVSPG-FIXMODE (0x20110011, E1) = 3 (AUTO)
                // keyVal[3]: CFG-NAVSPG-INFIL_MINCNO (0x201100a3, U1) = 30 [dBHz]
                // keyVal[4]: CFG-NAVSPG-OUTFIL_PDOP (0x301100b1, U2) = 20 [0.1]
                // keyVal[5]: CFG-NAVSPG-CONSTR_ALT (0x401100c1, I4) = 234 [0.01m]
                // keyVal[6]: CFG-MSGOUT-UBX_NAV_PVT_UART1 (0x20910007, U1) = 1
                // keyVal[7]: CFG-?-? (0x30fe0ff3, ?2) = 0xbeef
            }
        }
    \endcode
*/
bool ubloxcfg_stringifyKeyVal(char *str, const int size, const UBLOXCFG_KEYVAL_t *keyVal);

//! Maximum size (= length + 1) for key-value stringification \hideinitializer
#define UBLOXCFG_MAX_KEYVAL_STR_SIZE (_UBLOXCFG_MAX_ITEM_LEN +    /* "CFG-LOOOOONG-NAME"      (item name) */ \
                                      20 +                        /* " (0x........, XX) = "   (item Id and type) */ \
                                      21 +                        /* "0x................ ()"  (X8) */ \
                                      _UBLOXCFG_MAX_CONSTS_LEN +  /* "FIRST|LAST|BLA"         (all defined constants) */ \
                                      19 +                        /* "|0x................"    (remaining bits X8) */ \
                                      10)

//! Convert string to value
/*!
    \param[in]  str    String to convert
    \param[in]  type   Type of value
    \param[in]  item   Item (or NULL)
    \param[out] value  Value

    \returns true if string successfully converted, false otherwise

    The following conversions are accepted:
    - The L type can be converted from: \c "true", \c "false" or any decimal, hexadecimal or octal
      string that translate to the value 1 or 0.
    - The U and X types can be converted from decimal, hexadecimal or octal strings.
    - The I and E types can be converted from decimal or hexadecimal strings.
    - When an \c item is given for E and X types, converting constant names (see below) will be attempted first.
    - E type constants are single words, such as \c FOO.
    - X type constants are one or more words or hexadecimal strings separated by a |, e.g.:
      \c FOO, \c FOO|BAR, \c FOO|BAR|0x05, or \c 0x01|0x04|0x20

    \note The input \c str must not contain any characters that are not part of the value
          (for example leading or trailing whitespace).

    \b Examples
    \code{.c}
        UBLOXCFG_VALUE_t value;
        ubloxcfg_valueFromString("true", UBLOXCFG_TYPE_L,  NULL, &value); // value.L = true
        ubloxcfg_valueFromString("0",    UBLOXCFG_TYPE_L,  NULL, &value); // value.L = false
        ubloxcfg_valueFromString("0x2a", UBLOXCFG_TYPE_U1, NULL, &value); // value.U1 = 42
    \endcode
    \code{.c}
        const UBLOXCFG_ITEM_t *item = ubloxcfg_getItemById(UBLOXCFG_CFG_NAVSPG_FIXMODE_ID);
        ubloxcfg_valueFromString("AUTO", item->type, item, &value); // value.E1 = 3
    \endcode
    \code{.c}
        UBLOXCFG_VALUE_t value;
        const UBLOXCFG_ITEM_t *item = ubloxcfg_getItemById(UBLOXCFG_CFG_UBLOXCFGTEST_X1_ID);
        ubloxcfg_valueFromString("FIRST|LAST", item->type, item, &value); // value.X1 = 0x81
    \endcode
*/
bool ubloxcfg_valueFromString(const char *str, const UBLOXCFG_TYPE_t type, const UBLOXCFG_ITEM_t *item, UBLOXCFG_VALUE_t *value);

///@}

/* ********************************************************************************************** */

#ifdef __cplusplus
}
#endif

#endif // __UBLOXCFG_H__
///@}
// eof
