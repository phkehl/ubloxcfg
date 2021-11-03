/* ************************************************************************************************/ // clang-format off
// flipflip's cfggui
//
// Copyright (c) 2021 Philippe Kehl (flipflip at oinkzwurgl dot org),
// https://oinkzwurgl.org/hacking/ubloxcfg
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
// even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program.
// If not, see <https://www.gnu.org/licenses/>.

#include "gui_win_data_config.hpp"

#include "gui_win_data_inc.hpp"

/* ****************************************************************************************************************** */

static const char * const kDbItemsFilterHelp =
    "The filter matches against a string, which contains the comma-separated\n"
    "text from all columns as shown in the Receiver DB tab.\n"
    "\n"
    "Flags in the form 'changed-<layer>' are added for each layer where the\n"
    "value differs from the default.\n"
    "\n"
    "Examples:\n"
    "CFG-RATE-MEAS,0x30210001.U2,0.001,s,1000,,,1000,\n"
    "CFG-MSGOUT-UBX_NAV_PVT_UART1,0x20910008,U1,,,1,1,0,0,changed-RAM changed-Flash\n"
    "\n"
    "The filter also matches message names as shown in the Message rates tab. Matches\n"
    "on message names will also match the corresponding CFG-OUTMSG items in the\n"
    "Receiver DB tab.\n";
