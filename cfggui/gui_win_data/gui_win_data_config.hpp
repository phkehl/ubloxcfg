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

#ifndef __GUI_WIN_DATA_CONFIG_H__
#define __GUI_WIN_DATA_CONFIG_H__

#include "gui_win_data.hpp"

/* ***** Receiver configuration ************************************************************************************* */

class GuiWinDataConfig : public GuiWinData
{
    public:
        GuiWinDataConfig(const std::string &name,
            std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile, std::shared_ptr<Database> database);

        void                 Loop(const uint32_t &frame, const double &now) override;
        void                 ProcessData(const Data &data) override;
        void                 ClearData() override;
        void                 DrawWindow() override;

    protected:

        //! Index for DbItem.values[]
        enum DbItemLayer_e : int { IX_RAM = 0, IX_BBR = 1, IX_FLASH = 2, IX_DEFAULT = 3, NUM_LAYERS = 4 };
        //! Layer corresponding to #DbItemLayer_e
        static const UBLOXCFG_LAYER_t _cfgLayers[NUM_LAYERS];

        // Polling receiver config database
        static const UBLOXCFG_LAYER_t _dbPollLayers[NUM_LAYERS];
        enum PollState_e { POLL_IDLE, POLL_INIT, POLL_POLL, POLL_WAIT, POLL_DONE, POLL_FAIL };
        enum PollState_e     _dbPollState;
        std::size_t          _dbPollLayerIx;
        double               _dbPollTime;
        int                  _dbPollProgress;
        int                  _dbPollProgressMax;
        bool                 _dbPollDataAvail;
        enum SetState_e { SET_IDLE, SET_START, SET_WAIT, SET_DONE };
        enum SetState_e      _dbSetState;
        int                  _dbSetNumValset;
        int                  _dbSetTotValset;
        bool                 _dbApplyTrigger;
        bool                 _dbSetRam;
        bool                 _dbSetBbr;
        bool                 _dbSetFlash;
        bool                 _dbSetApply;

        //! Receiver config database entry
        struct DbItem
        {
            DbItem(const uint32_t _id);
            std::string            name;         //!< Name of the item: CFG-FOO-BAR, or 0x........ for unknown
            uint32_t               id;           //!< ID of the item
            std::string            idStr;        //!< ID as string: "0x........"
            UBLOXCFG_SIZE_t        size;         //!< Value (storage) size
            UBLOXCFG_TYPE_t        type;         //!< Value (storage) type
            std::string            typeStr;      //!< Type as string ("U1", "E2", "I4", "X8", etc.)
            const UBLOXCFG_ITEM_t *defitem;      //!< Pointer to definition, or nullptr for unknown
            std::string            title;        //!< Title (short description)
            std::string            unitStr;      //!< Unit ("s", "m", "m/s", etc.) or empty
            std::string            scaleStr;     //!< Scale ("0.001", "1e3", etc.) or empty

            // Value in each layer
            struct Value
            {
                Value() : valid{false}, val{}, str{}, changed{false} { }
                bool               valid;        //!< Data (val, str, changed) is valid
                UBLOXCFG_VALUE_t   val;          //!< The value
                std::string        str;          //!< The value as a string
                bool               changed;      //!< Value changed (different) from value in Default layer?
            };
            Value                  values[NUM_LAYERS];
            bool                   valueChanged; //!< One or more values changed?
            bool                   valueValid;   //!< One or more values valid (available)? Implies: available on this receiver
            void                   SetValue(const UBLOXCFG_VALUE_t &val, const enum DbItemLayer_e layerIx);
            void                   ClearValues();//!< Clear (invalidate) all values
            void                   _SyncValues(); //!< Update (filterString, changed flags)

            bool                   filterMatch;  //!< Filter matches this item
            std::string            filterString; //!< String to match filter against
            bool                   specialItem;  //!< Is special (port config, output message rate config) item?

            // Changes
            UBLOXCFG_VALUE_t       chValue;
            UBLOXCFG_VALUE_t       chReference;
            std::string            chValueStr;
            std::string            chPrettyStr;
            bool                   chDirty;
            void                   ChSet(const int valueLayerIx, const int referenceLayerIx);
            void                   ChSync();
        };
        std::vector<DbItem>  _dbItems;

        // Special port and output message rate configurations
        enum Port_e  : int { UART1 = 0, UART2 = 1, SPI = 2, I2C = 3, USB = 4, NUM_PORTS = 5 };
        enum Prot_e  : int { UBX = 0, NMEA = 1, RTCM3 = 2, NUM_PROTS = 3 };
        enum InOut_e : int { INPROT = 0, OUTPROT = 1, NUM_INOUT = 2 };
        static const char * const _portNames[NUM_PORTS];
        static const char * const _protNames[NUM_PROTS];
        static const uint32_t     _protItemIds[NUM_INOUT][NUM_PORTS][NUM_PROTS];
        static const uint32_t     _baudItemIds[NUM_PORTS];
        DbItem                   *_protDbitems[NUM_INOUT][NUM_PORTS][NUM_PROTS];  //!< _dbItems for port in/out protocol config
        DbItem                   *_baudDbitems[NUM_PORTS];                        //!< _dbItems for port baudrate config
        struct Msgrates { bool filterMatch; DbItem *items[NUM_PORTS]; Msgrates() : filterMatch{true}, items{} {} };
        std::map<std::string, Msgrates> _msgrateItems;                             //!< _dbItems for output message rate config

        enum DbItemOrder_e : int { DB_ORDER_BY_NAME, DB_ORDER_BY_ID, DB_ORDER_BY_ID_IGN_SIZE };
        enum DbItemOrder_e   _dbItemOrder;
        bool                 _dbShowUnknown;
        enum ChValueRef_e : int { REF_CURRENT = 0, REF_DEFAULT = 1};
        enum ChValueRef_e    _chValueRef;
        static const char * const _chValueRevertStrs[2];
        uint32_t             _dbItemDispCount[NUM_LAYERS];

        GuiWidgetFilter      _dbFilterWidget;
        bool                 _dbFilterUpdate;

        void                 _DbInit();  //!< Initialise DB, populate with known (documented) items
        void                 _DbSync();  //!< (Re-)sort database, do stuff
        void                 _DbClear(); //!< Clear (receiver) values
        void                 _DbUpdCh();
        std::vector<DbItem>::iterator _DbGetItem(const std::string &name);
        std::vector<DbItem>::iterator _DbGetItem(const uint32_t id);

        std::vector<UBLOXCFG_KEYVAL_t> _cfgChangedKv;
        std::vector<std::string> _cfgChangedStrs;
        void                 _UpdateChanges();

        std::string          _cfgFileName;
        double               _cfgFileSaveResultTo;
        std::string          _cfgFileSaveError;

        // Draw window
        bool                 _DrawControls();
        void                 _DrawDb();
        bool                 _DrawMsgRates();
        bool                 _DrawMsgRate(const std::string &msgName, Msgrates &msgrates);
        bool                 _DrawPorts();
        bool                 _DrawItems();
        bool                 _DrawItem(DbItem &dbitem);
        void                 _DrawChanges();
};


/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_CONFIG_H__
