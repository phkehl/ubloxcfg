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

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>

#include "ubloxcfg.h"

/* ****************************************************************************************************************** */

#if (defined(__STDC__) && (__STDC_VERSION__ < 199901L))
#  error This needs C99 or later!
#endif

const UBLOXCFG_ITEM_t *ubloxcfg_getItemByName(const char *name)
{
    if ( (name == NULL) || (strlen(name) < 2) )
    {
        return NULL;
    }
    const UBLOXCFG_ITEM_t *item = NULL;
    // Find by hex string of the ID
    if ( (name[0] == '0') && (name[1] == 'x') )
    {
        uint32_t id = 0;
        int numChar = 0;
        if ( (sscanf(name, "%"SCNx32"%n", &id, &numChar) == 1) && (numChar == (int)strlen(name)) )
        {
            item = ubloxcfg_getItemById(id);
        }
    }
    // Find by name
    else
    {
        const UBLOXCFG_ITEM_t **allItems = (const UBLOXCFG_ITEM_t **)_ubloxcfg_allItems();
        for (int ix = 0; ix < _UBLOXCFG_NUM_ITEMS; ix++)
        {
            if (strcmp(allItems[ix]->name, name) == 0)
            {
                item = allItems[ix];
                break;
            }
        }
    }
    return item;
}

const UBLOXCFG_ITEM_t *ubloxcfg_getItemById(const uint32_t id)
{
    const UBLOXCFG_ITEM_t *item = NULL;
    const UBLOXCFG_ITEM_t **allItems = (const UBLOXCFG_ITEM_t **)_ubloxcfg_allItems();
    for (int ix = 0; ix < _UBLOXCFG_NUM_ITEMS; ix++)
    {
        if (allItems[ix]->id == id)
        {
            item = allItems[ix];
            break;
        }
    }
    return item;
}

const UBLOXCFG_ITEM_t **ubloxcfg_getAllItems(int *num)
{
    *num = _UBLOXCFG_NUM_ITEMS;
    return (const UBLOXCFG_ITEM_t **)_ubloxcfg_allItems();
}

const UBLOXCFG_MSGRATE_t *ubloxcfg_getMsgRateCfg(const char *msgName)
{
    if (msgName == NULL)
    {
        return NULL;
    }
    const UBLOXCFG_MSGRATE_t *rates = NULL;
    const UBLOXCFG_MSGRATE_t **allRates = (const UBLOXCFG_MSGRATE_t **)_ubloxcfg_allRates();
    for (int ix = 0; ix < _UBLOXCFG_NUM_RATES; ix++)
    {
        if (strcmp(allRates[ix]->msgName, msgName) == 0)
        {
            rates = allRates[ix];
            break;
        }
    }
    return rates;
}

const UBLOXCFG_MSGRATE_t **ubloxcfg_getAllMsgRateCfgs(int *num)
{
    *num = _UBLOXCFG_NUM_RATES;
    return (const UBLOXCFG_MSGRATE_t **)_ubloxcfg_allRates();
}