GuiWinDataConfig::GuiWinDataConfig(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _dbPollState{POLL_IDLE}, _dbPollDataAvail{false},
    _dbSetState{SET_IDLE}, _dbSetNumValset{0},
    _dbSetRam{false}, _dbSetBbr{true}, _dbSetFlash{true}, _dbSetApply{true},
    _dbItems{}, _protDbitems{}, _baudDbitems{}, _msgrateItems{},
    _dbItemOrder{DB_ORDER_BY_NAME}, _dbShowUnknown{false},
    _chValueRef{REF_DEFAULT}, _dbItemDispCount{},
    _dbFilterWidget{kDbItemsFilterHelp}, _dbFilterUpdate{false},
    _cfgFileName{}, _cfgFileSaveResultTo{-1.0}, _cfgFileSaveError{}
{
    _winSize = { 155, 40 };
    _DbInit();
}

// Must match DbItemLayer_e!
const UBLOXCFG_LAYER_t GuiWinDataConfig::_cfgLayers[NUM_LAYERS] =
{
    UBLOXCFG_LAYER_RAM, UBLOXCFG_LAYER_BBR, UBLOXCFG_LAYER_FLASH, UBLOXCFG_LAYER_DEFAULT
};

const UBLOXCFG_LAYER_t GuiWinDataConfig::_dbPollLayers[NUM_LAYERS] =
{
    UBLOXCFG_LAYER_DEFAULT, UBLOXCFG_LAYER_RAM, UBLOXCFG_LAYER_BBR, UBLOXCFG_LAYER_FLASH
};

const char * const GuiWinDataConfig::_chValueRevertStrs[2] =
{
    "Revert to current value",
    "Revert to default value"
};

const char * const GuiWinDataConfig::_portNames[NUM_PORTS] =
{
    "UART1", "UART2", "SPI", "I2C", "USB"
};

const char * const GuiWinDataConfig::_protNames[NUM_PROTS] =
{
    "UBX", "NMEA", "RTCM3"
};

const uint32_t GuiWinDataConfig::_protItemIds[NUM_INOUT][NUM_PORTS][NUM_PROTS] =
{
    {
        { UBLOXCFG_CFG_UART1INPROT_UBX_ID,    UBLOXCFG_CFG_UART1INPROT_NMEA_ID,    UBLOXCFG_CFG_UART1INPROT_RTCM3X_ID },
        { UBLOXCFG_CFG_UART2INPROT_UBX_ID,    UBLOXCFG_CFG_UART2INPROT_NMEA_ID,    UBLOXCFG_CFG_UART2INPROT_RTCM3X_ID },
        { UBLOXCFG_CFG_SPIINPROT_UBX_ID,      UBLOXCFG_CFG_SPIINPROT_NMEA_ID,      UBLOXCFG_CFG_SPIINPROT_RTCM3X_ID },
        { UBLOXCFG_CFG_I2CINPROT_UBX_ID,      UBLOXCFG_CFG_I2CINPROT_NMEA_ID,      UBLOXCFG_CFG_I2CINPROT_RTCM3X_ID },
        { UBLOXCFG_CFG_USBINPROT_UBX_ID,      UBLOXCFG_CFG_USBINPROT_NMEA_ID,      UBLOXCFG_CFG_USBINPROT_RTCM3X_ID },
    },
    {
        { UBLOXCFG_CFG_UART1OUTPROT_UBX_ID,   UBLOXCFG_CFG_UART1OUTPROT_NMEA_ID,   UBLOXCFG_CFG_UART1OUTPROT_RTCM3X_ID },
        { UBLOXCFG_CFG_UART2OUTPROT_UBX_ID,   UBLOXCFG_CFG_UART2OUTPROT_NMEA_ID,   UBLOXCFG_CFG_UART2OUTPROT_RTCM3X_ID },
        { UBLOXCFG_CFG_SPIOUTPROT_UBX_ID,     UBLOXCFG_CFG_SPIOUTPROT_NMEA_ID,     UBLOXCFG_CFG_SPIOUTPROT_RTCM3X_ID },
        { UBLOXCFG_CFG_I2COUTPROT_UBX_ID,     UBLOXCFG_CFG_I2COUTPROT_NMEA_ID,     UBLOXCFG_CFG_I2COUTPROT_RTCM3X_ID },
        { UBLOXCFG_CFG_USBOUTPROT_UBX_ID,     UBLOXCFG_CFG_USBOUTPROT_NMEA_ID,     UBLOXCFG_CFG_USBOUTPROT_RTCM3X_ID },
    }
};

const uint32_t GuiWinDataConfig::_baudItemIds[NUM_PORTS] =
{
    UBLOXCFG_CFG_UART1_BAUDRATE_ID, UBLOXCFG_CFG_UART2_BAUDRATE_ID, 0, 0, 0
};

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::Loop(const uint32_t &frame, const double &now)
{
    (void)frame;
    (void)now;
    switch (_dbPollState)
    {
        case POLL_INIT:
            _dbPollLayerIx = 0;
            _dbPollProgressMax = 50;
            _dbPollProgress = 0;
            _dbPollState = POLL_POLL;
            _DbClear();
            break;
        case POLL_POLL:
        {
            _dbPollState = POLL_WAIT;
            _dbPollTime = ImGui::GetTime();
            std::vector<uint32_t> keys = { UBX_CFG_VALGET_V0_ALL_WILDCARD };
            _receiver->GetConfig(_dbPollLayers[_dbPollLayerIx], keys, _winUid);
            break;
        }
        case POLL_WAIT:
            // handled in ProcessData()
            break;
        case POLL_DONE:
            for (auto &dbitem: _dbItems)
            {
                dbitem.ChSet(IX_RAM, _chValueRef == REF_CURRENT ? IX_RAM : IX_DEFAULT);
            }
            _DbSync();
            _dbPollDataAvail = true;
            _dbPollState = POLL_IDLE;
            break;
        case POLL_IDLE:
            break;
        case POLL_FAIL:
            _dbPollState = POLL_IDLE;
            _DbClear();
            break;
    }

    // Initiate setting (storing) config
    switch (_dbSetState)
    {
        case SET_IDLE:
            break;
        case SET_START:
            if (_receiver->IsReady() && !_cfgChangedKv.empty())
            {
                _receiver->SetConfig(_dbSetRam, _dbSetBbr, _dbSetFlash, _dbSetApply, _cfgChangedKv, _winUid);
                // Work out how many UBX-CFG-VALSET will be needed
                int nMsgs = 0;
                UBX_CFG_VALSET_MSG_t *msgs = ubxKeyValToUbxCfgValset(_cfgChangedKv.data(), (int)_cfgChangedKv.size(), false, true, true, &nMsgs);
                std::free(msgs);
                _dbSetTotValset = nMsgs + 1;
                _dbSetNumValset = 0;
                _dbSetState = SET_WAIT;
            }
            else
            {
                _dbSetState = SET_IDLE;
            }
            break;
        case SET_WAIT:
            // handled in ProcessData()
            break;
        case SET_DONE:
            _dbSetTotValset = 0;
            _dbSetNumValset = 0;
            _dbSetState = SET_IDLE;
            break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::ProcessData(const Data &data)
{
    if (_dbPollState == POLL_WAIT)
    {
        // Handle timeout
        const float now = ImGui::GetTime();
        if ( (now - _dbPollTime) > 10.0 )
        {
            ERROR("Timeout polling configuration!");
            _dbPollState = POLL_FAIL;
        }

        // Handle response
        else if ( (data.type == Data::Type::DATA_KEYVAL) && (data.uid == _winUid) )
        {
            int layerIx = -1;
            for (int ix = 0; ix < NUM_LAYERS; ix++)
            {
                if (_cfgLayers[ix] == data.keyval->layer)
                {
                    layerIx = ix;
                    break;
                }
            }
            if (layerIx != -1)
            {
                // Process all key-value pairs in the response and store to the database
                for (auto &kv: data.keyval->kv)
                {
                    // Create/add new item
                    auto dbitem = _DbGetItem(kv.id);
                    if (dbitem == _dbItems.end())
                    {
                        auto newDbitem = DbItem(kv.id);
                        _dbItems.push_back(newDbitem);
                        dbitem = _dbItems.end() - 1;
                    }
                    // Store key-value for this layer
                    dbitem->SetValue(kv.val, static_cast<enum DbItemLayer_e>(layerIx));
                }

                _DbSync();
            }

            _dbPollLayerIx++;
            _dbPollProgress = 0;
            if (_dbPollLayerIx < NUM_LAYERS)
            {
                _dbPollState = POLL_POLL;
            }
            else
            {
                _dbPollState = POLL_DONE;
            }
        }

        // Count number of received UBX-CFG-VALGET (assuming they're ours..) to make the progress
        // indicator a bit more interesting
        else if (data.type == Data::Type::DATA_MSG)
        {
            if ( (data.msg->name == "UBX-CFG-VALGET") && (_dbPollProgress < _dbPollProgressMax) )
            {
                _dbPollProgress++;
            }
        }
    }

    if (_dbSetState == SET_WAIT)
    {
        // Count UBX-ACK-ACK that we receive while applying the configuration
        // FIXME: should perhaps check message content as there could be other stuff ongoing..
        if ( (data.type == Data::Type::DATA_MSG) && (data.msg->name == "UBX-ACK-ACK") )
        {
            _dbSetNumValset++;
        }
        else if ( (data.type == Data::Type::ACK) && (data.uid == _winUid) )
        {
            _dbSetState = SET_DONE;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataConfig::DbItem::DbItem(const uint32_t _id) :
    id{_id}, unitStr{}, scaleStr{}, values{}, valueChanged{false}, valueValid{false},
    filterMatch{true}, filterString{}, specialItem{false},
    chValue{}, chReference{}, chValueStr{}, chPrettyStr{}, chDirty{}
{
    defitem = ubloxcfg_getItemById(id);
    // Known items
    if (defitem != NULL)
    {
        name    = defitem->name;
        title   = defitem->title;
        type    = defitem->type;
        size    = defitem->size;
        if (defitem->unit != NULL)
        {
            unitStr = defitem->unit;
        }
        if (defitem->scale != NULL)
        {
            scaleStr = defitem->scale;
        }
    }
    // Unknown items
    else
    {
        defitem = nullptr;
        name    = Ff::Sprintf("0x%08x", id);
        title   = "Unknown (undocumented)";
        size    = UBLOXCFG_ID2SIZE(id); // 0...7
        static const UBLOXCFG_TYPE_t types[] = {
            UBLOXCFG_TYPE_X8 /* 0 = invalid */,
            UBLOXCFG_TYPE_L, UBLOXCFG_TYPE_X1, UBLOXCFG_TYPE_X2, UBLOXCFG_TYPE_X4, UBLOXCFG_TYPE_X8, /* 1...5 = valid */
            UBLOXCFG_TYPE_X8, UBLOXCFG_TYPE_X8  /* 6,7 = invalid */ };
        type    = types[size];
    }

    idStr   = Ff::Sprintf("0x%08x", id);
    typeStr = ubloxcfg_typeStr(type);
    _SyncValues();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::DbItem::SetValue(const UBLOXCFG_VALUE_t &val, const enum DbItemLayer_e layerIx)
{
    if ( (layerIx >= IX_RAM) && (layerIx < NUM_LAYERS) )
    {
        char str[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
        if (!ubloxcfg_stringifyValue(str, sizeof(str), type, defitem, &val))
        {
            std::strcpy(str, "wtf?!");
        }
        values[layerIx].val   = val;
        values[layerIx].str   = str;
        values[layerIx].valid = true;
        _SyncValues();
        ChSync();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::DbItem::_SyncValues()
{
    std::string sep = ",";
    std::string changedStr = "";
    filterString = name + sep + idStr + sep + typeStr + sep + scaleStr + sep + unitStr;
    bool changed = false;
    bool valid = false;
    if (values[IX_DEFAULT].valid)
    {
        for (int ix = 0; ix < NUM_LAYERS; ix++)
        {
            if (values[ix].valid)
            {
                values[ix].valid = true;
                valid = true;
                values[ix].changed = values[IX_DEFAULT].val._raw != values[ix].val._raw;
                if (values[ix].changed)
                {
                    changed = true;
                    changedStr += std::string(" changed-") + std::string(ubloxcfg_layerName(_cfgLayers[ix]));
                }
            }
        }
    }
    valueChanged = changed;
    valueValid   = valid;
    for (int ix = 0; ix < NUM_LAYERS; ix++)
    {
        filterString += sep;
        if (values[ix].valid)
        {
            filterString += values[ix].str;
        }
    }
    filterString += sep;
    if (changed)
    {
        filterString += changedStr.substr(1);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::DbItem::ClearValues()
{
    for (int ix = 0; ix < NUM_LAYERS; ix++)
    {
        values[ix].changed  = false;
        values[ix].val._raw = 0;
        values[ix].valid    = false;
        values[ix].str.clear();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::DbItem::ChSync()
{
    chDirty = chValue._raw != chReference._raw;

    char str[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
    if (ubloxcfg_stringifyValue(str, sizeof(str), type, defitem, &chValue))
    {
        chValueStr = std::string{ str };
        // Prefer pretty value
        char *valueStr;
        char *prettyStr;
        if (ubloxcfg_splitValueStr(str, &valueStr, &prettyStr) && (prettyStr != NULL))
        {
            chPrettyStr = std::string{ prettyStr };
        }
        else
        {
            chPrettyStr = std::string{ valueStr };
        }
    }
    else
    {
        chValueStr = std::string { "WTF?!" };
        chPrettyStr = chValueStr;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::DbItem::ChSet(const int valueLayerIx, const int referenceLayerIx)
{
    if (valueLayerIx >= 0)
    {
        chValue._raw = values[valueLayerIx].valid ? values[valueLayerIx].val._raw : 0;
    }
    if (referenceLayerIx >= 0)
    {
        chReference._raw = values[referenceLayerIx].valid ? values[referenceLayerIx].val._raw : 0;
    }
    ChSync();
}

// ---------------------------------------------------------------------------------------------------------------------

std::vector<GuiWinDataConfig::DbItem>::iterator GuiWinDataConfig::_DbGetItem(const std::string &name)
{
    return std::find_if(_dbItems.begin(), _dbItems.end(),
        [&name](const DbItem &it) { return it.name == name; });
}

std::vector<GuiWinDataConfig::DbItem>::iterator GuiWinDataConfig::_DbGetItem(const uint32_t id)
{
    return std::find_if(_dbItems.begin(), _dbItems.end(),
        [&id](const DbItem &it) { return it.id == id; });
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::_DbInit()
{
    _dbItems.clear();

    // Populate DB with all known (documented) items
    int numItems = 0;
    const UBLOXCFG_ITEM_t **defitems = ubloxcfg_getAllItems(&numItems);
    for (int ix = 0; ix < numItems; ix++)
    {
        const UBLOXCFG_ITEM_t *defitem = defitems[ix];
        auto dbitem = DbItem(defitem->id);
        _dbItems.push_back(dbitem);
    }

    // Sort and update special items
    _DbSync();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::_DbSync()
{
    // Sort
    switch (_dbItemOrder)
    {
        case DB_ORDER_BY_NAME:
            std::sort(_dbItems.begin(), _dbItems.end(), [](const DbItem &a, const DbItem &b)
            {
                const bool aIs0x = a.name.substr(0, 2) == "0x";
                const bool bIs0x = b.name.substr(0, 2) == "0x";
                if (aIs0x == bIs0x)
                {
                    return a.name.substr(3) < b.name.substr(3); // Ignore size (0x?.......)
                }
                else
                {
                    return aIs0x ? false : true;
                }
            });
            break;
        case DB_ORDER_BY_ID_IGN_SIZE:
            std::sort(_dbItems.begin(), _dbItems.end(), [](const DbItem &a, const DbItem &b)
            {
                return (a.id & 0x0fffffff) < (b.id & 0x0fffffff);
            });
            break;
        case DB_ORDER_BY_ID:
            std::sort(_dbItems.begin(), _dbItems.end(), [](const DbItem &a, const DbItem &b)
            {
                return a.id < b.id;
            });
            break;
    }

    // Mark special items used for the port config
    for (int port = 0; port < NUM_PORTS; port++)
    {
        for (int prot = 0; prot < NUM_PROTS; prot++)
        {
            for (int inout = 0; inout < NUM_INOUT; inout++)
            {

                auto dbitem = _DbGetItem(_protItemIds[inout][port][prot]);
                if (dbitem != _dbItems.end())
                {
                    dbitem->specialItem = true;
                    _protDbitems[inout][port][prot] = &(*dbitem);
                }
                else
                {
                    _protDbitems[inout][port][prot] = nullptr;
                }
            }
        }
        if (_baudItemIds[port] != 0)
        {
            auto dbitem = _DbGetItem(_baudItemIds[port]);
            if (dbitem != _dbItems.end())
            {
                dbitem->specialItem = true;
                _baudDbitems[port] = &(*dbitem);
            }
            else
            {
                dbitem->specialItem = false;
                _baudDbitems[port] = nullptr;
            }
        }
    }

    // Mark special items used for the output message rate config
    _msgrateItems.clear();
    int nMsgRates = 0;
    const UBLOXCFG_MSGRATE_t **msgRates = ubloxcfg_getAllMsgRateCfgs(&nMsgRates);
    for (int ix = 0; ix < nMsgRates; ix++)
    {
        auto msgrate = msgRates[ix];
        std::vector<const UBLOXCFG_ITEM_t *> defitems = {
            msgrate->itemUart1, msgrate->itemUart2, msgrate->itemSpi, msgrate->itemI2c, msgrate->itemUsb
        };
        std::string msgName = msgRates[ix]->msgName;
        for (int port = 0; port < NUM_PORTS; port++)
        {
            if (defitems[port]->id != 0)
            {
                auto dbitem = _DbGetItem(defitems[port]->id);
                if (dbitem != _dbItems.end())
                {
                    dbitem->specialItem = true;
                    _msgrateItems[msgName].items[port] = &(*dbitem);
                }
                else
                {
                    dbitem->specialItem = true;
                    _msgrateItems[msgName].items[port] = nullptr;
                }
            }
        }
    }

    // Trigger recalculating filter
    _dbFilterUpdate = true;

    _UpdateChanges();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::_DbClear()
{
    for (auto &dbitem: _dbItems)
    {
        dbitem.ClearValues();
    }
    _dbPollDataAvail = false;
    _DbSync();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::ClearData()
{
    _DbClear();
    for (auto &dbitem: _dbItems)
    {
        dbitem.ChSet(IX_RAM, _chValueRef == REF_CURRENT ? IX_RAM : IX_DEFAULT);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::_UpdateChanges()
{
    _cfgChangedKv.clear();
    _cfgChangedStrs.clear();

    // Ports
    for (int port = 0; port < NUM_PORTS; port++)
    {
        bool anyDirty = false;
        bool haveDefault = false;
        std::string comment = "";
        std::string str = Ff::Sprintf("%-10s", _portNames[port]);
        {
            auto dbitem = _baudDbitems[port];
            if (dbitem && dbitem->chDirty)
            {
                UBLOXCFG_KEYVAL_t kv;
                kv.id     = dbitem->id;
                kv.val.U4 = dbitem->chValue.U4;
                _cfgChangedKv.push_back(kv);
                str += Ff::Sprintf(" %6u", dbitem->chValue.U4);
                anyDirty = true;
            }
            else
            {
                str += std::string("      -");
            }
            if (dbitem && dbitem->values[IX_DEFAULT].valid)
            {
                comment += Ff::Sprintf(" %6u", dbitem->values[IX_DEFAULT].val.U4);
                haveDefault = true;
            }
            else
            {
                comment += std::string("      -");
            }
        }
        for (int inout = 0; inout < NUM_INOUT; inout++)
        {
            std::string protStr = "";
            std::string cmtStr = "";
            for (int prot = 0; prot < NUM_PROTS; prot++)
            {
                auto dbitem = _protDbitems[inout][port][prot];
                if (dbitem && dbitem->chDirty)
                {
                    UBLOXCFG_KEYVAL_t kv;
                    kv.id    = dbitem->id;
                    kv.val.L = dbitem->chValue.L;
                    _cfgChangedKv.push_back(kv);
                    protStr += std::string(",");
                    if (!dbitem->chValue.L)
                    {
                        protStr += std::string("!");
                    }
                    protStr += std::string(_protNames[prot]);
                    anyDirty = true;
                }
                if (dbitem && dbitem->values[IX_DEFAULT].valid)
                {
                    cmtStr += std::string(",");
                    if (!dbitem->values[IX_DEFAULT].val.L)
                    {
                        cmtStr += std::string("!");
                    }
                    cmtStr += std::string(_protNames[prot]);
                    haveDefault = true;
                }
            }
            if (protStr.size() > 0)
            {
                str += Ff::Sprintf("  %-19s", protStr.substr(1).c_str());
            }
            else
            {
                str += "  -                  ";
            }
            if (cmtStr.size() > 0)
            {
                comment += Ff::Sprintf("  %-19s", cmtStr.substr(1).c_str());
            }
            else
            {
                comment += "  -                  ";
            }
        }
        if (haveDefault)
        {
            str += std::string(" # default:") + comment;
        }
        if (anyDirty)
        {
            _cfgChangedStrs.push_back(str);
        }
    }

    // Message rates
    for (auto &entry: _msgrateItems)
    {
        std::string str = Ff::Sprintf("%-24s", entry.first.c_str());
        std::string comment = "";
        bool haveDefault = false;
        const auto *items = entry.second.items;
        bool anyDirty = false;
        for (int port = 0; port < NUM_PORTS; port++)
        {
            if (items[port] && items[port]->chDirty)
            {
                UBLOXCFG_KEYVAL_t kv;
                kv.id = items[port]->id;
                kv.val.U1 = items[port]->chValue.U1;
                _cfgChangedKv.push_back(kv);
                str += Ff::Sprintf(" %3u", kv.val.U1);
                anyDirty = true;
            }
            else
            {
                str += std::string("   -");
            }
            if (items[port]->values[IX_DEFAULT].valid)
            {
                comment += Ff::Sprintf(" %3u", items[port]->values[IX_DEFAULT].val.U1);
                haveDefault = true;
            }
            else
            {
                comment += std::string("   -");
            }
        }
        if (anyDirty)
        {
            if (haveDefault)
            {
                str += std::string("                # default:") + comment;
            }
            _cfgChangedStrs.push_back(str);
        }
    }
    // Remaining items
    for (auto &dbitem: _dbItems)
    {
        if (!dbitem.specialItem && dbitem.chDirty)
        {
            UBLOXCFG_KEYVAL_t kv;
            kv.id  = dbitem.id;
            kv.val = dbitem.chValue;
            _cfgChangedKv.push_back(kv);
            std::string commentStr = "";
            if (dbitem.values[IX_DEFAULT].valid)
            {
                commentStr = std::string(" default: ") + dbitem.values[IX_DEFAULT].str;
            }
            commentStr += std::string(" (type ") + dbitem.typeStr;
            if (!dbitem.scaleStr.empty())
            {
                commentStr += std::string(", ") + dbitem.scaleStr;
            }
            if (!dbitem.unitStr.empty())
            {
                commentStr += std::string(" [") + dbitem.unitStr + std::string("]");
            }
            commentStr += std::string(")");
            std::string str = Ff::Sprintf("%-35s %-23s #%s",
                dbitem.name.c_str(), dbitem.chPrettyStr.c_str(), commentStr.c_str());
            _cfgChangedStrs.push_back(str);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    bool somethingChanged = false;

    if (_DrawControls())
    {
        somethingChanged = true;
    }

    ImGui::Separator();

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_FittingPolicyScroll))
    {
        if (ImGui::BeginTabItem("Ports"))
        {
            if (_DrawPorts())
            {
                somethingChanged = true;
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Message rates"))
        {
            if (_DrawMsgRates())
            {
                somethingChanged = true;
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Items"))
        {
            if (_DrawItems())
            {
                somethingChanged = true;
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Changes"))
        {
            _DrawChanges();
            ImGui::EndTabItem();
        }

        // TODO: align next tab (more to the) right?

        if (ImGui::BeginTabItem("Receiver DB"))
        {
            _DrawDb();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    _DrawWindowEnd();

    if (somethingChanged)
    {
        _UpdateChanges();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinDataConfig::_DrawControls()
{
    const bool ctrlEnabled = _receiver->IsReady();
    bool somethingChanged = false;

    // Refresh items
    {
        if (!ctrlEnabled) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_REFRESH "##RefreshItems", _winSettings->iconButtonSize))
        {
            _dbPollState = POLL_INIT;
        }
        Gui::ItemTooltip("Load configuration data from receiver");
        if (!ctrlEnabled) { Gui::EndDisabled(); }
    }

    ImGui::SameLine();

    // Clear items
    {
        const bool dataAvail = _dbPollDataAvail;
        if (!ctrlEnabled || !dataAvail) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_TIMES "##ClearItems", _winSettings->iconButtonSize))
        {
            _DbClear();
        }
        Gui::ItemTooltip("Clear loaded configuration data");
        if (!ctrlEnabled || !dataAvail) { Gui::EndDisabled(); }
    }

    Gui::VerticalSeparator();

    // Options
    {
        // Reference layer
        enum ChValueRef_e ref = _chValueRef;
        switch (_chValueRef)
        {
            case REF_DEFAULT:
                if (ImGui::Button(ICON_FK_ARROW_DOWN "###RefLayer", _winSettings->iconButtonSize))
                {
                    ref = REF_CURRENT;
                }
                Gui::ItemTooltip("Reference is default configuration (Default layer)");
                break;
            case REF_CURRENT:
                if (ImGui::Button(ICON_FK_ARROW_UP "###RefLayer", _winSettings->iconButtonSize))
                {
                    ref = REF_DEFAULT;
                }
                Gui::ItemTooltip("Reference is current configuration (RAM layer)");
                break;
        }
        if (ImGui::BeginPopupContextItem("RefLayerContext"))
        {
            ImGui::RadioButton("Reference is default configuration (Default layer)", (int *)&ref, REF_DEFAULT);
            ImGui::RadioButton("Reference is current configuration (RAM layer)",     (int *)&ref, REF_CURRENT);
            ImGui::EndPopup();
        }
        if (ref != _chValueRef)
        {
            _chValueRef = ref;
            for (auto &dbitem: _dbItems)
            {
                dbitem.ChSet(-1, _chValueRef == REF_CURRENT ? IX_RAM : IX_DEFAULT);
            }
            somethingChanged = true;
        }

        ImGui::SameLine();

        // Sort
        DbItemOrder_e order = _dbItemOrder;
        switch (_dbItemOrder)
        {
            case DB_ORDER_BY_NAME:
                if (ImGui::Button(ICON_FK_SORT_ALPHA_ASC   "###SortOrder", _winSettings->iconButtonSize))
                {
                    order = DB_ORDER_BY_ID_IGN_SIZE;
                    _DbSync();
                }
                Gui::ItemTooltip("Items are ordered by name");
                break;
            case DB_ORDER_BY_ID_IGN_SIZE:
                if (ImGui::Button(ICON_FK_SORT_AMOUNT_ASC  "###SortOrder", _winSettings->iconButtonSize))
                {
                    order = DB_ORDER_BY_ID;
                    _DbSync();
                }
                Gui::ItemTooltip("Items are ordered by ID (ignoring size)");
                break;
            case DB_ORDER_BY_ID:
                if (ImGui::Button(ICON_FK_SORT_NUMERIC_ASC "###SortOrder", _winSettings->iconButtonSize))
                {
                    order = DB_ORDER_BY_NAME;
                    _DbSync();
                }
                Gui::ItemTooltip("Items are ordered by ID");
                break;
        }
        if (ImGui::BeginPopupContextItem("SortContext"))
        {
            ImGui::RadioButton("Sort items by name",             (int *)&order, DB_ORDER_BY_NAME);
            ImGui::RadioButton("Sort items by ID (ignore size)", (int *)&order, DB_ORDER_BY_ID_IGN_SIZE);
            ImGui::RadioButton("Sort items by ID",               (int *)&order, DB_ORDER_BY_ID);
            ImGui::EndPopup();
        }
        if (order != _dbItemOrder)
        {
            _dbItemOrder = order;
            _DbSync();
        }

        ImGui::SameLine();

        // Show unknown (undocumented)
        if (_dbShowUnknown)
        {
            if (ImGui::Button(ICON_FK_DOT_CIRCLE_O "###ShowUnknown", _winSettings->iconButtonSize))
            {
                _dbShowUnknown = false;
            }
            Gui::ItemTooltip("Showing unknown (undocumented) items");
        }
        else
        {
            if (ImGui::Button(ICON_FK_CIRCLE_O "###ShowUnknown", _winSettings->iconButtonSize))
            {
                _dbShowUnknown = true;
            }
            Gui::ItemTooltip("Not showing unknown (undocumented) items");
        }
        if (ImGui::BeginPopupContextItem("ShowUnknownContext"))
        {
            ImGui::Checkbox("Show unknown (undocumented) items", &_dbShowUnknown);
            ImGui::EndPopup();
        }
    }

    Gui::VerticalSeparator();

    // Layers
    {
        const bool disable = (_dbSetState != SET_IDLE);
        if (disable) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_BARS "##Layers", _winSettings->iconButtonSize))
        {
            ImGui::OpenPopup("Layers");
        }
        Gui::ItemTooltip("Configuration layers");
        if (ImGui::BeginPopup("Layers"))
        {
            ImGui::Checkbox("Store to RAM layer",   &_dbSetRam);
            ImGui::Checkbox("Store to BBR layer",   &_dbSetBbr);
            ImGui::Checkbox("Store to Flash layer", &_dbSetFlash);
            ImGui::Separator();
            ImGui::Checkbox("Apply (activate) after store", &_dbSetApply);
            ImGui::EndPopup();
        }
        if (disable) { Gui::EndDisabled(); }
    }

    ImGui::SameLine();

    // Apply configuration
    {
        const bool disable = !ctrlEnabled || _cfgChangedKv.empty() || (_dbSetState != SET_IDLE);
        if (disable) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_THUMBS_UP "##ApplyConfig", _winSettings->iconButtonSize))
        {
            _dbSetState = SET_START;
        }
        Gui::ItemTooltip("Apply configuration changes");
        if (disable) { Gui::EndDisabled(); }
    }

    ImGui::SameLine();

    // Clear configuration
    {
        const bool disable = _cfgChangedKv.empty() || (_dbSetState != SET_IDLE);
        if (disable) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_THUMBS_DOWN "##DiscardConfig", _winSettings->iconButtonSize))
        {
            for (auto &dbitem: _dbItems)
            {
                dbitem.chValue._raw = dbitem.chReference._raw;
                dbitem.ChSync();
            }
            somethingChanged = true;
        }
        Gui::ItemTooltip("Discard configuration changes");
        if (disable) { Gui::EndDisabled(); }
    }

    ImGui::SameLine();

    // Copy config to clipboard
    {
        const bool disable = _cfgChangedKv.empty();
        if (disable) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_FILES_O "##CopyChanges", _winSettings->iconButtonSize))
        {
            ImGui::LogToClipboard();
            for (auto &str: _cfgChangedStrs)
            {
                ImGui::LogText("%s\n", str.c_str());
            }
            ImGui::LogFinish();
        }
        Gui::ItemTooltip("Copy configuration changes to clipboard");
        if (disable) { Gui::EndDisabled(); }
    }

    ImGui::SameLine();

    // Save to file
    {
        const bool disable = _cfgChangedKv.empty();
        if (disable) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_FLOPPY_O "##Save", _winSettings->iconButtonSize))
        {
            ImGui::OpenPopup("SaveToFilePopup");
        }
        Gui::ItemTooltip("Save configuration to file");
        if (disable) { Gui::EndDisabled(); }
    }

    Gui::VerticalSeparator();

    // Clear everything
    {
        if (!ctrlEnabled) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_ERASER "##ClearAll", _winSettings->iconButtonSize))
        {
            ClearData();
        }
        Gui::ItemTooltip("Clear all data");
        if (!ctrlEnabled) { Gui::EndDisabled(); }
    }

    Gui::VerticalSeparator();

    // Filter
    const float width = 0.5f * (ImGui::GetWindowContentRegionWidth() - ImGui::GetCursorPosX());
    if (_dbFilterWidget.DrawWidget(width) || _dbFilterUpdate)
    {
        _dbFilterUpdate = false;
        bool haveMatch = false;
        const bool active =_dbFilterWidget.IsActive();
        for (auto &dbitem: _dbItems)
        {
            dbitem.filterMatch = active ? _dbFilterWidget.Match(dbitem.filterString) : true;
            if (dbitem.filterMatch)
            {
                haveMatch = true;
            }
        }
        for (auto &entry: _msgrateItems)
        {
            const bool msgNameMatch = _dbFilterWidget.Match(entry.first);
            entry.second.filterMatch = msgNameMatch;
            if (msgNameMatch)
            {
                haveMatch = true;
                auto items = entry.second.items;
                for (int port = 0; port < NUM_PORTS; port++)
                {
                    items[port]->filterMatch = true;
                }
            }
        }
        _dbFilterWidget.SetUserMsg(haveMatch ? std::string("Filter matches") : std::string("Filter does not match"));
    }

    Gui::VerticalSeparator();

    // Progress bar for polling...
    if ( (_dbPollState != POLL_IDLE) && (_dbPollLayerIx <= NUM_LAYERS) )
    {
        const float numLayers = NUM_LAYERS;
        const float layer = (float)_dbPollLayerIx + ((float)_dbPollProgress / (float)_dbPollProgressMax);
        const float progress = layer / numLayers;
        char str[100];
        std::snprintf(str, sizeof(str), "%s (%.0f%%)", ubloxcfg_layerName(_dbPollLayers[_dbPollLayerIx]), progress * 1e2f);
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), str);
    }
    // Progress bar for setting config
    else if (_dbSetState == SET_WAIT)
    {
        const float progress = (float)_dbSetNumValset / (float)_dbSetTotValset;
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "Storing configuration");
    }
    // ...or for pending items to be saved
    else
    {
        const float progress = 0.0f;
        char str[100];
        int count = _cfgChangedKv.size();
        std::snprintf(str, sizeof(str), "Config changes: %d", count);
        if (count > 0) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTMAGENTA)); }
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), str);
        if (count > 0) { ImGui::PopStyleColor(); }
    }

    // Handle save to file
    if (ImGui::BeginPopup("SaveToFilePopup"))
    {
        const bool showMessage = (_cfgFileSaveResultTo > 0.0f) && (ImGui::GetTime() < _cfgFileSaveResultTo);
        if (showMessage) { Gui::BeginDisabled(); }
        ImGui::PushItemWidth(400.0f);
        ImGui::InputTextWithHint("##FileName", "File name", &_cfgFileName);
        ImGui::PopItemWidth();
        if (showMessage) { Gui::EndDisabled(); }
        if (_cfgFileSaveResultTo < 0.0f)
        {
            if (_cfgFileName.empty()) { Gui::BeginDisabled(); }
            if (ImGui::Button(ICON_FK_CHECK " Save"))
            {
                _cfgFileSaveError.clear();
                std::ofstream out;
                out.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                try
                {
                    out.open(_cfgFileName, std::ofstream::binary);
                    PRINT("Writing to %s", _cfgFileName.c_str());
                    for (auto &str: _cfgChangedStrs)
                    {
                        out << str;
                        out << "\n";
                    }
                    PRINT("Configuration written to %s", _cfgFileName.c_str());
                }
                catch (...)
                {
                    ERROR("Failed writing %s: %s", _cfgFileName.c_str(), std::strerror(errno));
                    _cfgFileSaveError = Ff::Sprintf("Error saving: %s!", std::strerror(errno));
                }
                _cfgFileSaveResultTo = ImGui::GetTime() + 2.0;
            }
            ImGui::SameLine();
            if (_cfgFileName.empty()) { Gui::EndDisabled(); }
            if (ImGui::Button(ICON_FK_TIMES " Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, _cfgFileSaveError.size() > 0 ? GUI_COLOUR(C_BRIGHTRED) : GUI_COLOUR(C_BRIGHTGREEN));
            ImGui::TextUnformatted(_cfgFileSaveError.size() > 0 ? _cfgFileSaveError.c_str() : "Configuration saved to file");
            ImGui::PopStyleColor();
            if (!showMessage)
            {
                _cfgFileSaveResultTo = -1.0;
            }
        }

        ImGui::EndPopup();
    }

    return somethingChanged;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::_DrawDb()
{
    // TODO: Switch this to the new ImGui table API, once it's available..
    const float charWidth = _winSettings->charSize.x;
    const float wItem  = 36 * charWidth;
    const float wId    = 12 * charWidth;
    const float wType  =  4 * charWidth;
    const float wScale = 14 * charWidth;
    const float wUnit  = 10 * charWidth;
    const float wLayer = 22 * charWidth;

    const float width = wItem + wId + wType + wScale + wUnit + (NUM_LAYERS * wLayer);
    const int nColumns = 5 + NUM_LAYERS;
    const float currWidth = ImGui::GetWindowContentRegionWidth();

    // Child window of fixed width with horizontal scrolling
    ImGui::SetNextWindowContentSize(ImVec2(currWidth > width ? currWidth : width, 0.0f));
    ImGui::BeginChild("##Table_DbItems", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    // Header
    {
        ImGui::BeginChild("##Head", ImVec2(0.0f, ImGui::GetTextLineHeight() * 1.5));
        // Setup columns
        ImGui::Columns(nColumns, "##HeadCols");
        ImGui::SetColumnWidth(0, wItem);
        ImGui::SetColumnWidth(1, wId);
        ImGui::SetColumnWidth(2, wType);
        ImGui::SetColumnWidth(3, wScale);
        ImGui::SetColumnWidth(4, wUnit);
        for (int ix = 0; ix < NUM_LAYERS; ix++)
        {
            ImGui::SetColumnWidth(5 + ix, wLayer);
        }

        // Title
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTCYAN));
        ImGui::TextUnformatted("Item");
        ImGui::NextColumn();
        ImGui::TextUnformatted("ID");
        ImGui::NextColumn();
        ImGui::TextUnformatted("Type");
        ImGui::NextColumn();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::TextUnformatted("Unit");
        ImGui::NextColumn();
        for (int layer = 0; layer < NUM_LAYERS; layer++)
        {
            ImGui::TextUnformatted(ubloxcfg_layerName(_cfgLayers[layer]));
            if (_dbPollDataAvail)
            {
                ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_CYAN));
                ImGui::Text("(%u items)", _dbItemDispCount[layer]);
                ImGui::PopStyleColor();
            }
            ImGui::NextColumn();
        }
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::EndChild(); // Head
    }

    // No items available
    if (_dbItems.size() == 0)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_WHITE));
        ImGui::TextUnformatted("No receiver configuration data available...");
        ImGui::PopStyleColor();
        ImGui::EndChild(); // Table
        return;
    }

    // Body
    {
        ImGui::BeginChild("##Body", ImVec2(0.0f, 0.0f));

        // Setup columns (again...)
        ImGui::Columns(nColumns, "##BodyCols");
        ImGui::SetColumnWidth(0, wItem);
        ImGui::SetColumnWidth(1, wId);
        ImGui::SetColumnWidth(2, wType);
        ImGui::SetColumnWidth(3, wScale);
        ImGui::SetColumnWidth(4, wUnit);
        for (int ix = 0; ix < NUM_LAYERS; ix++)
        {
            ImGui::SetColumnWidth(5 + ix, wLayer);
        }

        // Draw list of items
        const bool highlight = _dbFilterWidget.IsActive() && _dbFilterWidget.IsHighlight();
        const float dimVal = _winSettings->style.Alpha * 0.5f;

        // Displayed items count
        std::memset(&_dbItemDispCount, 0, sizeof(_dbItemDispCount));

        // Draw list of items
        for (auto &dbitem: _dbItems)
        {
            // If receiver data available, skip items not available on receiver
            if (_dbPollDataAvail && !dbitem.valueValid)
            {
                continue;
            }

            const bool dim = highlight && !dbitem.filterMatch;
            if ( (dbitem.filterMatch || dim) && (_dbShowUnknown || dbitem.defitem) )
            {
                if (dim)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dimVal);
                }

                // Item name with tooltip
                if (dbitem.valueChanged) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTGREEN)); }
                ImGui::Selectable(dbitem.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
                if (dbitem.valueChanged) { ImGui::PopStyleColor(); }
                Gui::ItemTooltip(dbitem.title.size() > 0 ? dbitem.title.c_str() : "Unknown item");
                ImGui::NextColumn();

                // ID
                ImGui::TextUnformatted(dbitem.idStr.c_str());
                ImGui::NextColumn();

                // Type
                ImGui::TextUnformatted(dbitem.typeStr.c_str());
                ImGui::NextColumn();

                // Scale
                ImGui::TextUnformatted(dbitem.scaleStr.c_str());
                ImGui::NextColumn();

                // Unit
                ImGui::TextUnformatted(dbitem.unitStr.c_str());
                ImGui::NextColumn();

                // IX_RAM, BBR, Flash, Default
                for (int layer = 0; layer < NUM_LAYERS; layer++)
                {
                    if (dbitem.values[layer].valid)
                    {
                        const bool changed = dbitem.values[layer].changed;
                        if (changed) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTGREEN)); }
                        ImGui::TextWrapped("%s", dbitem.values[layer].str.c_str());
                        if (changed) { ImGui::PopStyleColor(); }
                        _dbItemDispCount[layer]++;
                    }
                    ImGui::NextColumn();
                }

                ImGui::Separator();

                if (dim)
                {
                    ImGui::PopStyleVar();
                }
            }
        }

        ImGui::EndChild(); // Body
    }

    ImGui::EndChild(); // Table
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinDataConfig::_DrawPorts()
{
    const float charWidth = _winSettings->charSize.x;
    const float wPort =  10 * charWidth;
    const float wBaud =  15 * charWidth;
    const float wProt =  42 * charWidth;
    const float width = wPort + wBaud + (2 * wProt);
    const float currWidth = ImGui::GetWindowContentRegionWidth();
    const float itemSpacing = _winSettings->style.ItemInnerSpacing.x;

    // Child window of fixed width with horizontal scrolling
    ImGui::SetNextWindowContentSize(ImVec2(currWidth > width ? currWidth : width, 0.0f));
    ImGui::BeginChild("##Table_Ports", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    // Header
    {
        ImGui::BeginChild("##Head", ImVec2(0.0f, ImGui::GetTextLineHeight() * 1.5));

        ImGui::Columns(4, "##HeadCols");
        ImGui::SetColumnWidth(0, wPort);
        ImGui::SetColumnWidth(1, wBaud);
        ImGui::SetColumnWidth(2, wProt);
        ImGui::SetColumnWidth(3, wProt);

        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTCYAN));
        ImGui::TextUnformatted("Port");
        ImGui::NextColumn();
        ImGui::TextUnformatted("Baudrate");
        ImGui::NextColumn();
        ImGui::TextUnformatted("Input protocols");
        ImGui::NextColumn();
        ImGui::TextUnformatted("Output protocols");
        ImGui::NextColumn();
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::EndChild(); // Head
    }

    static const uint32_t     baudrates[] = { 9600,   19200,   38400,   57600,   115200,   230400,   460800,   921600 };
    static const char * const baudstrs[] = { "9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600" };

    bool somethingChanged = false;

    // Body
    {
        ImGui::BeginChild("##Body", ImVec2(0.0f, 0.0f));

        // Setup columns (again...)
        ImGui::Columns(4, "##BodyCols");
        ImGui::SetColumnWidth(0, wPort);
        ImGui::SetColumnWidth(1, wBaud);
        ImGui::SetColumnWidth(2, wProt);
        ImGui::SetColumnWidth(3, wProt);

        for (int port = 0; port < NUM_PORTS; port++)
        {
            auto baudDbitem = _baudDbitems[port];
            ImGui::PushID(baudDbitem);

            // Port
            ImGui::AlignTextToFramePadding();
            ImGui::Selectable(_portNames[port]);
            if (Gui::ItemTooltipBegin())
            {
                ImGui::TextUnformatted("Layer   Baudrate   Input protocols    Output protocols");
                ImGui::Separator();
                for (int layer = 0; layer <= NUM_LAYERS; layer++)
                {
                    std::string tooltip = (layer < NUM_LAYERS) ?
                        Ff::Sprintf("%-7s ", ubloxcfg_layerName(_cfgLayers[layer])) : std::string("Changes ");
                    if (!baudDbitem)
                    {
                        tooltip += std::string("     -   ");
                    }
                    else if (layer == NUM_LAYERS)
                    {
                        tooltip += (baudDbitem && baudDbitem->chDirty) ? Ff::Sprintf(" %7u ", baudDbitem->chValue.U4) :
                            std::string("       - ");
                    }
                    else if (baudDbitem && baudDbitem->values[layer].valid)
                    {
                        tooltip += Ff::Sprintf(" %7u ", baudDbitem->values[layer].val.U4);
                    }
                    else
                    {
                        tooltip += std::string("       - ");
                    }
                    for (int inout = 0; inout < NUM_INOUT; inout++)
                    {
                        std::string str = "";
                        for (int prot = 0; prot < NUM_PROTS; prot++)
                        {
                            auto item = _protDbitems[inout][port][prot];
                            if (layer == NUM_LAYERS)
                            {
                                if (item && item->chDirty)
                                {
                                    str += std::string(",");
                                    if (!item->chValue.L)
                                    {
                                        str += std::string("!");
                                    }
                                    str += std::string(_protNames[prot]);
                                }
                            }
                            else if (item && item->values[layer].valid)
                            {
                                str += std::string(",");
                                if (!item->values[layer].val.L)
                                {
                                    str += std::string("!");
                                }
                                str += std::string(_protNames[prot]);
                            }
                        }
                        tooltip += Ff::Sprintf("  %-17s", str.size() > 0 ? str.substr(1).c_str() : "-");
                    }
                    if (layer == NUM_LAYERS)
                    {
                        ImGui::Separator();
                    }
                    ImGui::TextUnformatted(tooltip.c_str());
                }
                Gui::ItemTooltipEnd();
            }
            ImGui::NextColumn();

            // Baudrate
            if (baudDbitem)
            {
                // Revert to current/default
                const bool dirty = baudDbitem->chDirty;
                if (dirty) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTMAGENTA)); } else { Gui::BeginDisabled(); };
                if (ImGui::Button(dirty ? "#" : " ", _winSettings->iconButtonSize))
                {
                    baudDbitem->chValue._raw = baudDbitem->chReference._raw;
                    baudDbitem->ChSync();
                    somethingChanged = true;
                }
                if (dirty) { ImGui::PopStyleColor(); } else { Gui::EndDisabled(); };
                Gui::ItemTooltip(_chValueRevertStrs[_chValueRef]);

                ImGui::SameLine(0.0f, itemSpacing);

                int comboIx = -1;
                for (int ix = 0; ix < NUMOF(baudrates); ix++)
                {
                    if (baudDbitem->chValue.U4 == baudrates[ix])
                    {
                        comboIx = ix;
                        break;
                    }
                }

                // Value combo
                if (ImGui::Combo("##Baudrate", &comboIx, baudstrs, NUMOF(baudstrs), NUMOF(baudstrs)))
                {
                    baudDbitem->chValue.U4 = baudrates[comboIx];
                    baudDbitem->ChSync();
                    somethingChanged = true;
                }
            }

            ImGui::NextColumn();

            // Input/output protocols
            for (int inout = 0; inout < NUM_INOUT; inout++)
            {
                float offs = 0.0f;
                for (int prot = 0; prot < NUM_PROTS; prot++)
                {
                    auto dbitem = _protDbitems[inout][port][prot];
                    ImGui::PushID(dbitem);

                    if (prot > 0)
                    {
                        offs += wProt / NUM_PROTS;
                        ImGui::SameLine(offs);
                    }

                    if (dbitem)
                    {
                        // Revert to current/default
                        const bool dirty = dbitem->chDirty;
                        if (dirty) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTMAGENTA)); } else { Gui::BeginDisabled(); };
                        if (ImGui::Button(dirty ? "#" : " ", _winSettings->iconButtonSize))
                        {
                            dbitem->chValue._raw = dbitem->chReference._raw;
                            dbitem->ChSync();
                            somethingChanged = true;
                        }
                        if (dirty) { ImGui::PopStyleColor(); } else { Gui::EndDisabled(); };
                        Gui::ItemTooltip(_chValueRevertStrs[_chValueRef]);

                        ImGui::SameLine(0.0f, itemSpacing);

                        // Checkbox
                        if (ImGui::Checkbox(_protNames[prot], &dbitem->chValue.L))
                        {
                            dbitem->ChSync();
                            somethingChanged = true;
                        }
                    }

                    ImGui::PopID();
                }

                ImGui::NextColumn();
            }

            ImGui::Separator();

            ImGui::PopID();
        }

        ImGui::EndChild(); // Body
    }

    ImGui::EndChild(); // Table

    return somethingChanged;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinDataConfig::_DrawMsgRates()
{
    // Columns:
    // 0              1     2       3       4     5     6
    // Message name | ALL | UART1 | UART2 | SPI | I2C | USB |
    const float charWidth = _winSettings->charSize.x;
    const float wName = 30 * charWidth;;
    const float wRate = 20 * charWidth;

    const float width = wName + ((1 + NUM_PORTS) * wRate);
    const int nColumns = 1 + (1 + NUM_PORTS);
    const float currWidth = ImGui::GetWindowContentRegionWidth();
    const float itemSpacing = _winSettings->style.ItemInnerSpacing.x;

    bool somethingChanged = false;

    // Child window of fixed width with horizontal scrolling
    ImGui::SetNextWindowContentSize(ImVec2(currWidth > width ? currWidth : width, 0.0f));
    ImGui::BeginChild("##Table_MsgRates", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    // Header
    {
        ImGui::BeginChild("##Head", ImVec2(0.0f, ImGui::GetTextLineHeight() * 1.5));

        // Setup columns
        ImGui::Columns(nColumns, "##HeadCols");
        ImGui::SetColumnWidth(0, wName);
        for (int c = 1; c < nColumns; c++)
        {
            ImGui::SetColumnWidth(c, wRate);
        }

        // Titles
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTCYAN));

        ImGui::TextUnformatted("Message");
        ImGui::NextColumn();

        for (int port = -1; port < NUM_PORTS; port++)
        {
            ImGui::PushID(port);

            // Zero all value
            ImGui::TextUnformatted(port < 0 ? "All ports" : _portNames[port]);
            ImGui::SameLine(wRate - (2 * 25.0f) - itemSpacing);
            if (ImGui::SmallButton("0"))
            {
                somethingChanged = true;
                for (auto entry: _msgrateItems)
                {
                    auto dbitems = entry.second.items;
                    if (port < 0)
                    {
                        for (int p = 0; p < NUM_PORTS; p++)
                        {
                            if (dbitems[p])
                            {
                                dbitems[p]->chValue.U1 = 0;
                                dbitems[p]->ChSync();
                            }
                        }
                    }
                    else
                    {
                        if (dbitems[port])
                        {
                            dbitems[port]->chValue.U1 = 0;
                            dbitems[port]->ChSync();
                        }
                    }
                }
            }
            Gui::ItemTooltip("Zero rate for all messages");

            // Revert/current all values
            ImGui::SameLine(0.0f);
            if (ImGui::SmallButton("#"))
            {
                somethingChanged = true;
                for (auto entry: _msgrateItems)
                {
                    auto dbitems = entry.second.items;
                    if (port < 0)
                    {
                        for (int p = 0; p < NUM_PORTS; p++)
                        {
                            if (dbitems[p])
                            {
                                dbitems[p]->chValue._raw = dbitems[p]->chReference._raw;
                                dbitems[p]->ChSync();
                            }
                        }
                    }
                    else
                    {
                        if (dbitems[port])
                        {
                            dbitems[port]->chValue._raw = dbitems[port]->chReference._raw;
                            dbitems[port]->ChSync();
                        }
                    }
                }
            }
            Gui::ItemTooltip(_chValueRevertStrs[_chValueRef]);

            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::EndChild(); // Head
    }

    // Body
    {
        ImGui::BeginChild("##Body", ImVec2(0.0f, 0.0f));

        // Setup columns (again...)
        ImGui::Columns(nColumns, "##BodyCols");
        ImGui::SetColumnWidth(0, wName);
        for (int c = 1; c < nColumns; c++)
        {
            ImGui::SetColumnWidth(c, wRate);
        }

        const bool highlight = _dbFilterWidget.IsActive() && _dbFilterWidget.IsHighlight();
        const float dimVal = _winSettings->style.Alpha * 0.5f;

        for (auto entry: _msgrateItems)
        {
            // If receiver data available, skip items not available on receiver
            if (_dbPollDataAvail)
            {
                bool anyAvail = false;
                const auto items = entry.second.items;
                for (int port = 0; port < NUM_PORTS; port++)
                {
                    if (items[port]->valueValid)
                    {
                        anyAvail = true;
                        break;
                    }
                }
                if (!anyAvail)
                {
                    continue;
                }
            }

            const bool filterMatch = entry.second.filterMatch;
            const bool dim = highlight && !filterMatch;
            if (filterMatch || dim)
            {
                if (dim)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dimVal);
                }
                if (_DrawMsgRate(entry.first, entry.second))
                {
                    somethingChanged = true;
                }
                if (dim)
                {
                    ImGui::PopStyleVar();
                }
            }
        }

        ImGui::EndChild(); // Body
    }

    ImGui::EndChild(); // Table

    return somethingChanged;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinDataConfig::_DrawMsgRate(const std::string &msgName, Msgrates &msgrates)
{
    const float inputWidth  = 30.0f;
    const float dragSpeed   = 0.1f;
    const float itemSpacing = _winSettings->style.ItemInnerSpacing.x;

    auto items = msgrates.items;

    bool somethingChanged = false;

    bool anyPortEnabled = false;
    bool anyPortDirty   = false;
    bool anyPortAvail   = false;
    uint8_t anyPortRate = 0;
    const DbItem *rateItem = nullptr;
    for (int port = 0; port < NUM_PORTS; port++)
    {
        if (items[port])
        {
            if (!rateItem)
            {
                rateItem = items[port];
            }
            anyPortAvail = true;
            if (items[port]->chDirty)
            {
                anyPortDirty = true;
            }
            const uint8_t rate = items[port]->chValue.U1;
            if (rate > 0)
            {
                anyPortEnabled = true;
            }
            if (rate > anyPortRate)
            {
                anyPortRate = rate;
            }
        }
    }

    if (!anyPortAvail)
    {
        return false;
    }

    ImGui::PushID(msgName.c_str());

    // Message name (and hover highlight / tooltip for the entire row)
    {
        ImGui::AlignTextToFramePadding();
        if (anyPortEnabled) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTGREEN)); }
        ImGui::Selectable(msgName.c_str());
        if (anyPortEnabled) { ImGui::PopStyleColor(); }
        if (Gui::ItemTooltipBegin())
        {
            if (rateItem)
            {
                const auto pos = rateItem->title.find(" on port");
                ImGui::TextUnformatted(rateItem->title.c_str(), pos != std::string::npos ? rateItem->title.c_str() + pos : NULL);
                ImGui::Separator();
            }
            ImGui::TextUnformatted("Layer   UART1 UART2   SPI   I2C   USB");
            ImGui::Separator();
            for (int layer = 0; layer <= NUM_LAYERS; layer++)
            {
                std::string tooltip = (layer < NUM_LAYERS) ?
                    Ff::Sprintf("%-7s", ubloxcfg_layerName(_cfgLayers[layer])) : std::string("Changes");
                for (int port = 0; port < NUM_PORTS; port++)
                {
                    auto item = items[port];
                    if (layer == NUM_LAYERS)
                    {
                        tooltip += item && item->chDirty ? Ff::Sprintf("%6" PRIu8, item->chValue.U1) : std::string("     -");
                    }
                    else if (item && item->values[layer].valid)
                    {
                        tooltip += Ff::Sprintf("%6s", item->values[layer].str.c_str());
                    }
                    else
                    {
                        tooltip += std::string("     -");
                    }
                }
                if (layer == NUM_LAYERS)
                {
                    ImGui::Separator();
                }
                ImGui::TextUnformatted(tooltip.c_str());
            }
            Gui::ItemTooltipEnd();
        }
    }

    ImGui::NextColumn();

    // All ports, UART1, UART2, SPI, I2C, USB
    for (int port = -1; port < NUM_PORTS; port++)
    {
        auto item = port < 0 ? nullptr : items[port];

        // Skip controls for items that are not available
        if ( (port >= 0) && !item )
        {
            ImGui::NextColumn();
            continue;
        }

        ImGui::PushID(port);

        // Revert to current/default
        const bool dirty = port < 0 ? anyPortDirty : (item ? item->chDirty : false);
        if (dirty) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTMAGENTA)); } else { Gui::BeginDisabled(); };
        if (ImGui::Button(dirty ? "#" : " ", _winSettings->iconButtonSize))
        {
            somethingChanged = true;
            if (port < 0)
            {
                for (int p = 0; p < NUM_PORTS; p++)
                {
                    if (items[p])
                    {
                        items[p]->chValue._raw = items[p]->chReference._raw;
                        items[p]->ChSync();
                    }
                }
            }
            else
            {
                item->chValue._raw = item->chReference._raw;
                item->ChSync();
            }
        }
        if (dirty) { ImGui::PopStyleColor(); } else { Gui::EndDisabled(); };
        Gui::ItemTooltip(_chValueRevertStrs[_chValueRef]);

        ImGui::SameLine(0.0f, itemSpacing);

        // Toggle enable/disable
        if (ImGui::Button("T", _winSettings->iconButtonSize))
        {
            somethingChanged = true;
            if (port < 0)
            {
                for (int p = 0; p < NUM_PORTS; p++)
                {
                    if (items[p])
                    {
                        items[p]->chValue.U1 = anyPortEnabled ? 0 : 1;
                        items[p]->ChSync();
                    }
                }
            }
            else
            {
                item->chValue.U1 = item->chValue.U1 > 0 ? 0 : 1;
                item->ChSync();
            }
        }
        Gui::ItemTooltip("Toggle enable/disable");

        ImGui::SameLine(0.0f, itemSpacing);

        // The value
        if (port < 0)
        {
            const bool hl = anyPortEnabled;
            uint8_t rate = anyPortRate;
            if (hl) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTGREEN)); }
            ImGui::PushItemWidth(inputWidth);
            ImGui::DragScalar("##val", ImGuiDataType_U8, &rate, dragSpeed);
            ImGui::PopItemWidth();
            if (hl) { ImGui::PopStyleColor(); }
            if (rate != anyPortRate)
            {
                somethingChanged = true;
                for (int p = 0; p < NUM_PORTS; p++)
                {
                    if (items[p])
                    {
                        items[p]->chValue.U1 = rate;
                        items[p]->ChSync();
                    }
                }
            }
        }
        else
        {
            const bool hl = item->chValue.U1 > 0;
            if (hl) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTGREEN)); }
            ImGui::PushItemWidth(inputWidth);
            if (ImGui::DragScalar("##val", ImGuiDataType_U8, &item->chValue.U1, dragSpeed))
            {
                somethingChanged = true;
                item->ChSync();
            }
            ImGui::PopItemWidth();
            if (hl) { ImGui::PopStyleColor(); }
        }

        ImGui::SameLine(0.0f, itemSpacing);

        bool canInc = port < 0 ? false : (item->chValue.U1 < 0xff);
        bool canDec = port < 0 ? false : (item->chValue.U1 > 0x00);
        if (port < 0)
        {
            for (int p = 0; p < NUM_PORTS; p++)
            {
                if (items[p])
                {
                    if (items[p]->chValue.U1 < 0xff) { canInc = true; }
                    if (items[p]->chValue.U1 > 0x00) { canDec = true; }
                }
            }
        }

        ImGui::PushButtonRepeat(true);

        // Increment rate
        if (!canInc) { Gui::BeginDisabled(); }
        if (ImGui::Button("+", _winSettings->iconButtonSize))
        {
            somethingChanged = true;
            if (port < 0)
            {
                for (int p = 0; p < NUM_PORTS; p++)
                {
                    if (items[p])
                    {
                        if (items[p]->chValue.U1 < 0xff) { items[p]->chValue.U1++; }
                        items[p]->ChSync();
                    }
                }
            }
            else
            {
                item->chValue.U1++;
                item->ChSync();
            }
        }
        if (!canInc) { Gui::EndDisabled(); }
        Gui::ItemTooltip("Increment rate");

        ImGui::SameLine(0.0f, itemSpacing);

        // Decrement rate
        if (!canDec) { Gui::BeginDisabled(); }
        if (ImGui::Button("-", _winSettings->iconButtonSize))
        {
            somethingChanged = true;
            if (port < 0)
            {
                for (int p = 0; p < NUM_PORTS; p++)
                {
                    if (items[p])
                    {
                        if (items[p]->chValue.U1 > 0x00) { items[p]->chValue.U1--; }
                        items[p]->ChSync();
                    }
                }
            }
            else
            {
                item->chValue.U1--;
                item->ChSync();
            }
        }
        if (!canDec) { Gui::EndDisabled(); }
        Gui::ItemTooltip("Decrement rate");
        ImGui::PopButtonRepeat();

        ImGui::PopID();
        ImGui::NextColumn();
    }

    ImGui::Separator();

    ImGui::PopID();

    return somethingChanged;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinDataConfig::_DrawItems()
{
    const float currWidth = ImGui::GetWindowContentRegionWidth();

    const float charWidth = _winSettings->charSize.x;
    const float wItem  = 36 * charWidth;
    const float wType  =  4 * charWidth;
    const float wScale = 14 * charWidth;
    const float wUnit  = 10 * charWidth;
    const float wValueMin = 22 * charWidth;
    const float wValue = currWidth > (wItem + wType + wScale + wUnit + wValueMin) ?
        (currWidth - wItem - wType - wScale - wUnit) : wValueMin;

    const float width = wItem + wType + wScale + wUnit + wValue;
    const int nColumns = 5;

    // Child window of fixed width with horizontal scrolling
    ImGui::SetNextWindowContentSize(ImVec2(currWidth > width ? currWidth : width, 0.0f));
    ImGui::BeginChild("##Table_CfgItems", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    // Header
    {
        ImGui::BeginChild("##Head", ImVec2(0.0f, ImGui::GetTextLineHeight() * 1.5));
        // Setup columns
        ImGui::Columns(nColumns, "##HeadCols");
        ImGui::SetColumnWidth(0, wItem);
        ImGui::SetColumnWidth(1, wType);
        ImGui::SetColumnWidth(2, wScale);
        ImGui::SetColumnWidth(3, wUnit);
        ImGui::SetColumnWidth(4, wValue);

        // Title
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTCYAN));
        ImGui::TextUnformatted("Item");
        ImGui::NextColumn();
        ImGui::TextUnformatted("Type");
        ImGui::NextColumn();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::TextUnformatted("Unit");
        ImGui::NextColumn();
        ImGui::TextUnformatted("Value");
        ImGui::NextColumn();

        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::EndChild(); // Head
    }

    bool somethingChanged = true;

    // Body
    {
        ImGui::BeginChild("##Body", ImVec2(0.0f, 0.0f));

        // Setup columns (again...)
        ImGui::Columns(nColumns, "##BodyCols");
        ImGui::SetColumnWidth(0, wItem);
        ImGui::SetColumnWidth(1, wType);
        ImGui::SetColumnWidth(2, wScale);
        ImGui::SetColumnWidth(3, wUnit);
        ImGui::SetColumnWidth(4, wValue);

        const bool highlight = _dbFilterWidget.IsActive() && _dbFilterWidget.IsHighlight();
        const float dimVal = _winSettings->style.Alpha * 0.5f;

        // Draw list of items
        for (auto &dbitem: _dbItems)
        {
            // Skip items used in message rates and in port config views
            if (dbitem.specialItem)
            {
                continue;
            }
            // If receiver data available, skip items not available on receiver
            if (_dbPollDataAvail && !dbitem.valueValid)
            {
                continue;
            }

            const bool dim = highlight && !dbitem.filterMatch;
            if ( (dbitem.filterMatch || dim) && (_dbShowUnknown || dbitem.defitem) )
            {
                if (dim)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dimVal);
                }
                if (_DrawItem(dbitem))
                {
                    somethingChanged = true;
                }
                if (dim)
                {
                    ImGui::PopStyleVar();
                }
            }
        }

        ImGui::EndChild(); // Body
    }
    ImGui::EndChild(); // Table

    return somethingChanged;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinDataConfig::_DrawItem(DbItem &dbitem)
{
    //const float dragSpeed   = 0.1f;
    const float itemSpacing = _winSettings->style.ItemInnerSpacing.x;

    ImGui::PushID(dbitem.name.c_str());

    // Item name
    ImGui::AlignTextToFramePadding();
    ImGui::Selectable(dbitem.name.c_str());
    if (Gui::ItemTooltipBegin())
    {
        ImGui::TextUnformatted(dbitem.title.c_str());
        ImGui::Separator();
        ImGui::TextUnformatted("Layer    Value");
        ImGui::Separator();
        for (int layer = 0; layer <= NUM_LAYERS; layer++)
        {
            std::string tooltip = (layer < NUM_LAYERS) ?
                Ff::Sprintf("%-7s  ", ubloxcfg_layerName(_cfgLayers[layer])) : std::string("Changes  ");
            if (layer == NUM_LAYERS)
            {
                tooltip += dbitem.chDirty ? dbitem.chValueStr : std::string("-");
            }
            else
            {
                tooltip += (dbitem.values[layer].valid) ? dbitem.values[layer].str : std::string("-");
            }
            if (layer == NUM_LAYERS)
            {
                ImGui::Separator();
            }
            ImGui::TextUnformatted(tooltip.c_str());
        }
        Gui::ItemTooltipEnd();
    }

    ImGui::NextColumn();

    // Type
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(dbitem.typeStr.c_str());

    ImGui::NextColumn();

    // Scale
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(dbitem.scaleStr.c_str());

    ImGui::NextColumn();

    // Unit
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(dbitem.unitStr.c_str());

    ImGui::NextColumn();

    bool somethingChanged = false;

    // Revert to current/default value
    const bool dirty = dbitem.chDirty;
    if (dirty) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTMAGENTA)); } else { Gui::BeginDisabled(); };
    if (ImGui::Button(dirty ? "#" : " ", _winSettings->iconButtonSize))
    {
        dbitem.chValue._raw = dbitem.chReference._raw;
        dbitem.ChSync();
        somethingChanged = true;
    }
    if (dirty) { ImGui::PopStyleColor(); } else { Gui::EndDisabled(); };
    Gui::ItemTooltip(_chValueRevertStrs[_chValueRef]);

    ImGui::SameLine(0.0f, itemSpacing);

    // Value
    switch (dbitem.type)
    {
        case UBLOXCFG_TYPE_I1:
        case UBLOXCFG_TYPE_I2:
        case UBLOXCFG_TYPE_I4:
        case UBLOXCFG_TYPE_I8:
        case UBLOXCFG_TYPE_E1:
        case UBLOXCFG_TYPE_E2:
        case UBLOXCFG_TYPE_E4:
        {
            int64_t val  = dbitem.chValue.I1;
            int64_t min  = INT8_MIN;
            int64_t max  = INT8_MAX;
            int     dig  = 4;
            switch (dbitem.type)
            {
                case UBLOXCFG_TYPE_E1:
                case UBLOXCFG_TYPE_I1: break;
                case UBLOXCFG_TYPE_E2:
                case UBLOXCFG_TYPE_I2: val = dbitem.chValue.I2; dig =  6; min = INT16_MIN; max = INT16_MAX; break;
                case UBLOXCFG_TYPE_E4:
                case UBLOXCFG_TYPE_I4: val = dbitem.chValue.I4; dig = 11; min = INT32_MIN; max = INT32_MAX; break;
                case UBLOXCFG_TYPE_I8: val = dbitem.chValue.I8; dig = 20; min = INT64_MIN; max = INT64_MAX; break;
                default: break;
            }

            ImGui::PushButtonRepeat(true);

            // Increment
            const bool canInc = val < max;
            if (!canInc) { Gui::BeginDisabled(); };
            if (ImGui::Button("+", _winSettings->iconButtonSize))
            {
                val++;
                somethingChanged = true;
            }
            if (!canInc) { Gui::EndDisabled(); }
            Gui::ItemTooltip("Increment");

            ImGui::SameLine(0.0f, itemSpacing);

            // Decrement
            const bool canDec = val > min;
            if (!canDec) { Gui::BeginDisabled(); };
            if (ImGui::Button("-", _winSettings->iconButtonSize))
            {
                val--;
                somethingChanged = true;
            }
            if (!canDec) { Gui::EndDisabled(); }
            Gui::ItemTooltip("Decrement");

            ImGui::PopButtonRepeat();

            ImGui::SameLine(0.0f, itemSpacing);

            // Value
            ImGui::PushItemWidth((_winSettings->charSize.x * dig) + 5.0f);
            if (ImGui::InputScalar("##Value", ImGuiDataType_S64, &val, NULL, NULL, "%" PRIi64))
            {
                somethingChanged = true;
            }
            ImGui::PopItemWidth();

            // Constant selection
            if (ImGui::BeginPopupContextItem("Constants"))
            {
                char str[100];
                if (dbitem.defitem && (dbitem.defitem->nConsts > 0))
                {
                    ImGui::Separator();
                    bool valValid = false;
                    for (int ix = 0; ix < dbitem.defitem->nConsts; ix++)
                    {
                        const UBLOXCFG_CONST_t *c = &dbitem.defitem->consts[ix];
                        std::snprintf(str, sizeof(str), "%-20s (%s)", c->name, c->value);
                        if (ImGui::RadioButton(str, val == c->val.E))
                        {
                            val = c->val.E;
                            somethingChanged = true;
                        }
                        Gui::ItemTooltip(c->title);
                        if (val == c->val.E)
                        {
                            valValid = true;
                        }
                    }
                    if (!valValid)
                    {
                        ImGui::Separator();
                        std::snprintf(str, sizeof(str), "Undefined            (%" PRIi64 ")", val);
                        ImGui::RadioButton(str, true);
                    }
                }
                else
                {
                    snprintf(str, sizeof(str), "Minimum: %" PRIi64, min);
                    if (ImGui::RadioButton(str, val == min))
                    {
                        val = min;
                        somethingChanged = true;
                    }
                    snprintf(str, sizeof(str), "Maximum:  %" PRIi64, max);
                    if (ImGui::RadioButton(str, val == max))
                    {
                        val = max;
                        somethingChanged = true;
                    }
                    if (ImGui::RadioButton(       "Zero:     0", val == 0))
                    {
                        val = 0;
                        somethingChanged = true;
                    }
                }
                ImGui::EndPopup();
            }
            if (somethingChanged)
            {
                switch (dbitem.type)
                {
                    case UBLOXCFG_TYPE_I1: dbitem.chValue.I1 = val;  break;
                    case UBLOXCFG_TYPE_I2: dbitem.chValue.I2 = val;  break;
                    case UBLOXCFG_TYPE_I4: dbitem.chValue.I4 = val;  break;
                    case UBLOXCFG_TYPE_I8: dbitem.chValue.I8 = val;  break;
                    case UBLOXCFG_TYPE_E1: dbitem.chValue.E1 = val;  break;
                    case UBLOXCFG_TYPE_E2: dbitem.chValue.E2 = val;  break;
                    case UBLOXCFG_TYPE_E4: dbitem.chValue.E4 = val;  break;
                    default: break;
                }
                dbitem.ChSync();
            }
            break;
        }
        case UBLOXCFG_TYPE_U1:
        case UBLOXCFG_TYPE_U2:
        case UBLOXCFG_TYPE_U4:
        case UBLOXCFG_TYPE_U8:
        case UBLOXCFG_TYPE_X1:
        case UBLOXCFG_TYPE_X2:
        case UBLOXCFG_TYPE_X4:
        case UBLOXCFG_TYPE_X8:
        {
            uint64_t    val  = dbitem.chValue.U1;
            uint64_t    min  = 0;
            uint64_t    max  = UINT8_MAX;
            int         dig  = 3;
            const char *fmt  = "%" PRIu64;;
            bool        hex  = false;
            switch (dbitem.type)
            {
                case UBLOXCFG_TYPE_U1: break;
                case UBLOXCFG_TYPE_U2: val = dbitem.chValue.U2; dig =  5; max = UINT16_MAX; break;
                case UBLOXCFG_TYPE_U4: val = dbitem.chValue.U4; dig = 10; max = UINT32_MAX; break;
                case UBLOXCFG_TYPE_U8: val = dbitem.chValue.U8; dig = 20; max = UINT64_MAX; break;
                case UBLOXCFG_TYPE_X1: val = dbitem.chValue.X1; dig =  2; max = UINT8_MAX;  fmt = "0x%02" PRIx64;  hex = true; break;
                case UBLOXCFG_TYPE_X2: val = dbitem.chValue.X2; dig =  4; max = UINT16_MAX; fmt = "0x%04" PRIx64;  hex = true; break;
                case UBLOXCFG_TYPE_X4: val = dbitem.chValue.X4; dig =  8; max = UINT32_MAX; fmt = "0x%08" PRIx64;  hex = true; break;
                case UBLOXCFG_TYPE_X8: val = dbitem.chValue.X8; dig = 16; max = UINT64_MAX; fmt = "0x%016" PRIx64; hex = true; break;
                default: break;
            }

            ImGui::PushButtonRepeat(true);

            // Increment
            const bool canInc = val < max;
            if (!canInc) { Gui::BeginDisabled(); };
            if (ImGui::Button("+", _winSettings->iconButtonSize))
            {
                val++;
                somethingChanged = true;
            }
            if (!canInc) { Gui::EndDisabled(); }
            Gui::ItemTooltip("Increment");

            ImGui::SameLine(0.0f, itemSpacing);

            // Decrement
            const bool canDec = val > min;
            if (!canDec) { Gui::BeginDisabled(); };
            if (ImGui::Button("-", _winSettings->iconButtonSize))
            {
                val--;
                somethingChanged = true;
            }
            if (!canDec) { Gui::EndDisabled(); }
            Gui::ItemTooltip("Decrement");

            ImGui::PopButtonRepeat();

            ImGui::SameLine(0.0f, itemSpacing);

            // Value
            ImGui::PushItemWidth((_winSettings->charSize.x * dig) + 5.0f);
            if ( hex ?
                 ImGui::InputScalar("##Value", ImGuiDataType_U64, &val, NULL, NULL, &fmt[2], ImGuiInputTextFlags_CharsHexadecimal) : // FIXME: doesn't work with fmt "0x..."
                 ImGui::InputScalar("##Value", ImGuiDataType_U64, &val, NULL, NULL, fmt)
                 /*ImGui::DragScalar("##Value", type, &val, dragSpeed, NULL, NULL, fmt)*/ ) // FIXME: DragScalar() doesn't work with hex
            {
                somethingChanged = true;
            }
            ImGui::PopItemWidth();

            // Constant selection
            if (ImGui::BeginPopupContextItem("Constants"))
            {
                char fmt2[100];
                snprintf(fmt2, sizeof(fmt2), "%%-20s (%s)", fmt);
                char str[100];
                if (dbitem.defitem && (dbitem.defitem->nConsts > 0))
                {
                    uint64_t allbits = 0;
                    ImGui::Separator();
                    for (int ix = 0; ix < dbitem.defitem->nConsts; ix++)
                    {
                        const UBLOXCFG_CONST_t *c = &dbitem.defitem->consts[ix];
                        std::snprintf(str, sizeof(str), fmt2, c->name, c->val.X);
                        if (Gui::CheckboxFlagsX8(str, &val, c->val.X))
                        {
                            somethingChanged = true;
                        }
                        Gui::ItemTooltip(c->title);
                        allbits |= c->val.X;
                    }
                    ImGui::Separator();
                    std::snprintf(str, sizeof(str), fmt2, "All", allbits);
                    if (Gui::CheckboxFlagsX8(str, &val, allbits))
                    {
                        somethingChanged = true;
                    }
                    const uint64_t rembits = (~allbits) & val;
                    if (rembits != 0)
                    {
                        std::snprintf(str, sizeof(str), fmt2, "Undefined", rembits);
                        if (Gui::CheckboxFlagsX8(str, &val, rembits))
                        {
                            somethingChanged = true;
                        }
                    }
                }
                else
                {
                    snprintf(str, sizeof(str), fmt2, "Minimum", min);
                    if (ImGui::RadioButton(str, val == min))
                    {
                        val = min;
                        somethingChanged = true;
                    }
                    snprintf(str, sizeof(str), fmt2, "Maximum", max);
                    if (ImGui::RadioButton(str, val == max))
                    {
                        val = max;
                        somethingChanged = true;
                    }
                }
                ImGui::EndPopup();
            }
            if (somethingChanged)
            {
                switch (dbitem.type)
                {
                    case UBLOXCFG_TYPE_U1: dbitem.chValue.U1 = (uint8_t)val;  break;
                    case UBLOXCFG_TYPE_U2: dbitem.chValue.U2 = (uint16_t)val; break;
                    case UBLOXCFG_TYPE_U4: dbitem.chValue.U4 = (uint32_t)val; break;
                    case UBLOXCFG_TYPE_U8: dbitem.chValue.U8 = (uint64_t)val; break;
                    case UBLOXCFG_TYPE_X1: dbitem.chValue.X1 = (uint8_t)val;  break;
                    case UBLOXCFG_TYPE_X2: dbitem.chValue.X2 = (uint16_t)val; break;
                    case UBLOXCFG_TYPE_X4: dbitem.chValue.X4 = (uint32_t)val; break;
                    case UBLOXCFG_TYPE_X8: dbitem.chValue.X8 = (uint64_t)val; break;
                    default: break;
                }
                dbitem.ChSync();
            }
            break;
        }
        case UBLOXCFG_TYPE_R4:
        case UBLOXCFG_TYPE_R8:
        {
            double       val = dbitem.type == UBLOXCFG_TYPE_R4 ? dbitem.chValue.R4 : dbitem.chValue.R8;
            const double max = dbitem.type == UBLOXCFG_TYPE_R4 ? FLT_MAX : DBL_MAX;
            const double min = -max;
            const double step = val > 100.0 ? std::fabs(val * 1e-2) /* 1% */ : 1.0;

            ImGui::PushButtonRepeat(true);

            // Increment
            const bool canInc = val < max;
            if (!canInc) { Gui::BeginDisabled(); };
            if (ImGui::Button("+", _winSettings->iconButtonSize))
            {
                val += step;
                val = std::floor(val + 0.5);
                somethingChanged = true;
            }
            if (!canInc) { Gui::EndDisabled(); }
            Gui::ItemTooltip("Increment");

            ImGui::SameLine(0.0f, itemSpacing);

            // Decrement
            const bool canDec = val > min;
            if (!canDec) { Gui::BeginDisabled(); };
            if (ImGui::Button("-", _winSettings->iconButtonSize))
            {
                val -= step;
                val = std::floor(val - 0.5);
                somethingChanged = true;
            }
            if (!canDec) { Gui::EndDisabled(); }
            Gui::ItemTooltip("Decrement");

            ImGui::PopButtonRepeat();

            ImGui::SameLine(0.0f, itemSpacing);

            // Value
            ImGui::PushItemWidth((_winSettings->charSize.x * 15) + 5.0f);
            if (ImGui::InputScalar("##Value", ImGuiDataType_Double, &val, NULL, NULL, "%g"))
            {
                somethingChanged = true;
            }
            ImGui::PopItemWidth();

            // Popup
            if (ImGui::BeginPopupContextItem("Constants"))
            {
                char str[100];
                std::snprintf(str, sizeof(str), "Min:  %g", min);
                if (ImGui::MenuItem(str))
                {
                    val = min;
                    somethingChanged = true;
                }
                std::snprintf(str, sizeof(str), "Max:   %g", max);
                if (ImGui::MenuItem(str))
                {
                    val = max;
                    somethingChanged = true;
                }
                if (ImGui::MenuItem("Zero: 0.0"))
                {
                    val = 0.0;
                    somethingChanged = true;
                }
                ImGui::Separator();
                ImGui::TextUnformatted("Binary hex:");
                if (dbitem.type == UBLOXCFG_TYPE_R8)
                {
                    uint64_t valHex;
                    memcpy(&valHex, &val, sizeof(valHex));
                    ImGui::PushItemWidth((_winSettings->charSize.x * 16) + 5.0f);
                    if (ImGui::InputScalar("##DoubleHex", ImGuiDataType_U64, &valHex, NULL, NULL, "%016llx", ImGuiInputTextFlags_CharsHexadecimal))
                    {
                        memcpy(&val, &valHex, sizeof(val));
                        somethingChanged = true;
                    }
                    ImGui::PopItemWidth();
                }
                else
                {
                    float valFloat = (float)val;
                    uint32_t valHex;
                    memcpy(&valHex, &valFloat, sizeof(valHex));
                    ImGui::PushItemWidth((_winSettings->charSize.x * 8) + 5.0f);
                    if (ImGui::InputScalar("##FloatHex", ImGuiDataType_U32, &valHex, NULL, NULL, "%08lx", ImGuiInputTextFlags_CharsHexadecimal))
                    {
                        memcpy(&valFloat, &valHex, sizeof(valFloat));
                        val = valFloat;
                        somethingChanged = true;
                    }
                    ImGui::PopItemWidth();
                }
                ImGui::EndPopup();
            }
            if (somethingChanged)
            {
                switch (dbitem.type)
                {
                    case UBLOXCFG_TYPE_R4: dbitem.chValue.R4 = (float)val; break;
                    case UBLOXCFG_TYPE_R8: dbitem.chValue.R8 =        val; break;
                    default: break;
                }
                dbitem.ChSync();
            }
            break;
        }
        case UBLOXCFG_TYPE_L:
        {
            ImGui::SameLine(0.0f, (3 * itemSpacing) + (2 * _winSettings->iconButtonSize.x));
            if (ImGui::Checkbox("##L", &dbitem.chValue.L))
            {
                somethingChanged = true;
                dbitem.ChSync();
            }
            break;
        }
    }

    // Value string
    Gui::VerticalSeparator(230.0f);
    ImGui::TextUnformatted(dbitem.chPrettyStr.c_str());

    ImGui::NextColumn();

    ImGui::Separator();

    ImGui::PopID();

    return somethingChanged;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataConfig::_DrawChanges()
{
    if (_cfgChangedStrs.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_WHITE));
        ImGui::TextUnformatted("No configuration changes...");
        ImGui::PopStyleColor();
        return;
    }

    // Config file contents
    //const float currWidth = ImGui::GetWindowContentRegionWidth();
    //ImGui::SetNextWindowContentSize(ImVec2(currWidth, 0.0f));
    ImGui::BeginChild("##CfgFile", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
    {
        for (auto &str: _cfgChangedStrs)
        {
            ImGui::TextUnformatted(str.c_str());
        }
    }
    ImGui::EndChild();
}

/* ****************************************************************************************************************** */