bool ubloxcfg_makeData(uint8_t *data, const int size, const UBLOXCFG_KEYVAL_t *keyVal, const int nKeyVal, int *dataSize)
{
    if ( (data == NULL) || (size <= 0) || (keyVal == NULL) || (nKeyVal < 1) || (dataSize == NULL) )
    {
        return false;
    }

    bool res = true;
    int dataIx = 0;
    memset(data, 0, size);

    for (int kvIx = 0; res && (kvIx < nKeyVal); kvIx++)
    {
        // enough space for key?
        if ( (size - dataIx) < 4 )
        {
            res = false;
            break;
        }

        // encode key ID
        const UBLOXCFG_KEYVAL_t *kv = &keyVal[kvIx];
        const uint32_t key = kv->id;
        data[dataIx++] = (key >>  0) & 0xff;
        data[dataIx++] = (key >>  8) & 0xff;
        data[dataIx++] = (key >> 16) & 0xff;
        data[dataIx++] = (key >> 24) & 0xff;

        // encode value, and also check that there's enough space left in data
        const UBLOXCFG_SIZE_t valSize = UBLOXCFG_ID2SIZE(kv->id);
        const UBLOXCFG_VALUE_t val = kv->val;
        switch (valSize)
        {
            case UBLOXCFG_SIZE_BIT:
                if ( (size - dataIx) < 1 )
                {
                    res = false;
                    break;
                }
                data[dataIx++] = val._bytes[0];
                break;
            case UBLOXCFG_SIZE_ONE:
                if ( (size - dataIx) < 1 )
                {
                    res = false;
                    break;
                }
                data[dataIx++] = val._bytes[0];
                break;
            case UBLOXCFG_SIZE_TWO:
                if ( (size - dataIx) < 2 )
                {
                    res = false;
                    break;
                }
                data[dataIx++] = val._bytes[0];
                data[dataIx++] = val._bytes[1];
                break;
            case UBLOXCFG_SIZE_FOUR:
                if ( (size - dataIx) < 4 )
                {
                    res = false;
                    break;
                }
                data[dataIx++] = val._bytes[0];
                data[dataIx++] = val._bytes[1];
                data[dataIx++] = val._bytes[2];
                data[dataIx++] = val._bytes[3];
                break;
            case UBLOXCFG_SIZE_EIGHT:
                if ( (size - dataIx) < 8 )
                {
                    res = false;
                    break;
                }
                data[dataIx++] = val._bytes[0];
                data[dataIx++] = val._bytes[1];
                data[dataIx++] = val._bytes[2];
                data[dataIx++] = val._bytes[3];
                data[dataIx++] = val._bytes[4];
                data[dataIx++] = val._bytes[5];
                data[dataIx++] = val._bytes[6];
                data[dataIx++] = val._bytes[7];
                break;
            default:
                res = false;
                break;
        }
    }
    *dataSize = dataIx;

    return res;
}

bool ubloxcfg_parseData(const uint8_t *data, const int size, UBLOXCFG_KEYVAL_t *keyVal, const int maxKeyVal, int *nKeyVal)
{
    if ( (data == NULL) || (size <= 0) || (keyVal == NULL) || (maxKeyVal < 1) || (nKeyVal == NULL) )
    {
        return false;
    }

    bool res = true;
    int kvIx = 0;
    int dataIx = 0;
    memset(keyVal, 0, maxKeyVal * sizeof(*keyVal));

    while (res)
    {
        // next key?
        if ( dataIx > (size - 4) )
        {
            break;
        }
        const uint8_t k0 = data[dataIx++];
        const uint8_t k1 = data[dataIx++];
        const uint8_t k2 = data[dataIx++];
        const uint8_t k3 = data[dataIx++];
        const uint32_t id = k0 | (k1 << 8) | (k2 << 16) | (k3 << 24);
        const UBLOXCFG_SIZE_t valSize = UBLOXCFG_ID2SIZE(id);
        UBLOXCFG_VALUE_t val;
        val.U8 = 0;
        switch (valSize)
        {
            case UBLOXCFG_SIZE_BIT:
                if ( dataIx > (size - 1) )
                {
                    res = false;
                    break;
                }
                else
                {
                    val._bytes[0] = data[dataIx++];
                }
                break;
            case UBLOXCFG_SIZE_ONE:
                if ( dataIx > (size - 1) )
                {
                    res = false;
                    break;
                }
                else
                {
                    val._bytes[0] = data[dataIx++];
                }
                break;
            case UBLOXCFG_SIZE_TWO:
                if ( dataIx > (size - 1) )
                {
                    res = false;
                    break;
                }
                else
                {
                    val._bytes[0] = data[dataIx++];
                    val._bytes[1] = data[dataIx++];
                }
                break;
            case UBLOXCFG_SIZE_FOUR:
                if ( dataIx > (size - 1) )
                {
                    res = false;
                    break;
                }
                else
                {
                    val._bytes[0] = data[dataIx++];
                    val._bytes[1] = data[dataIx++];
                    val._bytes[2] = data[dataIx++];
                    val._bytes[3] = data[dataIx++];
                }
                break;
            case UBLOXCFG_SIZE_EIGHT:
                if ( dataIx > (size - 1) )
                {
                    res = false;
                    break;
                }
                else
                {
                    val._bytes[0] = data[dataIx++];
                    val._bytes[1] = data[dataIx++];
                    val._bytes[2] = data[dataIx++];
                    val._bytes[3] = data[dataIx++];
                    val._bytes[4] = data[dataIx++];
                    val._bytes[5] = data[dataIx++];
                    val._bytes[6] = data[dataIx++];
                    val._bytes[7] = data[dataIx++];
                }
                break;
            default:
                res = false;
                break;
        }

        if (res)
        {
            // enough space in list?
            if (kvIx < maxKeyVal)
            {
                keyVal[kvIx].id = id;
                keyVal[kvIx].val = val;
                kvIx++;
            }
            // output list too short, abort
            else
            {
                res = 0;
            }
        }
    }

    *nKeyVal = kvIx;

    return res;
}

const char *ubloxcfg_typeStr(UBLOXCFG_TYPE_t type)
{
    switch (type)
    {
        case UBLOXCFG_TYPE_U1: return "U1";
        case UBLOXCFG_TYPE_U2: return "U2";
        case UBLOXCFG_TYPE_U4: return "U4";
        case UBLOXCFG_TYPE_U8: return "U8";
        case UBLOXCFG_TYPE_I1: return "I1";
        case UBLOXCFG_TYPE_I2: return "I2";
        case UBLOXCFG_TYPE_I4: return "I4";
        case UBLOXCFG_TYPE_I8: return "I8";
        case UBLOXCFG_TYPE_X1: return "X1";
        case UBLOXCFG_TYPE_X2: return "X2";
        case UBLOXCFG_TYPE_X4: return "X4";
        case UBLOXCFG_TYPE_X8: return "X8";
        case UBLOXCFG_TYPE_R4: return "R4";
        case UBLOXCFG_TYPE_R8: return "R8";
        case UBLOXCFG_TYPE_E1: return "E1";
        case UBLOXCFG_TYPE_E2: return "E2";
        case UBLOXCFG_TYPE_E4: return "E4";
        case UBLOXCFG_TYPE_L:  return "L";
    }
    return NULL;
}

bool ubloxcfg_stringifyValue(char *str, const int size, const UBLOXCFG_TYPE_t type, const UBLOXCFG_ITEM_t *item, const UBLOXCFG_VALUE_t *val)
{
    if ( (str == NULL) || (size <= 0) || ((item != NULL) && (item->type != type)) )
    {
        return false;
    }

    str[0] = '\0';
    bool res = false;

    switch (type)
    {
        case UBLOXCFG_TYPE_U1:
            if (size >= 4) // 0..255
            {
                snprintf(str, size, "%" PRIu8, val->U1);
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_U2:
            if (size >= 6) // 0..65535
            {
                snprintf(str, size, "%" PRIu16, val->U2);
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_U4:
            if (size >= 11) // 0..4294967295
            {
                snprintf(str, size, "%" PRIu32, val->U4);
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_U8:
            if (size >= 21) // 0..18446744073709551615
            {
                snprintf(str, size, "%" PRIu64, val->U8);
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_I1:
            if (size >= 5) // -128..127
            {
                snprintf(str, size, "%" PRIi8, val->I1);
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_I2:
            if (size >= 7) // -32768..32767
            {
                snprintf(str, size, "%" PRIi16, val->I2);
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_I4:
            if (size >= 12) // −2147483648..2147483647
            {
                snprintf(str, size, "%" PRIi32, val->I4);
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_I8:
            if (size >= 21) // -9223372036854775808..9223372036854775807
            {
                snprintf(str, size, "%" PRIi64, val->I8);
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_X1:
        case UBLOXCFG_TYPE_X2:
        case UBLOXCFG_TYPE_X4:
        case UBLOXCFG_TYPE_X8:
            if (size >= (19 + 20)) // 0x00 (...) // 0x0000 (...) // 0x00000000 (...) // 0x0000000000000000 (...)
            {
                const char *fmt1 = NULL;
                const char *fmt2 = NULL;
                uint64_t valX;
                switch (type)
                {
                    case UBLOXCFG_TYPE_X1: fmt1 = "0x%02"  PRIx64 " "; fmt2 = "|0x%02"  PRIx64; valX = val->X1; break;
                    case UBLOXCFG_TYPE_X2: fmt1 = "0x%04"  PRIx64 " "; fmt2 = "|0x%04"  PRIx64; valX = val->X2; break;
                    case UBLOXCFG_TYPE_X4: fmt1 = "0x%08"  PRIx64 " "; fmt2 = "|0x%08"  PRIx64; valX = val->X4; break;
                    case UBLOXCFG_TYPE_X8: fmt1 = "0x%016" PRIx64 " "; fmt2 = "|0x%016" PRIx64; valX = val->X8; break;
                    default: break;
                }
                // render hex value
                int len = snprintf(str, size, fmt1, valX);
                const int ixBracket = len;
                // add constant names for known bits
                uint64_t usedBits = 0;
                if (item != NULL)
                {
                    for (int ix = 0; ix < item->nConsts; ix++)
                    {
                        if ((item->consts[ix].val.X & valX) != 0)
                        {
                            len += snprintf(&str[len], size - len, "|%s", item->consts[ix].name);
                            usedBits |= item->consts[ix].val.X;
                            if ((size - len - 1) <= 0)
                            {
                                break;
                            }
                        }
                    }
                }
                // add hex value of remaining bits (for which no constant was defined)
                if ((size - len - 1) > 0)
                {
                    const uint64_t unusedBits = valX & (~usedBits);
                    if (unusedBits == valX)
                    {
                        strncat(&str[len], "|n/a", size - len);
                        len += 4;
                    }
                    else if (unusedBits != 0)
                    {
                        len += snprintf(&str[len], size - len, fmt2, unusedBits);
                    }
                }
                // fix up and terminate string
                str[ixBracket] = '('; // "|" --> "("
                if ((size - len - 1) > 0)
                {
                    str[len++] = ')';
                    str[len] = '\0';
                    res = true;
                }
            }
            break;
        case UBLOXCFG_TYPE_E1:
        case UBLOXCFG_TYPE_E2:
        case UBLOXCFG_TYPE_E4:
            if (size > (15 + 30))
            {
                int32_t valE;
                switch (type)
                {
                    case UBLOXCFG_TYPE_E1: valE = val->E1; break;
                    case UBLOXCFG_TYPE_E2: valE = val->E2; break;
                    case UBLOXCFG_TYPE_E4: valE = val->E4; break;
                    default: break;
                }
                if (item != NULL)
                {
                    for (int ix = 0; ix < item->nConsts; ix++)
                    {
                        if ((int8_t)item->consts[ix].val.E == valE)
                        {
                            snprintf(str, size, "%s (%s)", item->consts[ix].value, item->consts[ix].name);
                            res = true;
                            break;
                        }
                    }
                }
                if (!res)
                {
                    snprintf(str, size, "%" PRIi8 " (n/a)", valE);
                    res = true;
                }
            }
            break;
        case UBLOXCFG_TYPE_R4:
            if (size >= 30) // -1.17549435082228750796874e-38..3.40282346638528859811704e+38
            {
                snprintf(str, size, "%.24g", val->R4);
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_R8:
            if (size >= 61) //-2.22507385850720138309023271733240406421921598046233183e-308..1.79769313486231570814527423731704356798070567525844997e+308
            {
                snprintf(str, size, "%.54g", val->R8);
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_L:
            if (size >= 10)
            {
                snprintf(str, size, val->L ? "1 (true)" : "0 (false)");
                res = true;
            }
            break;
    }

    return res;
}

bool ubloxcfg_splitValueStr(char *str, char **valueStr, char **prettyStr)
{
    if ( (str == NULL) || (valueStr == NULL) || (prettyStr == NULL) )
    {
        return false;
    }
    *valueStr = str;
    *prettyStr = NULL;
    char *space = strchr(str, ' ');
    if (space != NULL)
    {
        *prettyStr = &space[2];
        char *bracket = strchr(space, ')');
        *space = '\0';
        if (bracket != NULL)
        {
            bracket[0] = '\0';
        }
        if (strcmp(*prettyStr, "n/a") == 0)
        {
            *prettyStr = NULL;
        }
    }
    return true;
}

bool ubloxcfg_stringifyKeyVal(char *str, const int size, const UBLOXCFG_KEYVAL_t *keyVal)
{
    if ( (str == NULL) || (size <= 0) || (keyVal == NULL) )
    {
        return false;
    }

    const UBLOXCFG_ITEM_t *item = ubloxcfg_getItemById(keyVal->id);
    str[0] = '\0';
    if (item == NULL)
    {
        int len = 0;
        const UBLOXCFG_SIZE_t valSize = UBLOXCFG_ID2SIZE(keyVal->id);
        switch (valSize)
        {
            case UBLOXCFG_SIZE_BIT:
                len = snprintf(str, size, "CFG-?-? (0x%02" PRIx32 ", ?0) = 0x%"PRIx8,
                    keyVal->id, keyVal->val.X1);
                break;
            case UBLOXCFG_SIZE_ONE:
                len = snprintf(str, size, "CFG-?-? (0x%02" PRIx32 ", ?1) = 0x%02"PRIx8,
                    keyVal->id, keyVal->val.X1);
                break;
            case UBLOXCFG_SIZE_TWO:
                len = snprintf(str, size, "CFG-?-? (0x%02" PRIx32 ", ?2) = 0x%04"PRIx16,
                    keyVal->id, keyVal->val.X2);
                break;
            case UBLOXCFG_SIZE_FOUR:
                len = snprintf(str, size, "CFG-?-? (0x%02" PRIx32 ", ?4) = 0x%08"PRIx32,
                    keyVal->id, keyVal->val.X4);
                break;
            case UBLOXCFG_SIZE_EIGHT:
                len = snprintf(str, size, "CFG-?-? (0x%08" PRIx32 ", ?8) = 0x%016"PRIx64,
                    keyVal->id, keyVal->val.X8);
                break;
        }
        return len < size;
    }

    // add "item name (ID, type) = "
    const char *type = ubloxcfg_typeStr(item->type);
    int len = snprintf(str, size, "%s (0x%08" PRIx32 ", %s) = ", item->name, item->id, type != NULL ? type : "?");
    if ((size - 1) <= len)
    {
        str[0] = '\0';
        return false;
    }

    // add stringified value
    if (!ubloxcfg_stringifyValue(&str[len], size - len, item->type, item, &keyVal->val))
    {
        str[0] = '\0';
        return false;
    }

    // was there enough space?
    len = strlen(str);
    if ((size - 1 - 3) <= len)
    {
        str[0] = '\0';
        return false;
    }

    // add scale and/or unit
    if ( (item->scale != NULL) || (item->unit != NULL) )
    {
        str[len++] = ' ';
        str[len++] = '[';
        str[len] = '\0';

        if (item->scale != NULL)
        {
            len += snprintf(&str[len], size - len, "%s", item->scale);
        }
        if ((size - 1 - 2) <= len)
        {
            str[0] = '\0';
            return false;
        }
        if (item->unit != NULL)
        {
            len += snprintf(&str[len], size - len, "%s", item->unit);
        }
        if ((size - 1 - 1) <= len)
        {
            str[0] = '\0';
            return false;
        }
        str[len++] = ']';
        str[len] = '\0';
    }

    return true;
}

static bool strToValUnsigned(const char *str, const UBLOXCFG_TYPE_t type, uint64_t *val);
static bool strToValSigned(const char *str, const UBLOXCFG_TYPE_t type, int64_t *val);
static bool findEnumValue(const char *str, const UBLOXCFG_ITEM_t *item, int64_t *val);
static bool findConstValue(const char *str, const UBLOXCFG_ITEM_t *item, uint64_t *val);

bool ubloxcfg_valueFromString(const char *str, UBLOXCFG_TYPE_t type, const UBLOXCFG_ITEM_t *item, UBLOXCFG_VALUE_t *value)
{
    if ( (str == NULL) || (value == NULL) || ((item != NULL) && (item->type != type)) )
    {
        return false;
    }
    
    bool res = false;

    uint64_t valUnsigned;
    int64_t  valSigned;
    float    valFloat;
    double   valDouble;
    int      numChar;
    const int len = strlen(str);
    switch (type)
    {
        case UBLOXCFG_TYPE_L:
            if (strcmp(str, "false") == 0)
            {
                 value->L = false;
                 res = true;
            }
            else if (strcmp(str, "true") == 0)
            {
                value->L = true;
                res = true;
            }
            else if (strToValUnsigned(str, UBLOXCFG_TYPE_U8, &valUnsigned))
            {
                if (valUnsigned == 0)
                {
                    value->L = false;
                    res = true;
                }
                else if (valUnsigned == 1)
                {
                    value->L = true;
                    res = true;
                }
            }
            break;
        case UBLOXCFG_TYPE_U1:
            if (strToValUnsigned(str, type, &valUnsigned))
            {
                value->U1 = (uint8_t)valUnsigned;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_U2:
            if (strToValUnsigned(str, type, &valUnsigned))
            {
                value->U2 = (uint16_t)valUnsigned;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_U4:
            if (strToValUnsigned(str, type, &valUnsigned))
            {
                value->U4 = (uint32_t)valUnsigned;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_U8:
            if (strToValUnsigned(str, type, &valUnsigned))
            {
                value->U8 = valUnsigned;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_X1:
            if (findConstValue(str, item, &valUnsigned))
            {
                value->X1 = (uint8_t)valUnsigned;
                res = true;
            }
            else if (strToValUnsigned(str, type, &valUnsigned))
            {
                value->U1 = (uint8_t)valUnsigned;
                res = true;
            }
            break; 
        case UBLOXCFG_TYPE_X2:
            if (findConstValue(str, item, &valUnsigned))
            {
                value->X2 = (uint16_t)valUnsigned;
                res = true;
            }
            else if (strToValUnsigned(str, type, &valUnsigned))
            {
                value->X2 = (uint16_t)valUnsigned;
                res = true;
            }
            break; 
        case UBLOXCFG_TYPE_X4:
            if (findConstValue(str, item, &valUnsigned))
            {
                value->X4 = (uint32_t)valUnsigned;
                res = true;
            }
            else if (strToValUnsigned(str, type, &valUnsigned))
            {
                value->X4 = (uint32_t)valUnsigned;
                res = true;
            }
            break; 
        case UBLOXCFG_TYPE_X8:
            if (findConstValue(str, item, &valUnsigned))
            {
                value->X8 = valUnsigned;
                res = true;
            }
            else if (strToValUnsigned(str, type, &valUnsigned))
            {
                value->X8 = valUnsigned;
                res = true;
            }
            break; 
        case UBLOXCFG_TYPE_I1:
            if (strToValSigned(str, type, &valSigned))
            {
                value->I1 = (int8_t)valSigned;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_I2:
            if (strToValSigned(str, type, &valSigned))
            {
                value->I2 = (int16_t)valSigned;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_I4:
            if (strToValSigned(str, type, &valSigned))
            {
                value->I4 = (int32_t)valSigned;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_I8:
            if (strToValSigned(str, type, &valSigned))
            {
                value->I8 = valSigned;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_R4:
            if ( (sscanf(str, "%f%n", &valFloat, &numChar) == 1) && (numChar == len) )
            {
                value->R4 = valFloat;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_R8:
            if ( (sscanf(str, "%lf%n", &valDouble, &numChar) == 1) && (numChar == len) )
            {
                value->R8 = valDouble;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_E1:
            if (findEnumValue(str, item, &valSigned))
            {
                value->E1 = (int8_t)valSigned;
                res = true;
            }
            else if (strToValSigned(str, type, &valSigned))
            {
                value->I1 = (int8_t)valSigned;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_E2:
            if (findEnumValue(str, item, &valSigned))
            {
                value->E2 = (int16_t)valSigned;
                res = true;
            }
            else if (strToValSigned(str, type, &valSigned))
            {
                value->I2 = (int16_t)valSigned;
                res = true;
            }
            break;
        case UBLOXCFG_TYPE_E4:
            if (findEnumValue(str, item, &valSigned))
            {
                value->E4 = (int32_t)valSigned;
                res = true;
            }
            else if (strToValSigned(str, type, &valSigned))
            {
                value->I4= (int32_t)valSigned;
                res = true;
            }
            break;
    }
    return res;
}

static bool strToValUnsigned(const char *str, const UBLOXCFG_TYPE_t type, uint64_t *val)
{
    if ( (str == NULL) || (val == NULL) )
    {
        return false;
    }
    const int len = strlen(str);
    if ( (len < 1) || (isspace((int)str[0]) != 0) )
    {
        return false;
    }

    bool res = false;
    uint64_t max = 0;
    switch (type)
    {
        case UBLOXCFG_TYPE_U1:
        case UBLOXCFG_TYPE_X1:
            max = UINT8_MAX;
            break;
        case UBLOXCFG_TYPE_U2:
        case UBLOXCFG_TYPE_X2:
            max = UINT16_MAX;
            break;
        case UBLOXCFG_TYPE_U4:
        case UBLOXCFG_TYPE_X4:
            max = UINT32_MAX;
            break;
        case UBLOXCFG_TYPE_U8:
        case UBLOXCFG_TYPE_X8:
            max = UINT64_MAX;
            break;
        default: break;
    }
    uint64_t value;
    if (len < 1)
    {
        return false;
    }
    // hex
    else if ( (len > 1) && (str[0] == '0') && (str[1] == 'x') )
    {
        int numChar = 0;
        if ( (len > 2) && (sscanf(str, "%" SCNx64"%n", &value, &numChar) == 1) && (numChar == len) )
        {
            res = true;
        }
    }
    // octal
    else if (str[0] == '0')
    {
        int numChar = 0;
        if ( (sscanf(str, "%" SCNo64"%n", &value, &numChar) == 1) && (numChar == len) )
        {
            res = true;
        }
    }
    // dec
    else
    {
        int numChar = 0;
        if ( (sscanf(str, "%" SCNu64"%n", &value, &numChar) == 1) && (numChar == len) )
        {
            res = true;
        }
    }
    
    if (res)
    {
        *val = value;
    }
    return res && (value <= max);
}


static bool strToValSigned(const char *str, UBLOXCFG_TYPE_t type, int64_t *val)
{
    if ( (str == NULL) || (val == NULL) )
    {
        return false;
    }
    const int len = strlen(str);
    if ( (len < 1) || (isspace((int)str[0]) != 0) )
    {
        return false;
    }

    bool res = false;
    int64_t min = 0;
    int64_t max = 0;
    switch (type)
    {
        case UBLOXCFG_TYPE_I1:
        case UBLOXCFG_TYPE_E1:
            min = INT8_MIN;
            max = INT8_MAX;
            break;
        case UBLOXCFG_TYPE_I2:
        case UBLOXCFG_TYPE_E2:
            min = INT16_MIN;
            max = INT16_MAX;
            break;
        case UBLOXCFG_TYPE_I4:
        case UBLOXCFG_TYPE_E4:
            min = INT32_MIN;
            max = INT32_MAX;
            break;
        case UBLOXCFG_TYPE_I8:
            min = INT64_MIN;
            max = INT64_MAX;
            break;
        default: break;
    }
    int64_t value;
    // hex
    if ( (len > 1) && (str[0] == '0') && (str[1] == 'x') )
    {
        uint64_t valUnsigned;
        int numChar = 0;
        if ( (len > 2) && ( sscanf(str, "%" SCNx64"%n", &valUnsigned, &numChar) == 1) && (numChar ==len) )
        {
            // sign extend to size
            switch (type)
            {
                case UBLOXCFG_TYPE_I1:
                case UBLOXCFG_TYPE_E1:
                    value = (int64_t)(int8_t)valUnsigned;
                    break;
                case UBLOXCFG_TYPE_I2:
                case UBLOXCFG_TYPE_E2:
                    value = (int64_t)(int16_t)valUnsigned;
                    break;
                case UBLOXCFG_TYPE_I4:
                case UBLOXCFG_TYPE_E4:
                    value = (int64_t)(int32_t)valUnsigned;
                    break;
                case UBLOXCFG_TYPE_I8:
                    value = (int64_t)valUnsigned;
                    break;
                default:
                    break;
            }
            res = true;
        }
    }
    // dec
    else
    {
        int numChar = 0;
        if ( (sscanf(str, "%" SCNi64"%n", &value, &numChar) == 1) && (numChar == len) )
        {
            res = true;
        }
    }
    
    if (res)
    {
        *val = value;
    }
    return res && (value >= min) && (value <= max);
}


static bool findConstValue(const char *str, const UBLOXCFG_ITEM_t *item, uint64_t *val)
{
    if ( (item == NULL) || (val == NULL) || (str == NULL) || (strlen(str) < 1) )
    {
        return false;
    }

    bool res = true;
    uint64_t valRes = 0;

    // iterate over parts of the string, separated by '|'
    const char sep = '|';
    const char *pStr = str;
    const char *pSep = strchr(pStr, sep);
    while (*pStr != '\0')
    {
        // number of characters at beginning of pStr to compare
        const int cmpLen = strlen(pStr) - ( (pSep != NULL) ? strlen(pSep) : 0 );

        // interpret (part of) the string, which can be a 0x.. hex number or a constant name
        if ( (cmpLen > 2) && (pStr[0] == '0') && (pStr[1] == 'x') )
        {
            uint64_t v = 0;
            int numChar = 0;
            if ( (cmpLen < 3) || (sscanf(pStr, "%" SCNx64"%n", &v, &numChar) != 1) || (numChar != cmpLen) )
            {
                res = false;
                break; // bad hex, give up
            }
            valRes |= v;
        }
        else
        {
            bool found = false;
            for (int ix = 0; (ix < item->nConsts) && !found; ix++)
            {
                const int nameLen = strlen(item->consts[ix].name);
                if ( (nameLen == cmpLen) && (strncmp(item->consts[ix].name, pStr, cmpLen) == 0) )
                {
                    valRes |= item->consts[ix].val.X;
                    found = true;
                }
            }
            if (!found)
            {
                res = false;
                break; // bad part of string, give up
            }
        }

        // next part or done
        if (pSep != NULL)
        {
            pStr = pSep + 1;
            pSep = strchr(pStr, sep);
        }
        else
        {
            break;
        }
    }

    if (res)
    {
        *val = valRes;
    }
    return res;
}

static bool findEnumValue(const char *str, const UBLOXCFG_ITEM_t *item, int64_t *val)
{
    bool res = false;
    if (item != NULL)
    {
        for (int ix = 0; ix < item->nConsts; ix++)
        {
            if (strcmp(str, item->consts[ix].name) == 0)
            {
                *val = (int64_t)item->consts[ix].val.E;
                res = true;
                break;
            }
        }
    }
    return res;
}

const char *ubloxcfg_layerName(const UBLOXCFG_LAYER_t layer)
{
    switch (layer)
    {
        case UBLOXCFG_LAYER_RAM:      return "RAM";
        case UBLOXCFG_LAYER_BBR:      return "BBR";
        case UBLOXCFG_LAYER_FLASH:    return "Flash";
        case UBLOXCFG_LAYER_DEFAULT:  return "Default";
    }
    return "?";
}

bool ubloxcfg_layerFromName(const char *name, UBLOXCFG_LAYER_t *layer)
{
    if ( (name == NULL) || (name[0] == '\0') )
    {
        return false;
    }
    char str[20];
    int len = strlen(name);
    if (len > ((int)sizeof(str) - 1))
    {
        return false;
    }
    str[len] = '\0';
    while (len > 0)
    {
        len--;
        str[len] = tolower(name[len]);
    }
    if (strcmp(str, "ram") == 0)
    {
        *layer = UBLOXCFG_LAYER_RAM;
    }
    else if (strcmp(str, "bbr") == 0)
    {
        *layer = UBLOXCFG_LAYER_BBR;
    }
    else if (strcmp(str, "flash") == 0)
    {
        *layer = UBLOXCFG_LAYER_FLASH;
    }
    else if (strcmp(str, "default") == 0)
    {
        *layer = UBLOXCFG_LAYER_DEFAULT;
    }
    else
    {
        return false;
    }
    return true;
}


/* ****************************************************************************************************************** */

// eof
