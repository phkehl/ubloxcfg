// flipflip's UBX protocol stuff
//
// Copyright (c) 2020 Philippe Kehl (flipflip at oinkzwurgl dot org),
// https://oinkzwurgl.org/hacking/ubloxcfg
//
// Copyright (c) 2021 Charles Parent (charles.parent@orolia2s.com)
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

#ifndef __FF_UBX_H__
#define __FF_UBX_H__

#include <stdint.h>
#include <stdbool.h>

#include "ubloxcfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

#define UBX_FRAME_SIZE    8           //!< Size (in bytes) of UBX frame
#define UBX_SYNC_1        0xb5        //!< UBX frame sync char 1
#define UBX_SYNC_2        0x62        //!< UBX frame sync char 2
#define UBX_HEAD_SIZE     6           //!< Size of UBX frame header
#define UBX_CLSID(msg)    ((msg)[2])  //!< Get class ID from message
#define UBX_MSGID(msg)    ((msg)[3])  //!< Get message ID from message

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_ACK_CLSID                0x05
#define UBX_ACK_ACK_MSGID            0x01
#define UBX_ACK_NAK_MSGID            0x00

#define UBX_CFG_CLSID                0x06
#define UBX_CFG_PWR_MSGID            0x57
#define UBX_CFG_RST_MSGID            0x04
#define UBX_CFG_CFG_MSGID            0x09
#define UBX_CFG_VALSET_MSGID         0x8a
#define UBX_CFG_VALGET_MSGID         0x8b
#define UBX_CFG_VALDEL_MSGID         0x8c

#define UBX_ESF_CLSID                0x10
#define UBX_ESF_ALG_MSGID            0x14
#define UBX_ESF_INS_MSGID            0x15
#define UBX_ESF_MEAS_MSGID           0x02
#define UBX_ESF_RAW_MSGID            0x03
#define UBX_ESF_STATUS_MSGID         0x10

#define UBX_INF_CLSID                0x04
#define UBX_INF_ERROR_MSGID          0x00
#define UBX_INF_WARNING_MSGID        0x01
#define UBX_INF_NOTICE_MSGID         0x02
#define UBX_INF_TEST_MSGID           0x03
#define UBX_INF_DEBUG_MSGID          0x04

#define UBX_LOG_CLSID                0x21
#define UBX_LOG_CREATE_MSGID         0x07
#define UBX_LOG_ERASE_MSGID          0x03
#define UBX_LOG_FINDTIME_MSGID       0x0e
#define UBX_LOG_INFO_MSGID           0x08
#define UBX_LOG_RETRPOSX_MSGID       0x0f
#define UBX_LOG_RETRPOS_MSGID        0x0b
#define UBX_LOG_RETRSTR_MSGID        0x0d
#define UBX_LOG_RETR_MSGID           0x09
#define UBX_LOG_STR_MSGID            0x04

#define UBX_MGA_CLSID                0x13
#define UBX_MGA_ACK_MSGID            0x60
#define UBX_MGA_DBD_MSGID            0x80
#define UBX_MGA_INI_MSGID            0x40
#define UBX_MGA_GPS_MSGID            0x00
#define UBX_MGA_GAL_MSGID            0x02
#define UBX_MGA_BDS_MSGID            0x03
#define UBX_MGA_QZSS_MSGID           0x05
#define UBX_MGA_GLO_MSGID            0x06

#define UBX_MON_CLSID                0x0a
#define UBX_MON_GNSS_MSGID           0x28
#define UBX_MON_HW_MSGID             0x09
#define UBX_MON_HW2_MSGID            0x0b
#define UBX_MON_HW3_MSGID            0x37
#define UBX_MON_RF_MSGID             0x38
#define UBX_MON_IO_MSGID             0x02
#define UBX_MON_COMMS_MSGID          0x36
#define UBX_MON_MSGPP_MSGID          0x06
#define UBX_MON_PATCH_MSGID          0x27
#define UBX_MON_RXR_MSGID            0x21
#define UBX_MON_RXBUF_MSGID          0x07
#define UBX_MON_SPAN_MSGID           0x31
#define UBX_MON_TXBUF_MSGID          0x08
#define UBX_MON_VER_MSGID            0x04
#define UBX_MON_TEMP_MSGID           0x0e // says u-center...

#define UBX_NAV_CLSID                0x01
#define UBX_NAV_PVT_MSGID            0x07
#define UBX_NAV_SAT_MSGID            0x35
#define UBX_NAV_ORB_MSGID            0x34
#define UBX_NAV_STATUS_MSGID         0x03
#define UBX_NAV_SIG_MSGID            0x43
#define UBX_NAV_CLOCK_MSGID          0x22
#define UBX_NAV_DOP_MSGID            0x04
#define UBX_NAV_POSECEF_MSGID        0x01
#define UBX_NAV_HPPOSECEF_MSGID      0x13
#define UBX_NAV_POSLLH_MSGID         0x02
#define UBX_NAV_HPPOSLLH_MSGID       0x14
#define UBX_NAV_RELPOSNED_MSGID      0x3c
#define UBX_NAV_VELECEF_MSGID        0x11
#define UBX_NAV_VELNED_MSGID         0x12
#define UBX_NAV_SVIN_MSGID           0x3b
#define UBX_NAV_EOE_MSGID            0x61
#define UBX_NAV_GEOFENCE_MSGID       0x39
#define UBX_NAV_ODO_MSGID            0x09
#define UBX_NAV_RESETODO_MSGID       0x10
#define UBX_NAV_TIMEUTC_MSGID        0x21
#define UBX_NAV_TIMELS_MSGID         0x26
#define UBX_NAV_TIMEGPS_MSGID        0x20
#define UBX_NAV_TIMEGLO_MSGID        0x23
#define UBX_NAV_TIMEBDS_MSGID        0x24
#define UBX_NAV_TIMEGAL_MSGID        0x25
#define UBX_NAV_TIMELS_MSGID         0x26
#define UBX_NAV_COV_MSGID            0x36
#define UBX_NAV_EELL_MSGID           0x3d
#define UBX_NAV_ATT_MSGID            0x05
#define UBX_NAV_PVAT_MSGID           0x17

#define UBX_RXM_CLSID                0x02
#define UBX_RXM_MEASX_MSGID          0x14
#define UBX_RXM_RAWX_MSGID           0x15
#define UBX_RXM_SFRBX_MSGID          0x13
#define UBX_RXM_PMREQ_MSGID          0x41
#define UBX_RXM_RLM_MSGID            0x59
#define UBX_RXM_RTCM_MSGID           0x32
#define UBX_RXM_SPARTN_MSGID         0x33

#define UBX_SEC_CLSID                0x27
#define UBX_SEC_UNIQUEID_MSGID       0x03

#define UBX_TIM_CLSID                0x0d
#define UBX_TIM_TM2_MSGID            0x03
#define UBX_TIM_TP_MSGID             0x01
#define UBX_TIM_VRFY_MSGID           0x06

#define UBX_UPD_CLSID                0x09
#define UBX_UPD_SOS_MSGID            0x14
#define UBX_UPD_POS_MSGID            0x15 // says u-center...
#define UBX_UPD_SAFEBOOT_MSGID       0x07 // says ubxfwupdate.exe that comes with u-center...

#define UBX_SEC_CLSID                0x27
#define UBX_SEC_SIG_MSGID            0x09
#define UBX_SEC_UNIQID_MSGID         0x03

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_CLASSES(_P_) \
    _P_(UBX_ACK_CLSID, "UBX-ACK") \
    _P_(UBX_CFG_CLSID, "UBX-CFG") \
    _P_(UBX_ESF_CLSID, "UBX-ESF") \
    _P_(UBX_INF_CLSID, "UBX-INF") \
    _P_(UBX_LOG_CLSID, "UBX-LOG") \
    _P_(UBX_MGA_CLSID, "UBX-MGA") \
    _P_(UBX_MON_CLSID, "UBX-MON") \
    _P_(UBX_NAV_CLSID, "UBX-NAV") \
    _P_(UBX_RXM_CLSID, "UBX-RXM") \
    _P_(UBX_SEC_CLSID, "UBX-SEC") \
    _P_(UBX_TIM_CLSID, "UBX-TIM") \
    _P_(UBX_UPD_CLSID, "UBX-UPD")

#define UBX_MESSAGES(_P_) \
    _P_(UBX_ACK_CLSID, UBX_ACK_ACK_MSGID,         "UBX-ACK-ACK") \
    _P_(UBX_ACK_CLSID, UBX_ACK_NAK_MSGID,         "UBX-ACK-NAK") \
    _P_(UBX_CFG_CLSID, UBX_CFG_PWR_MSGID,         "UBX-CFG-PWR") \
    _P_(UBX_CFG_CLSID, UBX_CFG_RST_MSGID,         "UBX-CFG-RST") \
    _P_(UBX_CFG_CLSID, UBX_CFG_CFG_MSGID,         "UBX-CFG-CFG") \
    _P_(UBX_CFG_CLSID, UBX_CFG_VALSET_MSGID,      "UBX-CFG-VALSET") \
    _P_(UBX_CFG_CLSID, UBX_CFG_VALGET_MSGID,      "UBX-CFG-VALGET") \
    _P_(UBX_CFG_CLSID, UBX_CFG_VALDEL_MSGID,      "UBX-CFG-VALDEL") \
    _P_(UBX_ESF_CLSID, UBX_ESF_ALG_MSGID,         "UBX-ESF-ALG") \
    _P_(UBX_ESF_CLSID, UBX_ESF_INS_MSGID,         "UBX-ESF-INS") \
    _P_(UBX_ESF_CLSID, UBX_ESF_MEAS_MSGID,        "UBX-ESF-MEAS") \
    _P_(UBX_ESF_CLSID, UBX_ESF_RAW_MSGID,         "UBX-ESF-RAW") \
    _P_(UBX_ESF_CLSID, UBX_ESF_STATUS_MSGID,      "UBX-ESF-STATUS") \
    _P_(UBX_INF_CLSID, UBX_INF_ERROR_MSGID,       "UBX-INF-ERROR") \
    _P_(UBX_INF_CLSID, UBX_INF_WARNING_MSGID,     "UBX-INF-WARNING") \
    _P_(UBX_INF_CLSID, UBX_INF_NOTICE_MSGID,      "UBX-INF-NOTICE") \
    _P_(UBX_INF_CLSID, UBX_INF_TEST_MSGID,        "UBX-INF-TEST") \
    _P_(UBX_INF_CLSID, UBX_INF_DEBUG_MSGID,       "UBX-INF-DEBUG") \
    _P_(UBX_LOG_CLSID, UBX_LOG_CREATE_MSGID,      "UBX-LOG-CREATE") \
    _P_(UBX_LOG_CLSID, UBX_LOG_ERASE_MSGID,       "UBX-LOG-ERASE") \
    _P_(UBX_LOG_CLSID, UBX_LOG_FINDTIME_MSGID,    "UBX-LOG-FINDTIME") \
    _P_(UBX_LOG_CLSID, UBX_LOG_INFO_MSGID,        "UBX-LOG-INFO") \
    _P_(UBX_LOG_CLSID, UBX_LOG_RETRPOSX_MSGID,    "UBX-LOG-RETRPOSX") \
    _P_(UBX_LOG_CLSID, UBX_LOG_RETRPOS_MSGID,     "UBX-LOG-RETRPOS") \
    _P_(UBX_LOG_CLSID, UBX_LOG_RETRSTR_MSGID,     "UBX-LOG-RETRSTR") \
    _P_(UBX_LOG_CLSID, UBX_LOG_RETR_MSGID,        "UBX-LOG-RETR") \
    _P_(UBX_LOG_CLSID, UBX_LOG_STR_MSGID,         "UBX-LOG-STR") \
    _P_(UBX_MGA_CLSID, UBX_MGA_ACK_MSGID,         "UBX-MGA-ACK") \
    _P_(UBX_MGA_CLSID, UBX_MGA_DBD_MSGID,         "UBX-MGA-DBD") \
    _P_(UBX_MGA_CLSID, UBX_MGA_INI_MSGID,         "UBX-MGA-INI") \
    _P_(UBX_MGA_CLSID, UBX_MGA_GPS_MSGID,         "UBX-MGA-GPS") \
    _P_(UBX_MGA_CLSID, UBX_MGA_GAL_MSGID,         "UBX-MGA-GAL") \
    _P_(UBX_MGA_CLSID, UBX_MGA_BDS_MSGID,         "UBX-MGA-BDS") \
    _P_(UBX_MGA_CLSID, UBX_MGA_QZSS_MSGID,        "UBX-MGA-QZSS") \
    _P_(UBX_MGA_CLSID, UBX_MGA_GLO_MSGID,         "UBX-MGA-GLO") \
    _P_(UBX_MON_CLSID, UBX_MON_GNSS_MSGID,        "UBX-MON-GNSS") \
    _P_(UBX_MON_CLSID, UBX_MON_HW_MSGID,          "UBX-MON-HW") \
    _P_(UBX_MON_CLSID, UBX_MON_HW2_MSGID,         "UBX-MON-HW2") \
    _P_(UBX_MON_CLSID, UBX_MON_HW3_MSGID,         "UBX-MON-HW3") \
    _P_(UBX_MON_CLSID, UBX_MON_RF_MSGID,          "UBX-MON-RF") \
    _P_(UBX_MON_CLSID, UBX_MON_IO_MSGID,          "UBX-MON-IO") \
    _P_(UBX_MON_CLSID, UBX_MON_COMMS_MSGID,       "UBX-MON-COMMS") \
    _P_(UBX_MON_CLSID, UBX_MON_MSGPP_MSGID,       "UBX-MON-MSGPP") \
    _P_(UBX_MON_CLSID, UBX_MON_PATCH_MSGID,       "UBX-MON-PATCH") \
    _P_(UBX_MON_CLSID, UBX_MON_RXR_MSGID,         "UBX-MON-RXR") \
    _P_(UBX_MON_CLSID, UBX_MON_RXBUF_MSGID,       "UBX-MON-RXBUF") \
    _P_(UBX_MON_CLSID, UBX_MON_SPAN_MSGID,        "UBX-MON-SPAN") \
    _P_(UBX_MON_CLSID, UBX_MON_TXBUF_MSGID,       "UBX-MON-TXBUF") \
    _P_(UBX_MON_CLSID, UBX_MON_TEMP_MSGID,        "UBX-MON-TEMP") \
    _P_(UBX_MON_CLSID, UBX_MON_VER_MSGID,         "UBX-MON-VER") \
    _P_(UBX_NAV_CLSID, UBX_NAV_PVT_MSGID,         "UBX-NAV-PVT") \
    _P_(UBX_NAV_CLSID, UBX_NAV_SAT_MSGID,         "UBX-NAV-SAT") \
    _P_(UBX_NAV_CLSID, UBX_NAV_ORB_MSGID,         "UBX-NAV-ORB") \
    _P_(UBX_NAV_CLSID, UBX_NAV_STATUS_MSGID,      "UBX-NAV-STATUS") \
    _P_(UBX_NAV_CLSID, UBX_NAV_SIG_MSGID,         "UBX-NAV-SIG") \
    _P_(UBX_NAV_CLSID, UBX_NAV_CLOCK_MSGID,       "UBX-NAV-CLOCK") \
    _P_(UBX_NAV_CLSID, UBX_NAV_DOP_MSGID,         "UBX-NAV-DOP") \
    _P_(UBX_NAV_CLSID, UBX_NAV_POSECEF_MSGID,     "UBX-NAV-POSECEF") \
    _P_(UBX_NAV_CLSID, UBX_NAV_HPPOSECEF_MSGID,   "UBX-NAV-HPPOSECEF") \
    _P_(UBX_NAV_CLSID, UBX_NAV_POSLLH_MSGID,      "UBX-NAV-POSLLH") \
    _P_(UBX_NAV_CLSID, UBX_NAV_HPPOSLLH_MSGID,    "UBX-NAV-HPPOSLLH") \
    _P_(UBX_NAV_CLSID, UBX_NAV_RELPOSNED_MSGID,   "UBX-NAV-RELPOSNED") \
    _P_(UBX_NAV_CLSID, UBX_NAV_VELECEF_MSGID,     "UBX-NAV-VELECEF") \
    _P_(UBX_NAV_CLSID, UBX_NAV_VELNED_MSGID,      "UBX-NAV-VELNED") \
    _P_(UBX_NAV_CLSID, UBX_NAV_SVIN_MSGID,        "UBX-NAV-SVIN") \
    _P_(UBX_NAV_CLSID, UBX_NAV_EOE_MSGID,         "UBX-NAV-EOE") \
    _P_(UBX_NAV_CLSID, UBX_NAV_GEOFENCE_MSGID,    "UBX-NAV-GEOFENCE") \
    _P_(UBX_NAV_CLSID, UBX_NAV_ODO_MSGID,         "UBX-NAV-ODO") \
    _P_(UBX_NAV_CLSID, UBX_NAV_RESETODO_MSGID,    "UBX-NAV-RESETODO") \
    _P_(UBX_NAV_CLSID, UBX_NAV_TIMEUTC_MSGID,     "UBX-NAV-TIMEUTC") \
    _P_(UBX_NAV_CLSID, UBX_NAV_TIMELS_MSGID,      "UBX-NAV-TIMELS") \
    _P_(UBX_NAV_CLSID, UBX_NAV_TIMEGPS_MSGID,     "UBX-NAV-TIMEGPS") \
    _P_(UBX_NAV_CLSID, UBX_NAV_TIMEGLO_MSGID,     "UBX-NAV-TIMEGLO") \
    _P_(UBX_NAV_CLSID, UBX_NAV_TIMEBDS_MSGID,     "UBX-NAV-TIMEBDS") \
    _P_(UBX_NAV_CLSID, UBX_NAV_TIMEGAL_MSGID,     "UBX-NAV-TIMEGAL") \
    _P_(UBX_NAV_CLSID, UBX_NAV_COV_MSGID,         "UBX-NAV-COV") \
    _P_(UBX_NAV_CLSID, UBX_NAV_EELL_MSGID,        "UBX-NAV-EELL") \
    _P_(UBX_NAV_CLSID, UBX_NAV_ATT_MSGID,         "UBX-NAV-ATT") \
    _P_(UBX_NAV_CLSID, UBX_NAV_PVAT_MSGID,        "UBX-NAV-PVAT") \
    _P_(UBX_RXM_CLSID, UBX_RXM_MEASX_MSGID,       "UBX-RXM-MEASX") \
    _P_(UBX_RXM_CLSID, UBX_RXM_RAWX_MSGID,        "UBX-RXM-RAWX") \
    _P_(UBX_RXM_CLSID, UBX_RXM_SFRBX_MSGID,       "UBX-RXM-SFRBX") \
    _P_(UBX_RXM_CLSID, UBX_RXM_PMREQ_MSGID,       "UBX-RXM-PMREQ") \
    _P_(UBX_RXM_CLSID, UBX_RXM_RLM_MSGID,         "UBX-RXM-RLM") \
    _P_(UBX_RXM_CLSID, UBX_RXM_RTCM_MSGID,        "UBX-RXM-RTCM") \
    _P_(UBX_RXM_CLSID, UBX_RXM_SPARTN_MSGID,      "UBX-RXM-SPARTN") \
    _P_(UBX_SEC_CLSID, UBX_SEC_UNIQUEID_MSGID,    "UBX-SEC-UNIQUEID") \
    _P_(UBX_TIM_CLSID, UBX_TIM_TM2_MSGID,         "UBX-TIM-TM2") \
    _P_(UBX_TIM_CLSID, UBX_TIM_TP_MSGID,          "UBX-TIM-TP") \
    _P_(UBX_TIM_CLSID, UBX_TIM_VRFY_MSGID,        "UBX-TIM-VRFY") \
    _P_(UBX_UPD_CLSID, UBX_UPD_SOS_MSGID,         "UBX-UPD-SOS") \
    _P_(UBX_UPD_CLSID, UBX_UPD_POS_MSGID,         "UBX-UPD-POS") \
    _P_(UBX_UPD_CLSID, UBX_UPD_SAFEBOOT_MSGID,    "UBX-UPD-SAFEBOOT") \
    _P_(UBX_SEC_CLSID, UBX_SEC_SIG_MSGID,         "UBX-SEC-SIG") \
    _P_(UBX_SEC_CLSID, UBX_SEC_UNIQID_MSGID,      "UBX-SEC-UNIQID")

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_GNSSID_NONE     0xff
#define UBX_GNSSID_GPS      0
#define UBX_GNSSID_SBAS     1
#define UBX_GNSSID_GAL      2
#define UBX_GNSSID_BDS      3
#define UBX_GNSSID_QZSS     5
#define UBX_GNSSID_GLO      6

#define UBX_SIGID_NONE      0xff
#define UBX_SIGID_GPS_L1CA  0
#define UBX_SIGID_GPS_L2CL  3
#define UBX_SIGID_GPS_L2CM  4
#define UBX_SIGID_SBAS_L1CA 0
#define UBX_SIGID_GAL_E1C   0
#define UBX_SIGID_GAL_E1B   1
#define UBX_SIGID_GAL_E5BI  5
#define UBX_SIGID_GAL_E5BQ  6
#define UBX_SIGID_BDS_B1ID1 0
#define UBX_SIGID_BDS_B1ID2 1
#define UBX_SIGID_BDS_B2ID1 2
#define UBX_SIGID_BDS_B2ID2 3
#define UBX_SIGID_QZSS_L1CA 0
#define UBX_SIGID_QZSS_L1S  1
#define UBX_SIGID_QZSS_L2CM 4
#define UBX_SIGID_QZSS_L2CL 5
#define UBX_SIGID_GLO_L1OF  0
#define UBX_SIGID_GLO_L2OF  2

#define UBX_NUM_GPS        32
#define UBX_NUM_SBAS       39
#define UBX_NUM_GAL        36
#define UBX_NUM_BDS        37
#define UBX_NUM_QZSS       10
#define UBX_NUM_GLO        32

#define UBX_FIRST_GPS       1
#define UBX_FIRST_SBAS    120
#define UBX_FIRST_GAL       1
#define UBX_FIRST_BDS       1
#define UBX_FIRST_QZSS      1
#define UBX_FIRST_GLO       1

/* ****************************************************************************************************************** */

#define UBX_CFG_VALSET_VERSION_GET(msg)    ( ((uint8_t *)(msg))[UBX_HEAD_SIZE + 0] )

//! UBX-CFG-VALSET (version 0, input) message payload header
typedef struct UBX_CFG_VALSET_V0_GROUP0_s
{
    uint8_t  version;                            //!< Message version (#UBX_CFG_VALSET_V1_VERSION)
    uint8_t  layers;                             //!< Configuration layers
    uint8_t  reserved[2];                        //!< Reserved (set to 0x00)
} UBX_CFG_VALSET_V0_GROUP0_t;

#define UBX_CFG_VALSET_V0_VERSION                0x00      //!< UBX-CFG-VALSET.version value
#define UBX_CFG_VALSET_V0_LAYER_RAM              0x01      //!< UBX-CFG-VALSET.layers flag: layer RAM
#define UBX_CFG_VALSET_V0_LAYER_BBR              0x02      //!< UBX-CFG-VALSET.layers flag: layer BBR
#define UBX_CFG_VALSET_V0_LAYER_FLASH            0x04      //!< UBX-CFG-VALSET.layers flag: layer Flash
#define UBX_CFG_VALSET_V0_RESERVED               0x00      //!< UBX-CFG-VALSET.reserved value
#define UBX_CFG_VALSET_V0_MAX_KV                 64        //!< UBX-CFG-VALSET.cfgData: maximum number of key-value pairs
#define UBX_CFG_VALSET_V0_CFGDATA_MAX (UBX_CFG_VALSET_V1_MAX_KV * (4 + 8)) //!< UBX-CFG-VALSET.cfgData maximum size

#define UBX_CFG_VALSET_V0_MAX_SIZE \
    ((int)(sizeof(UBX_CFG_VALSET_V0_GROUP0_t) + UBX_CFG_VALSET_V0_CFGDATA_MAX + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-CFG-VALSET (version 1, input) message payload header
typedef struct UBX_CFG_VALSET_V1_GROUP0_s
{
    uint8_t  version;                            //!< Message version (#UBX_CFG_VALSET_V1_VERSION)
    uint8_t  layers;                             //!< Configuration layers
    uint8_t  transaction;                        //!< Transaction mode
    uint8_t  reserved;                           //!< Reserved (set to 0x00)
} UBX_CFG_VALSET_V1_GROUP0_t;

#define UBX_CFG_VALSET_V1_VERSION                0x01      //!< UBX-CFG-VALSET.version value
#define UBX_CFG_VALSET_V1_LAYER_RAM              0x01      //!< UBX-CFG-VALSET.layers flag: layer RAM
#define UBX_CFG_VALSET_V1_LAYER_BBR              0x02      //!< UBX-CFG-VALSET.layers flag: layer BBR
#define UBX_CFG_VALSET_V1_LAYER_FLASH            0x04      //!< UBX-CFG-VALSET.layers flag: layer Flash
#define UBX_CFG_VALSET_V1_TRANSACTION_NONE       0         //!< UBX-CFG-VALSET.transaction value: no transaction
#define UBX_CFG_VALSET_V1_TRANSACTION_BEGIN      1         //!< UBX-CFG-VALSET.transaction value: transaction begin
#define UBX_CFG_VALSET_V1_TRANSACTION_CONTINUE   2         //!< UBX-CFG-VALSET.transaction value: transaction continue
#define UBX_CFG_VALSET_V1_TRANSACTION_END        3         //!< UBX-CFG-VALSET.transaction value: transaction: end
#define UBX_CFG_VALSET_V1_RESERVED               0x00      //!< UBX-CFG-VALSET.reserved value
#define UBX_CFG_VALSET_V1_MAX_KV                 64        //!< UBX-CFG-VALSET.cfgData: maximum number of key-value pairs
#define UBX_CFG_VALSET_V1_CFGDATA_MAX (UBX_CFG_VALSET_V1_MAX_KV * (4 + 8)) //!< UBX-CFG-VALSET.cfgData maximum size

#define UBX_CFG_VALSET_V1_MAX_SIZE \
    ((int)(sizeof(UBX_CFG_VALSET_V1_GROUP0_t) + UBX_CFG_VALSET_V1_CFGDATA_MAX + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_CFG_VALGET_VERSION_GET(msg)    ( ((uint8_t *)(msg))[UBX_HEAD_SIZE + 0] )

//! UBX-CFG-VALGET (version 0, poll) message payload header
typedef struct UBX_CFG_VALGET_V0_GROUP0_s
{
    uint8_t  version;                           //!< Message version (#UBX_CFG_VALGET_V0_VERSION)
    uint8_t  layer;                             //!< Configuration layer
    uint16_t position;                          //!< Number of values to skip in result set
} UBX_CFG_VALGET_V0_GROUP0_t;

#define UBX_CFG_VALGET_V0_VERSION                0x00      //!< UBX-CFG-VALGET.version value
#define UBX_CFG_VALGET_V0_LAYER_RAM              0         //!< UBX-CFG-VALGET.layers value: layer RAM
#define UBX_CFG_VALGET_V0_LAYER_BBR              1         //!< UBX-CFG-VALGET.layers value: layer BBR
#define UBX_CFG_VALGET_V0_LAYER_FLASH            2         //!< UBX-CFG-VALGET.layers value: layer Flash
#define UBX_CFG_VALGET_V0_LAYER_DEFAULT          7         //!< UBX-CFG-VALGET.layers value: layer Default
#define UBX_CFG_VALGET_V0_MAX_K                  64        //!< UBX-CFG-VALGET.cfgData maximum number of keys
#define UBX_CFG_VALGET_V0_KEYS_MAX (UBX_CFG_VALGET_V0_MAX_K * 4) //!< UBX-CFG-VALGET.keys maximum size

#define UBX_CFG_VALGET_V0_GROUP_WILDCARD(groupId)  ((groupId) | 0x0000ffff) //!< UBX-CFG-VALGET.keys group wildcard
#define UBX_CFG_VALGET_V0_ALL_WILDCARD                          0x0fffffff  //!< UBX-CFG-VALGET.keys all wildcard

#define UBX_CFG_VALGET_V0_MAX_SIZE \
    ((int)(sizeof(UBX_CFG_VALGET_V0_GROUP0_t) + UBX_CFG_VALGET_V0_KEYS_MAX + UBX_FRAME_SIZE))

//! UBX-CFG-VALGET (version 1, output) message payload header
typedef struct UBX_CFG_VALGET_V1_GROUP0_s
{
    uint8_t  version;                            //!< Message version (#UBX_CFG_VALGET_V1_VERSION)
    uint8_t  layer;                              //!< Configuration layer
    uint16_t position;                           //!< Number of values to skip in result set
} UBX_CFG_VALGET_V1_GROUP0_t;

#define UBX_CFG_VALGET_V1_VERSION                0x01      //!< UBX-CFG-VALGET.version value
#define UBX_CFG_VALGET_V1_LAYER_RAM              0         //!< UBX-CFG-VALGET.layers value: layer RAM
#define UBX_CFG_VALGET_V1_LAYER_BBR              1         //!< UBX-CFG-VALGET.layers value: layer BBR
#define UBX_CFG_VALGET_V1_LAYER_FLASH            2         //!< UBX-CFG-VALGET.layers value: layer Flash
#define UBX_CFG_VALGET_V1_LAYER_DEFAULT          7         //!< UBX-CFG-VALGET.layers value: layer Default
#define UBX_CFG_VALGET_V1_MAX_KV                 64        //!< UBX-CFG-VALGET.cfgData maximum number of key-value pairs
#define UBX_CFG_VALGET_V1_CFGDATA_MAX (UBX_CFG_VALGET_V1_MAX_KV * (4 + 8)) //!< UBX-CFG-VALGET.cfgData maximum size

#define UBX_CFG_VALGET_V1_MAX_SIZE \
    ((int)(sizeof(UBX_CFG_VALGET_V1_GROUP0_t) + UBX_CFG_VALGET_V1_CFGDATA_MAX + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_CFG_VALDEL_VERSION_GET(msg)    ( ((uint8_t *)(msg))[UBX_HEAD_SIZE + 0] )

//! UBX-CFG-VALDEL (version 1, input) message payload header
typedef struct UBX_CFG_VALDEL_V1_GROUP0_s
{
    uint8_t  version;                            //!< Message version (#UBX_CFG_VALGET_V1_VERSION)
    uint8_t  layers;                             //!< Configuration layers
    uint8_t  transaction;                        //!< Transaction mode
    uint8_t  reserved;                           //!< Reserved (set to 0x00)
} UBX_CFG_VALDEL_V1_GROUP0_t;

#define UBX_CFG_VALDEL_V1_VERSION                0x01      //!< UBX-CFG-VALDEL.version value
#define UBX_CFG_VALDEL_V1_LAYER_BBR              0x02      //!< UBX-CFG-VALDEL.layers flag: layer BBR
#define UBX_CFG_VALDEL_V1_LAYER_FLASH            0x04      //!< UBX-CFG-VALDEL.layers flag: layer Flash
#define UBX_CFG_VALDEL_V1_TRANSACTION_NONE       0         //!< UBX-CFG-VALDEL.transaction value: no transaction
#define UBX_CFG_VALDEL_V1_TRANSACTION_BEGIN      1         //!< UBX-CFG-VALDEL.transaction value: transaction begin
#define UBX_CFG_VALDEL_V1_TRANSACTION_CONTINUE   2         //!< UBX-CFG-VALDEL.transaction value: transaction continue
#define UBX_CFG_VALDEL_V1_TRANSACTION_END        3         //!< UBX-CFG-VALDEL.transaction value: transaction: end
#define UBX_CFG_VALDEL_V1_RESERVED               0x00      //!< UBX-CFG-VALDEL.reserved value
#define UBX_CFG_VALDEL_V1_MAX_K                  64        //!< UBX-CFG-VALSET.cfgData maximum number of key IDs
#define UBX_CFG_VALDEL_V1_KEYS_MAX (UBX_CFG_VALDEL_V1_MAX_K * 4) //!< UBX-CFG-VALDEL.keys maximum size

#define UBX_CFG_VALDEL_V1_MAX_SIZE \
    ((int)(sizeof(UBX_CFG_VALDEL_V1_GROUP0_t) + UBX_CFG_VALDEL_V1_KEYS_MAX + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-CFG-RST (version 0, command) message payload
typedef struct UBX_CFG_RST_V0_GROUP0_s
{
    uint16_t navBbrMask;                         //!< BBR sections to clear
    uint8_t  resetMode;                          //!< Reset type
    uint8_t  reserved;                           //!< Reserved
} UBX_CFG_RST_V0_GROUP0_t;

#define UBX_CFG_RST_V0_NAVBBR_NONE               0x0001    //!< Nothing
#define UBX_CFG_RST_V0_NAVBBR_EPH                0x0001    //!< Ephemeris
#define UBX_CFG_RST_V0_NAVBBR_ALM                0x0002    //!< Almanac
#define UBX_CFG_RST_V0_NAVBBR_HEALTH             0x0004    //!< Health
#define UBX_CFG_RST_V0_NAVBBR_KLOB               0x0008    //!< Klobuchar parameters
#define UBX_CFG_RST_V0_NAVBBR_POS                0x0010    //!< Position
#define UBX_CFG_RST_V0_NAVBBR_CLKD               0x0020    //!< Clock drift
#define UBX_CFG_RST_V0_NAVBBR_OSC                0x0040    //!< Oscillator parameters
#define UBX_CFG_RST_V0_NAVBBR_UTC                0x0080    //!< UTC and leap second parameters
#define UBX_CFG_RST_V0_NAVBBR_RTC                0x0100    //!< RTC
#define UBX_CFG_RST_V0_NAVBBR_AOP                0x8000    //!< AssistNow Autonomous
#define UBX_CFG_RST_V0_NAVBBR_HOTSTART           0x0000    //!< Hostsart (keep all data)
#define UBX_CFG_RST_V0_NAVBBR_WARMSTART          0x0001    //!< Warmstart (clear ephemerides)
#define UBX_CFG_RST_V0_NAVBBR_COLDSTART          0xffff    //!< Coldstart (erase all data)
#define UBX_CFG_RST_V0_RESETMODE_HW_FORCED       0x00      //!< Forced, immediate hardware reset
#define UBX_CFG_RST_V0_RESETMODE_SW              0x01      //!< Controlled software reset
#define UBX_CFG_RST_V0_RESETMODE_GNSS            0x02      //!< Restart GNSS
#define UBX_CFG_RST_V0_RESETMODE_HW_CONTROLLED   0x04      //!< Controlled hardware reset
#define UBX_CFG_RST_V0_RESETMODE_GNSS_STOP       0x08      //!< Stop GNSS
#define UBX_CFG_RST_V0_RESETMODE_GNSS_START      0x09      //!< Start GNSS
#define UBX_CFG_RST_V0_RESERVED                  0x00      //!< Reserved

#define UBX_CFG_RST_V0_SIZE    ((int)(sizeof(UBX_CFG_RST_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-CFG-CFG (version 0, command) message head
typedef struct UBX_CFG_CFG_V0_GROUP0_s
{
    uint32_t clearMask;
    uint32_t saveMask;
    uint32_t loadMask;
} UBX_CFG_CFG_V0_GROUP0_t;

//! UBX-CFG-CFG (version 0, command) message optional group
typedef struct UBX_CFG_CFG_V0_GROUP1_s
{
    uint8_t deviceMask;
} UBX_CFG_CFG_V0_GROUP1_t;

#define UBX_CFG_CFG_V0_CLEAR_NONE                0x00000000 //!< Clear no config
#define UBX_CFG_CFG_V0_CLEAR_ALL                 0xffffffff //!< Clear all config
#define UBX_CFG_CFG_V0_SAVE_NONE                 0x00000000 //!< Save no config
#define UBX_CFG_CFG_V0_SAVE_ALL                  0xffffffff //!< Save all config
#define UBX_CFG_CFG_V0_LOAD_NONE                 0x00000000 //!< Load no config
#define UBX_CFG_CFG_V0_LOAD_ALL                  0xffffffff //!< Load all config
#define UBX_CFG_CFG_V0_DEVICE_BBR                0x01      //!< Layer BBR
#define UBX_CFG_CFG_V0_DEVICE_FLASH              0x02      //!< Layer Flash

#define UBX_CFG_CFG_V0_MIN_SIZE    ((int)(sizeof(UBX_CFG_CFG_V0_GROUP0_t) + UBX_FRAME_SIZE))
#define UBX_CFG_CFG_V0_MAX_SIZE    (UBX_CFG_CFG_V0_MIN_SIZE + (int)sizeof(UBX_CFG_CFG_V0_GROUP1_t))

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-MON-VER (version 0, output) message payload header
typedef struct UBX_MON_VER_V0_GROUP0_s
{
    char swVersion[30];
    char hwVersion[10];
} UBX_MON_VER_V0_GROUP0_t;

//! UBX-MON-VER (version 0, output) optional repeated field
typedef struct UBX_MON_VER_V0_GROUP1_s
{
    char extension[30];
} UBX_MON_VER_V0_GROUP1_t;

#define UBX_MON_VER_V0_MIN_SIZE    ((int)(sizeof(UBX_MON_VER_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

typedef struct UBX_MON_HW_V0_GROUP0_s
{
    uint32_t pinSel;
    uint32_t pinBank;
    uint32_t pinDir;
    uint32_t pinVal;
    uint16_t noisePerMS;
    uint16_t agcCnt;
    uint8_t  aStatus;
    uint8_t  aPower;
    uint8_t  flags;
    uint8_t  reserved0;
    uint32_t usedMask;
    uint8_t  VP[17];
    uint8_t  jamInd;
    uint8_t  reserved1[2];
    uint32_t pinIrq;
    uint32_t pullH;
    uint32_t pullL;
} UBX_MON_HW_V0_GROUP0_t;

#define UBX_MON_HW_V0_FLAGS_RTCCALIB                  0x01
#define UBX_MON_HW_V0_FLAGS_SAFEBOOT                  0x02
#define UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_GET(f)       ( ((int8_t)(f) >> 2) & 0x03 )
#define UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_UNKNOWN      0x00
#define UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_OK           0x01
#define UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_WARNING      0x02
#define UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_CRITICAL     0x03
#define UBX_MON_HW_V0_FLAGS_XTALABSENT                0x10
#define UBX_MON_HW_V0_ASTATUS_INIT                    0x00
#define UBX_MON_HW_V0_ASTATUS_UNKNOWN                 0x01
#define UBX_MON_HW_V0_ASTATUS_OK                      0x02
#define UBX_MON_HW_V0_ASTATUS_SHORT                   0x03
#define UBX_MON_HW_V0_ASTATUS_OPEN                    0x04
#define UBX_MON_HW_V0_APOWER_OFF                      0x00
#define UBX_MON_HW_V0_APOWER_ON                       0x01
#define UBX_MON_HW_V0_APOWER_UNKNOWN                  0x02
#define UBX_MON_HW_V0_NOISEPERMS_MAX                  200 // This seems to be what u-center uses..
#define UBX_MON_HW_V0_AGCCNT_MAX                      8191
#define UBX_MON_HW_V0_JAMIND_MAX                      255

#define UBX_MON_HW_V0_SIZE    ((int)(sizeof(UBX_MON_HW_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

typedef struct UBX_MON_HW2_V0_GROUP0_s
{
    int8_t   ofsI;
    uint8_t  magI;
    int8_t   ofsQ;
    uint8_t  magQ;
    uint8_t  cfgSource;
    uint8_t  reserved0[3];
    uint32_t lowLevCfg;
    uint8_t  reserved1[8];
    uint32_t postStatus;
    uint8_t  reserved2[3];
} UBX_MON_HW2_V0_GROUP0_t;

#define UBX_MON_HW2_V0_CFGSOURCE_ROM                  114
#define UBX_MON_HW2_V0_CFGSOURCE_OTP                  111
#define UBX_MON_HW2_V0_CFGSOURCE_PIN                  112
#define UBX_MON_HW2_V0_CFGSOURCE_FLASH                102

#define UBX_MON_HW2_V0_SIZE    ((int)(sizeof(UBX_MON_HW2_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_MON_HW3_VERSION_GET(msg)    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 0])

typedef struct UBX_MON_HW3_V0_GROUP0_s
{
    uint8_t  version;
    uint8_t  nPins;
    uint8_t  flags;
    char     hwVersion[10];
    uint8_t  reserved0[9];
} UBX_MON_HW3_V0_GROUP0_t;

typedef struct UBX_MON_HW3_V0_GROUP1_s
{
    uint16_t pinId;   // u-center shows ((pinId & 0xff00) >> 8) | ((pinId & 0x00ff) << 8), seems to be: (pinNo << 8) | pinBank
    uint16_t pinMask;
    uint8_t  VP;
    uint8_t  reserved1;
} UBX_MON_HW3_V0_GROUP1_t;

#define UBX_MON_HW3_V0_VERSION                        0x00
#define UBX_MON_HW3_V0_FLAGS_RTCCALIB                 0x01
#define UBX_MON_HW3_V0_FLAGS_SAFEBOOT                 0x02
#define UBX_MON_HW3_V0_FLAGS_XTALABSENT               0x04
#define UBX_MON_HW3_V0_PINMASK_PERIPHPIO              0x01
#define UBX_MON_HW3_V0_PINMASK_PINBANK_GET(f)         ( ((int16_t)(f) >> 1) & 0x07 )
#define UBX_MON_HW3_V0_PINMASK_PINBANK_A              0
#define UBX_MON_HW3_V0_PINMASK_PINBANK_B              1
#define UBX_MON_HW3_V0_PINMASK_PINBANK_C              2
#define UBX_MON_HW3_V0_PINMASK_PINBANK_D              3
#define UBX_MON_HW3_V0_PINMASK_PINBANK_E              4
#define UBX_MON_HW3_V0_PINMASK_PINBANK_F              5
#define UBX_MON_HW3_V0_PINMASK_PINBANK_G              6
#define UBX_MON_HW3_V0_PINMASK_PINBANK_H              7
#define UBX_MON_HW3_V0_PINMASK_DIRECTION              0x0010
#define UBX_MON_HW3_V0_PINMASK_VALUE                  0x0020
#define UBX_MON_HW3_V0_PINMASK_VPMANAGER              0x0040
#define UBX_MON_HW3_V0_PINMASK_PIOIRQ                 0x0080
#define UBX_MON_HW3_V0_PINMASK_PIOPULLHIGH            0x0100
#define UBX_MON_HW3_V0_PINMASK_PIOPULLLOW             0x0200

#define UBX_MON_HW3_V0_MIN_SIZE    ((int)(sizeof(UBX_MON_HW3_V0_GROUP0_t) + UBX_FRAME_SIZE))
#define UBX_MON_HW3_V0_SIZE(msg) \
    ((int)(sizeof(UBX_MON_HW3_V0_GROUP0_t) + UBX_FRAME_SIZE + \
    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 1] * sizeof(UBX_MON_HW3_V0_GROUP1_t))))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_MON_TEMP_VERSION_GET(msg)    (((int8_t *)(msg))[UBX_HEAD_SIZE + 0])

// from u-center...
typedef struct UBX_MON_TEMP_V0_GROUP0_s
{
    uint8_t  version;  // probably..
    uint8_t  reserved0[3];
    int16_t  temperature;
    uint8_t  unknown;  // unit? 1 = C?
    uint8_t  reserved1[5];
} UBX_MON_TEMP_V0_GROUP0_t;

#define UBX_MON_TEMP_V0_VERSION                       0x00

#define UBX_MON_TEMP_V0_SIZE    ((int)(sizeof(UBX_MON_TEMP_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-ACK-ACK (version 0, output) payload
typedef struct UBX_ACK_ACK_V0_GROUP0_s
{
    uint8_t clsId;                                    //!< Class ID of ack'ed message
    uint8_t msgId;                                    //!< Message ID of ack'ed message
} UBX_ACK_ACK_V0_GROUP0_t;

#define UBX_ACK_ACK_V0_SIZE    ((int)(sizeof(UBX_ACK_ACK_V0_GROUP0_t) + UBX_FRAME_SIZE))

//! UBX-ACK-NCK (version 0, output) payload
typedef struct UBX_ACK_NAK_V0_GROUP0_s
{
    uint8_t clsId;                                    //!< Class ID of not-ack'ed message
    uint8_t msgId;                                    //!< Message ID of not-ack'ed message
} UBX_ACK_NAK_V0_GROUP0_t;

#define UBX_ACK_NAK_V0_SIZE    ((int)(sizeof(UBX_ACK_NAK_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_NAV_COV_VERSION_GET(msg)    (((int8_t *)(msg))[UBX_HEAD_SIZE + sizeof(uint32_t)])

//! UBX-NAV-COV (version 0, output) payload head
typedef struct UBX_NAV_COV_V0_GROUP0_s
{
    uint32_t iTOW;
    uint8_t  version;
    uint8_t  posCovValid;
    uint8_t  velCovValid;
    uint8_t  reserved[9];
    float    posCovNN;
    float    posCovNE;
    float    posCovND;
    float    posCovEE;
    float    posCovED;
    float    posCovDD;
    float    velCovNN;
    float    velCovNE;
    float    velCovND;
    float    velCovEE;
    float    velCovED;
    float    velCovDD;
} UBX_NAV_COV_V0_GROUP0_t;

#define UBX_NAV_COV_V0_VERSION                        0x00
#define UBX_NAV_COV_V0_ITOW_SCALE                     1e-3

#define UBX_NAV_COV_V0_SIZE    ((int)(sizeof(UBX_NAV_COV_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define  UBX_NAV_EELL_VERSION_GET(msg)    (((int8_t *)(msg))[UBX_HEAD_SIZE + sizeof(uint32_t)])

//! UBX-NAV-EELL (version 0, output) payload head
typedef struct UBX_NAV_EELL_V0_GROUP0_s
{
    uint32_t iTOW;
    uint8_t  version;
    uint8_t  reserved;
    uint16_t errEllipseOrient;
    uint32_t errEllipseMajor;
    uint32_t errEllipseMinor;
} UBX_NAV_EELL_V0_GROUP0_t;

#define  UBX_NAV_EELL_V0_VERSION                      0x00
#define  UBX_NAV_EELL_V0_ITOW_SCALE                   1e-3
#define  UBX_NAV_EELL_V0_ELLIPSEORIENT_SCALE          1e-2
#define  UBX_NAV_EELL_V0_ELLIPSEMAJOR_SCALE           1e-3
#define  UBX_NAV_EELL_V0_ELLIPSEMINOR_SCALE           1e-3

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-NAV-EOE (version 0, output) payload
typedef struct UBX_NAV_EOE_V0_GROUP0_s
{
    uint32_t iTOW;
} UBX_NAV_EOE_V0_GROUP0_t;

#define UBX_NAV_EOE_V0_ITOW_SCALE                     1e-3

#define UBX_NAV_EOE_V0_SIZE ((int)(sizeof(UBX_NAV_EOE_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_NAV_HPPOSECEF_VERSION_GET(msg)    (((int8_t *)(msg))[UBX_HEAD_SIZE + 0])

//! UBX-NAV-HPPOSECEF (version 0, output) payload
typedef struct UBX_NAV_HPPOSECEF_V0_GROUP0_s
{
    uint8_t  version;
    uint8_t  reserved[3];
    uint32_t iTOW;
    int32_t  ecefX;
    int32_t  ecefY;
    int32_t  ecefZ;
    int8_t   ecefXHp;
    int8_t   ecefYHp;
    int8_t   ecefZHp;
    uint8_t  flags;
    uint32_t pAcc;
} UBX_NAV_HPPOSECEF_V0_GROUP0_t;

#define UBX_NAV_HPPOSECEF_V0_VERSION                  0x00
#define UBX_NAV_HPPOSECEF_V0_ITOW_SCALE               1e-3
#define UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_SCALE           1e-2
#define UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_HP_SCALE        1e-4
#define UBX_NAV_HPPOSECEF_V0_PACC_SCALE               1e-4
#define UBX_NAV_HPPOSECEF_V0_FLAGS_INVALIDECEF        0x01

#define UBX_NAV_HPPOSECEF_V0_SIZE ((int)(sizeof(UBX_NAV_HPPOSECEF_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_NAV_HPPOSLLH_VERSION_GET(msg)    (((int8_t *)(msg))[UBX_HEAD_SIZE + 0])

//! UBX-NAV-HPPOSLLH (version 0) payload
typedef struct UBX_NAV_HPPOSLLH_V0_GROUP0_s
{
    uint8_t  version;
    uint8_t  reserved[2];
    uint8_t  flags;
    uint32_t iTOW;
    int32_t  lon;
    int32_t  lat;
    int32_t  height;
    int32_t  hMSL;
    int8_t   lonHp;
    int8_t   latHp;
    int8_t   heightHp;
    int8_t   hMSLHp;
    uint32_t hAcc;
    uint32_t vAcc;
} UBX_NAV_HPPOSLLH_V0_GROUP0_t;

#define UBX_NAV_HPPOSLLH_V0_VERSION                   0x00
#define UBX_NAV_HPPOSLLH_V0_FLAGS_INVALIDLLH          0x01
#define UBX_NAV_HPPOSLLH_V0_ITOW_SCALE                1e-3
#define UBX_NAV_HPPOSLLH_V0_LL_SCALE                  1e-7
#define UBX_NAV_HPPOSLLH_V0_H_SCALE                   1e-3
#define UBX_NAV_HPPOSLLH_V0_LL_HP_SCALE               1e-9
#define UBX_NAV_HPPOSLLH_V0_H_HP_SCALE                1e-4
#define UBX_NAV_HPPOSLLH_V0_ACC_SCALE                 1e-4

#define UBX_NAV_HPPOSLLH_V0_SIZE    ((int)(sizeof(UBX_NAV_HPPOSLLH_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-NAV-POSECEF (version 0, output) payload
typedef struct UBX_NAV_POSECEF_V0_GROUP0_s
{
    uint32_t iTOW;
    int32_t  ecefX;
    int32_t  ecefY;
    int32_t  ecefZ;
    uint32_t pAcc;
} UBX_NAV_POSECEF_V0_GROUP0_t;

#define UBX_NAV_POSECEF_V0_ECEF_XYZ_SCALE             1e-2
#define UBX_NAV_POSECEF_V0_PACC_SCALE                 1e-2

#define UBX_NAV_POSECEF_V0_SIZE    ((int)(sizeof(UBX_NAV_POSECEF_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-NAV-PVT (version 1, output) payload
typedef struct UBX_NAV_PVT_V1_GROUP0_s
{
    uint32_t iTOW;
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint8_t  valid;
    uint32_t tAcc;
    int32_t  nano;
    uint8_t  fixType;
    uint8_t  flags;
    uint8_t  flags2;
    uint8_t  numSV;
    int32_t  lon;
    int32_t  lat;
    int32_t  height;
    int32_t  hMSL;
    uint32_t hAcc;
    uint32_t vAcc;
    int32_t  velN;
    int32_t  velE;
    int32_t  velD;
    int32_t  gSpeed;
    int32_t  headMot;
    uint32_t sAcc;
    uint32_t headAcc;
    uint16_t pDOP;
    uint8_t  flags3;
    uint8_t  reserved[5];
    int32_t  headVeh;
    int16_t  magDec;
    uint16_t magAcc;
} UBX_NAV_PVT_V1_GROUP0_t;

#define UBX_NAV_PVT_V1_ITOW_SCALE                     1e-3
#define UBX_NAV_PVT_V1_VALID_VALIDDATE                0x01
#define UBX_NAV_PVT_V1_VALID_VALIDTIME                0x02
#define UBX_NAV_PVT_V1_VALID_FULLYRESOLVED            0x04
#define UBX_NAV_PVT_V1_VALID_VALIDMAG                 0x08
#define UBX_NAV_PVT_V1_FIXTYPE_NOFIX                  0
#define UBX_NAV_PVT_V1_FIXTYPE_DRONLY                 1
#define UBX_NAV_PVT_V1_FIXTYPE_2D                     2
#define UBX_NAV_PVT_V1_FIXTYPE_3D                     3
#define UBX_NAV_PVT_V1_FIXTYPE_3D_DR                  4
#define UBX_NAV_PVT_V1_FIXTYPE_TIME                   5
#define UBX_NAV_PVT_V1_FLAGS_GNSSFIXOK                0x01
#define UBX_NAV_PVT_V1_FLAGS_DIFFSOLN                 0x02
#define UBX_NAV_PVT_V1_FLAGS_CARRSOLN_GET(f)          ( ((int8_t)(f) >> 6) & 0x03 )
#define UBX_NAV_PVT_V1_FLAGS_CARRSOLN_NO              0
#define UBX_NAV_PVT_V1_FLAGS_CARRSOLN_FLOAT           1
#define UBX_NAV_PVT_V1_FLAGS_CARRSOLN_FIXED           2
#define UBX_NAV_PVT_V1_FLAGS2_CONFAVAIL               0x20
#define UBX_NAV_PVT_V1_FLAGS2_CONFDATE                0x40
#define UBX_NAV_PVT_V1_FLAGS2_CONFTIME                0x80
#define UBX_NAV_PVT_V1_FLAGS3_INVALIDLLH              0x01
#define UBX_NAV_PVT_V1_LAT_SCALE                      1e-7
#define UBX_NAV_PVT_V1_LON_SCALE                      1e-7
#define UBX_NAV_PVT_V1_HEIGHT_SCALE                   1e-3
#define UBX_NAV_PVT_V1_HACC_SCALE                     1e-3
#define UBX_NAV_PVT_V1_VACC_SCALE                     1e-3
#define UBX_NAV_PVT_V1_VELNED_SCALE                   1e-3
#define UBX_NAV_PVT_V1_GSPEED_SCALE                   1e-3
#define UBX_NAV_PVT_V1_HEADMOT_SCALE                  1e-5
#define UBX_NAV_PVT_V1_SACC_SCALE                     1e-3
#define UBX_NAV_PVT_V1_HEADACC_SCALE                  1e-5
#define UBX_NAV_PVT_V1_PDOP_SCALE                     1e-2
#define UBX_NAV_PVT_V1_TACC_SCALE                     1e-9
#define UBX_NAV_PVT_V1_NANO_SCALE                     1e-9

#define UBX_NAV_PVT_V1_SIZE    ((int)(sizeof(UBX_NAV_PVT_V1_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_NAV_ATT_VERSION_GET(msg)    (((uint8_t *)(msg))[UBX_HEAD_SIZE + sizeof(uint32_t)])

//! UBX-NAV-ATT (version 0, output) payload
typedef struct UBX_NAV_ATT_V0_GROUP0_s
{
    uint32_t iTOW;
    uint8_t  version;
    uint8_t  reserved0[3];
    int32_t  roll;
    int32_t  pitch;
    int32_t  heading;
    uint32_t accRoll;
    uint32_t accPitch;
    uint32_t accHeading;
} UBX_NAV_ATT_V0_GROUP0_t;

#define UBX_NAV_ATT_V0_VERSION                        0x00
#define UBX_NAV_ATT_V0_ITOW_SCALE                     1e-3
#define UBX_NAV_ATT_V0_RPH_SCALING                    1e-5

#define UBX_NAV_ATT_V0_SIZE    ((int)(sizeof(UBX_NAV_ATT_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_NAV_RELPOSNED_VERSION_GET(msg)    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 0])

//! UBX-NAV-RELPOSNED (version 1, output) payload
typedef struct UBX_NAV_RELPOSNED_V1_GROUP0_s
{
    uint8_t  version;
    uint8_t  reserved0;
    uint16_t refStationId;
    uint32_t iTOW;
    int32_t  relPosN;
    int32_t  relPosE;
    int32_t  relPosD;
    int32_t  relPosLength;
    int32_t  relPosHeading;
    uint8_t  reserved1[4];
    int8_t   relPosHPN;
    int8_t   relPosHPE;
    int8_t   relPosHPD;
    int8_t   relPosHPLength;
    uint32_t accN;
    uint32_t accE;
    uint32_t accD;
    uint32_t accLength;
    uint32_t accHeading;
    uint8_t  reserved2[4];
    uint32_t flags;
} UBX_NAV_RELPOSNED_V1_GROUP0_t;

#define UBX_NAV_RELPOSNED_V1_VERSION                  0x01
#define UBX_NAV_RELPOSNED_V1_ITOW_SCALE               1e-3
#define UBX_NAV_RELPOSNED_V1_RELPOSN_E_D_SCALE        1e-2
#define UBX_NAV_RELPOSNED_V1_RELPOSLENGTH_SCALE       1e-2
#define UBX_NAV_RELPOSNED_V1_RELPOSHEADING_SCALE      1e-5
#define UBX_NAV_RELPOSNED_V1_RELPOSHPN_E_D_SCALE      1e-4
#define UBX_NAV_RELPOSNED_V1_RELPOSHPLENGTH_SCALE     1e-4
#define UBX_NAV_RELPOSNED_V1_ACCN_E_D_SCALE           1e-4
#define UBX_NAV_RELPOSNED_V1_ACCLENGTH_SCALE          1e-4
#define UBX_NAV_RELPOSNED_V1_ACCHEADING_SCALE         1e-5
#define UBX_NAV_RELPOSNED_V1_FLAGS_GNSSFIXOK          0x0001
#define UBX_NAV_RELPOSNED_V1_FLAGS_DIFFSOLN           0x0002
#define UBX_NAV_RELPOSNED_V1_FLAGS_RELPOSVALID        0x0004
#define UBX_NAV_RELPOSNED_V1_FLAGS_CARRSOLN_GET(f)    ( ((int32_t)(f) >> 3) & 0x03 )
#define UBX_NAV_RELPOSNED_V1_FLAGS_CARRSOLN_NO        0
#define UBX_NAV_RELPOSNED_V1_FLAGS_CARRSOLN_FLOAT     1
#define UBX_NAV_RELPOSNED_V1_FLAGS_CARRSOLN_FIXED     2
#define UBX_NAV_RELPOSNED_V1_FLAGS_ISMOVING           0x0020
#define UBX_NAV_RELPOSNED_V1_FLAGS_REFPOSMISS         0x0040
#define UBX_NAV_RELPOSNED_V1_FLAGS_REFOBSMISS         0x0080
#define UBX_NAV_RELPOSNED_V1_FLAGS_RELPOSHEADINGVALID 0x0100
#define UBX_NAV_RELPOSNED_V1_FLAGS_RELPOSNORMALIZED   0x0200

#define UBX_NAV_RELPOSNED_V1_SIZE    ((int)(sizeof(UBX_NAV_RELPOSNED_V1_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-NAV-STATUS payload
typedef struct UBX_NAV_STATUS_V0_GROUP0_s
{
    uint32_t iTow;
    uint8_t  gpsFix;
    uint8_t  flags;
    uint8_t  fixStat;
    uint8_t  flags2;
    uint32_t ttff;
    uint32_t msss;
} UBX_NAV_STATUS_V0_GROUP0_t;

#define UBX_NAV_STATUS_V0_ITOW_SCALE                  1e-3
#define UBX_NAV_STATUS_V0_FIXTYPE_NOFIX               0
#define UBX_NAV_STATUS_V0_FIXTYPE_DRONLY              1
#define UBX_NAV_STATUS_V0_FIXTYPE_2D                  2
#define UBX_NAV_STATUS_V0_FIXTYPE_3D                  3
#define UBX_NAV_STATUS_V0_FIXTYPE_3D_DR               4
#define UBX_NAV_STATUS_V0_FIXTYPE_TIME                5
#define UBX_NAV_STATUS_V0_FLAGS_GPSFIXOK              0x01
#define UBX_NAV_STATUS_V0_FLAGS_DIFFSOLN              0x02
#define UBX_NAV_STATUS_V0_FLAGS_WKNSET                0x04
#define UBX_NAV_STATUS_V0_FLAGS_TOWSET                0x08
#define UBX_NAV_STATUS_V0_FIXSTAT_DIFFCORR            0x01
#define UBX_NAV_STATUS_V0_FIXSTAT_CARRSOLNVALID       0x02
#define UBX_NAV_STATUS_V0_FLAGS2_CARRSOLN_GET(f)      ( ((int8_t)(f) >> 6) & 0x03 )
#define UBX_NAV_STATUS_V0_FLAGS2_CARRSOLN_NO          0
#define UBX_NAV_STATUS_V0_FLAGS2_CARRSOLN_FLOAT       1
#define UBX_NAV_STATUS_V0_FLAGS2_CARRSOLN_FIXED       2
#define UBX_NAV_STATUS_V0_TTFF_SCALE                  1e-3
#define UBX_NAV_STATUS_V0_MSSS_SCALE                  1e-3

#define UBX_NAV_STATUS_V0_SIZE    ((int)(sizeof(UBX_NAV_STATUS_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-NAV-TIMEGPS payload
typedef struct UBX_NAV_TIMEGPS_V0_GROUP0_s
{
    uint32_t iTow;
    int32_t  fTOW;
    int16_t  week;
    int8_t   leapS;
    uint8_t  valid;
    uint32_t tAcc;
} UBX_NAV_TIMEGPS_V0_GROUP0_t;

#define UBX_NAV_TIMEGPS_V0_ITOW_SCALE                 1e-3
#define UBX_NAV_TIMEGPS_V0_FTOW_SCALE                 1e-9
#define UBX_NAV_TIMEGPS_V0_TACC_SCALE                 1e-9
#define UBX_NAV_TIMEGPS_V0_VALID_TOWVALID             0x01
#define UBX_NAV_TIMEGPS_V0_VALID_WEEKVALID            0x02
#define UBX_NAV_TIMEGPS_V0_VALID_LEAPSVALID           0x04

#define UBX_NAV_TIMEGPS_V0_SIZE    ((int)(sizeof(UBX_NAV_TIMEGPS_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-NAV-TIMEGPS payload
typedef struct UBX_NAV_TIMELS_V0_GROUP0_s
{
    uint32_t iTOW;
    uint8_t  version;
    uint8_t  reserved0[3];
    uint8_t  srcOfCurrLs;
    int8_t   currLs;
    uint8_t  srcOfLsChange;
    int8_t   lsChange;
    int32_t  timeToLsEvent;
    uint16_t dateOfLsGpsWn;
    uint16_t dateOfLsGpsDn;
    uint8_t  reserved1[3];
    uint8_t  valid;
} UBX_NAV_TIMELS_V0_GROUP0_t;

#define UBX_NAV_TIMELS_V0_ITOW_SCALE                  1e-3
#define UBX_NAV_TIMELS_V0_SRCOFCURRLS_DEFAULT         0
#define UBX_NAV_TIMELS_V0_SRCOFCURRLS_GPSGLO          1
#define UBX_NAV_TIMELS_V0_SRCOFCURRLS_GPS             2
#define UBX_NAV_TIMELS_V0_SRCOFCURRLS_SBAS            3
#define UBX_NAV_TIMELS_V0_SRCOFCURRLS_BDS             4
#define UBX_NAV_TIMELS_V0_SRCOFCURRLS_GAL             5
#define UBX_NAV_TIMELS_V0_SRCOFCURRLS_CONFIG          7
#define UBX_NAV_TIMELS_V0_SRCOFCURRLS_UNKNOWN         255
#define UBX_NAV_TIMELS_V0_SRCOFCURRLSCHANGE_NONE      0
#define UBX_NAV_TIMELS_V0_SRCOFCURRLSCHANGE_GPS       2
#define UBX_NAV_TIMELS_V0_SRCOFCURRLSCHANGE_SBAS      3
#define UBX_NAV_TIMELS_V0_SRCOFCURRLSCHANGE_BDS       4
#define UBX_NAV_TIMELS_V0_SRCOFCURRLSCHANGE_GAL       5
#define UBX_NAV_TIMELS_V0_SRCOFCURRLSCHANGE_GLO       6
#define UBX_NAV_TIMELS_V0_VALID_CURRLSVALID           0x01
#define UBX_NAV_TIMELS_V0_VALID_TIMETOLSEVENTVALID    0x02

#define UBX_NAV_TIMELS_V0_SIZE    ((int)(sizeof(UBX_NAV_TIMELS_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_NAV_SAT_VERSION_GET(msg)    (((uint8_t *)(msg))[UBX_HEAD_SIZE + sizeof(uint32_t)])

//! UBX-NAV-SAT (version 1, output) payload head
typedef struct UBX_NAV_SAT_V1_GROUP0_s
{
    uint32_t iTOW;
    uint8_t  version;
    uint8_t  numSvs;
    uint8_t  reserved[2];
} UBX_NAV_SAT_V1_GROUP0_t;

//! UBX-NAV-SAT (version 1, output) payload repeated group
typedef struct UBX_NAV_SAT_V1_GROUP1_s
{
    uint8_t  gnssId;
    uint8_t  svId;
    uint8_t  cno;
    int8_t   elev;
    int16_t  azim;
    int16_t  prRes;
    uint32_t flags;
} UBX_NAV_SAT_V1_GROUP1_t;

#define UBX_NAV_SAT_V1_VERSION                        0x01
#define UBX_NAV_SAT_V1_ITOW_SCALE                     1e-3
// Note: only those flags relevant for a SV are defined below. All other info should be taken from UBX-NAV-SIG.
#define UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_GET(f)       ( ((int32_t)(f) >> 8) & 0x00000007 )
#define UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_NONE         0
#define UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_EPH          1
#define UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_ALM          2
#define UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_ANO          3
#define UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_ANA          4
#define UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_OTHER1       5
#define UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_OTHER2       6
#define UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_OTHER3       7
#define UBX_NAV_SAT_V1_FLAGS_EPHAVAIL                 0x00000800
#define UBX_NAV_SAT_V1_FLAGS_ALMAVAIL                 0x00001000
#define UBX_NAV_SAT_V1_FLAGS_ANOAVAIL                 0x00002000
#define UBX_NAV_SAT_V1_FLAGS_AOPAVAIL                 0x00004000

#define UBX_NAV_SAT_V1_MIN_SIZE    ((int)(sizeof(UBX_NAV_SAT_V1_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_NAV_SIG_VERSION_GET(msg)    (((uint8_t *)(msg))[UBX_HEAD_SIZE + sizeof(uint32_t)])

//! UBX-NAV-SIG (version 0, output) payload head
typedef struct UBX_NAV_SIG_V0_GROUP0_s
{
    uint32_t iTOW;
    uint8_t  version;
    uint8_t  numSigs;
    uint8_t  reserved[2];
} UBX_NAV_SIG_V0_GROUP0_t;

//! UBX-NAV-SIG (version 0, output) payload repeated group
typedef struct UBX_NAV_SIG_V0_GROUP1_s
{
    uint8_t  gnssId;
    uint8_t  svId;
    uint8_t  sigId;
    uint8_t  freqId;
    int16_t  prRes;
    uint8_t  cno;
    uint8_t  qualityInd;
    uint8_t  corrSource;
    uint8_t  ionoModel;
    uint16_t sigFlags;
    uint8_t  reserved[4];
} UBX_NAV_SIG_V0_GROUP1_t;

#define UBX_NAV_SIG_V0_VERSION                        0x00
#define UBX_NAV_SIG_V0_ITOW_SCALE                     1e-3
#define UBX_NAV_SIG_V0_FREQID_OFFS                    7
#define UBX_NAV_SIG_V0_QUALITYIND_NOSIG               0
#define UBX_NAV_SIG_V0_QUALITYIND_SEARCH              1
#define UBX_NAV_SIG_V0_QUALITYIND_ACQUIRED            2
#define UBX_NAV_SIG_V0_QUALITYIND_UNUSED              3
#define UBX_NAV_SIG_V0_QUALITYIND_CODELOCK            4
#define UBX_NAV_SIG_V0_QUALITYIND_CARRLOCK1           5
#define UBX_NAV_SIG_V0_QUALITYIND_CARRLOCK2           6
#define UBX_NAV_SIG_V0_QUALITYIND_CARRLOCK3           7
#define UBX_NAV_SIG_V0_IONOMODEL_NONE                 0
#define UBX_NAV_SIG_V0_IONOMODEL_KLOB_GPS             1
#define UBX_NAV_SIG_V0_IONOMODEL_SBAS                 2
#define UBX_NAV_SIG_V0_IONOMODEL_KLOB_BDS             3
#define UBX_NAV_SIG_V0_IONOMODEL_DUALFREQ             8
#define UBX_NAV_SIG_V0_SIGFLAGS_HEALTH_GET(f)         ( (int16_t)(f) & 0x03 )
#define UBX_NAV_SIG_V0_SIGFLAGS_HEALTH_UNKNO          0
#define UBX_NAV_SIG_V0_SIGFLAGS_HEALTH_HEALTHY        1
#define UBX_NAV_SIG_V0_SIGFLAGS_HEALTH_UNHEALTHY      2
#define UBX_NAV_SIG_V0_SIGFLAGS_PR_SMOOTHED           0x0004
#define UBX_NAV_SIG_V0_SIGFLAGS_PR_USED               0x0008
#define UBX_NAV_SIG_V0_SIGFLAGS_CR_USED               0x0010
#define UBX_NAV_SIG_V0_SIGFLAGS_DO_USED               0x0020
#define UBX_NAV_SIG_V0_SIGFLAGS_PR_CORR_USED          0x0040
#define UBX_NAV_SIG_V0_SIGFLAGS_CR_CORR_USED          0x0080
#define UBX_NAV_SIG_V0_SIGFLAGS_DO_CORR_USED          0x0100
#define UBX_NAV_SIG_V0_CORRUSED_NONE                  0
#define UBX_NAV_SIG_V0_CORRUSED_SBAS                  1
#define UBX_NAV_SIG_V0_CORRUSED_BDS                   2
#define UBX_NAV_SIG_V0_CORRUSED_RTCM2                 3
#define UBX_NAV_SIG_V0_CORRUSED_RTCM3_OSR             4
#define UBX_NAV_SIG_V0_CORRUSED_RTCM3_SSR             5
#define UBX_NAV_SIG_V0_CORRUSED_QZSS_SLAS             6
#define UBX_NAV_SIG_V0_CORRUSED_SPARTN                7
#define UBX_NAV_SIG_V0_PRRES_SCALE                    1e-1

#define UBX_NAV_SIG_V0_MIN_SIZE    ((int)(sizeof(UBX_NAV_SIG_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

//! UBX-NAV-VELECEF payload
typedef struct UBX_NAV_VELECEF_V0_GROUP0_s
{
    uint32_t iTOW;
    int32_t  ecefVX;
    int32_t  ecefVY;
    int32_t  ecefVZ;
    uint32_t sAcc;
} UBX_NAV_VELECEF_V0_GROUP0_t;

#define UBX_NAV_VELECEF_V0_ITOW_SCALE                 1e-3
#define UBX_NAV_VELECEF_V0_ECEF_XYZ_SCALE             1e-2
#define UBX_NAV_VELECEF_V0_SACC_SCALE                 1e-2

#define UBX_NAV_VELECEF_V0_SIZE    ((int)(sizeof(UBX_NAV_VELECEF_V0_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_RXM_RAWX_VERSION_GET(msg)     (((uint8_t *)(msg))[UBX_HEAD_SIZE + 13])

//! UBX-RXM-RAWX (version 1, output) payload head
typedef struct UBX_RXM_RAWX_V1_GROUP0_s
{
    double   rcvTow;
    uint16_t week;
    int8_t   leapS;
    uint8_t  numMeas;
    uint8_t  recStat;
    uint8_t  version;
    uint8_t  reserved[2];
} UBX_RXM_RAWX_V1_GROUP0_t;

//! UBX-RXM-RAWX (version 1, output) payload repeated group
typedef struct UBX_RXM_RAWX_V1_GROUP1_s
{
    double   prMeas;
    double   cpMeas;
    float    doMeas;
    uint8_t  gnssId;
    uint8_t  svId;
    uint8_t  sigId;
    uint8_t  freqId;
    uint16_t locktime;
    uint8_t  cno;
    uint8_t  prStdev;
    uint8_t  cpStdev;
    uint8_t  doStdev;
    uint8_t  trkStat;
    uint8_t  reserved[1];
} UBX_RXM_RAWX_V1_GROUP1_t;

#define UBX_RXM_RAWX_V1_VERSION                       0x01
#define UBX_RXM_RAWX_V1_RECSTAT_LEAPSEC               0x01
#define UBX_RXM_RAWX_V1_RECSTAT_CLKRESET              0x02
#define UBX_RXM_RAWX_V1_PRSTDEV_PRSTD_GET(f)          ((f) & 0x0f)
#define UBX_RXM_RAWX_V1_PRSTD_SCALE(s)                (0.01 * exp2(s))
#define UBX_RXM_RAWX_V1_CPSTDEV_CPSTD_GET(f)          ((f) & 0x0f)
#define UBX_RXM_RAWX_V1_CPSTD_SCALE(c)                (0.004 * (double)(c))
#define UBX_RXM_RAWX_V1_DOSTDEV_DOSTD_GET(f)          ((f) & 0x0f)
#define UBX_RXM_RAWX_V1_DOSTD_SCALE(s)                (0.002 * exp2(s))
#define UBX_RXM_RAWX_V1_LOCKTIME_SCALE                1e-3
#define UBX_RXM_RAWX_V1_TRKSTAT_PRVALID               0x01
#define UBX_RXM_RAWX_V1_TRKSTAT_CPVALID               0x02
#define UBX_RXM_RAWX_V1_TRKSTAT_HALFCYC               0x04
#define UBX_RXM_RAWX_V1_TRKSTAT_SUBHALFCYC            0x08

#define UBX_RXM_RAWX_V1_MIN_SIZE    ((int)(sizeof(UBX_RXM_RAWX_V1_GROUP0_t) + UBX_FRAME_SIZE))
#define UBX_RXM_RAWX_V1_SIZE(msg) \
    ((int)(sizeof(UBX_RXM_RAWX_V1_GROUP0_t) + UBX_FRAME_SIZE + \
    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 11] * sizeof(UBX_RXM_RAWX_V1_GROUP1_t))))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_RXM_RTCM_VERSION_GET(msg)    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 0])

typedef struct UBX_RXM_RTCM_V2_GROUP0_s
{
    uint8_t  version;
    uint8_t  flags;
    uint16_t subType;
    uint16_t refStation;
    uint16_t msgType;
} UBX_RXM_RTCM_V2_GROUP0_t;

#define UBX_RXM_RTCM_V2_VERSION                       0x02
#define UBX_RXM_RTCM_V2_FLAGS_CRCFAILED               0x01
#define UBX_RXM_RTCM_V2_FLAGS_MSGUSED_GET(f)          ( ((f) >> 1) & 0x03 )
#define UBX_RXM_RTCM_V2_FLAGS_MSGUSED_UNKNOWN         0x00
#define UBX_RXM_RTCM_V2_FLAGS_MSGUSED_UNUSED          0x01
#define UBX_RXM_RTCM_V2_FLAGS_MSGUSED_USED            0x02

#define UBX_RXM_RTCM_V2_SIZE    ((int)(sizeof(UBX_RXM_RTCM_V2_GROUP0_t) + UBX_FRAME_SIZE))
// ---------------------------------------------------------------------------------------------------------------------

#define UBX_RXM_SFRBX_VERSION_GET(msg)    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 6])

typedef struct UBX_RXM_SFRBX_V2_GROUP0_s
{
    uint8_t  gnssId;
    uint8_t  svId;
    uint8_t  sigId; // interface description says "reserved", but u-center says "sigId"...
    uint8_t  freqId;
    uint8_t  numWords;
    uint8_t  chn;
    uint8_t  version;
    uint8_t  reserved1;
} UBX_RXM_SFRBX_V2_GROUP0_t;

typedef struct UBX_RXM_SFRBX_V2_GROUP1_s
{
    uint32_t dwrd;
} UBX_RXM_SFRBX_V2_GROUP1_t;

#define UBX_RXM_SFRBX_V2_VERSION                      0x02

#define UBX_RXM_SFRBX_V2_MIN_SIZE    ((int)(sizeof(UBX_RXM_SFRBX_V2_GROUP0_t) + UBX_FRAME_SIZE))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_MON_RF_VERSION_GET(msg)    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 0])

//! UBX-MON-RF (version 0, output) payload head
typedef struct UBX_MON_RF_V0_GROUP0_s
{
    uint8_t  version;
    uint8_t  nBlocks;
    uint8_t  reserved[2];
} UBX_MON_RF_V0_GROUP0_t;

//! UBX-MON-RF (version 0, output) payload repeated group
typedef struct UBX_MON_RF_V0_GROUP1_s
{
    uint8_t  blockId;
    uint8_t  flags;
    uint8_t  antStatus;
    uint8_t  antPower;
    uint32_t postStatus;
    uint8_t  reserved1[4];
    uint16_t noisePerMS;
    uint16_t agcCnt;
    uint8_t  jamInd;
    int8_t   ofsI;
    uint8_t  magI;
    int8_t   ofsQ;
    uint8_t  magQ;
    uint8_t  reserved2[3];
} UBX_MON_RF_V0_GROUP1_t;

#define UBX_MON_RF_V0_VERSION                         0x00
#define UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_GET(f)       ( (f) & 0x03 )
#define UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_UNKN         0
#define UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_OK           1
#define UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_WARN         2
#define UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_CRIT         3
#define UBX_MON_RF_V0_ANTSTATUS_INIT                  0
#define UBX_MON_RF_V0_ANTSTATUS_DONTKNOW              1
#define UBX_MON_RF_V0_ANTSTATUS_OK                    2
#define UBX_MON_RF_V0_ANTSTATUS_SHORT                 3
#define UBX_MON_RF_V0_ANTSTATUS_OPEN                  4
#define UBX_MON_RF_V0_ANTPOWER_OFF                    0
#define UBX_MON_RF_V0_ANTPOWER_ON                     1
#define UBX_MON_RF_V0_ANTPOWER_DONTKNOW               2
#define UBX_MON_RF_V0_NOISEPERMS_MAX                  200 // This seems to be what u-center uses..
#define UBX_MON_RF_V0_AGCCNT_MAX                      8191
#define UBX_MON_RF_V0_JAMIND_MAX                      255

#define UBX_MON_RF_V0_MIN_SIZE    ((int)(sizeof(UBX_MON_RF_V0_GROUP0_t) + UBX_FRAME_SIZE))
#define UBX_MON_RF_V0_SIZE(msg) \
    ((int)(sizeof(UBX_MON_RF_V0_GROUP0_t) + UBX_FRAME_SIZE + \
    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 1] * sizeof(UBX_MON_RF_V0_GROUP1_t))))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_MON_COMMS_VERSION_GET(msg)    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 0])

//! UBX-MON-COMMS (version 0, output) payload head
typedef struct UBX_MON_COMMS_V0_GROUP0_s
{
    uint8_t  version;
    uint8_t  nPorts;
    uint8_t  txErrors;
    uint8_t  reserved0;
    uint8_t  protIds[4];
} UBX_MON_COMMS_V0_GROUP0_t;

//! UBX-MON-COMMS (version 0, output) payload repeated group
typedef struct UBX_MON_COMMS_V0_GROUP1_s
{
    // Similar to UBX-MON-HW3.pinId this seems to be made up of two values actually:
    // - a port ID (probably same enum as in u-center's UBX-CFG-PRT: 0 = I2C, 1 = UART1, 2 = UART2, 3 = USB, 4 = SPI)
    // - a "bank" of ports (0, 1, ...)
    // u-center only shows some of the ports it seems (why?!)
    uint16_t portId;
    uint16_t txPending;
    uint32_t txBytes;
    uint8_t  txUsage;
    uint8_t  txPeakUsage;
    uint16_t rxPending;
    uint32_t rxBytes;
    uint8_t  rxUsage;
    uint8_t  rxPeakUsage;
    uint16_t overrunErrors;
    uint16_t msgs[4];
    uint8_t  reserved1[8];
    uint32_t skipped;
} UBX_MON_COMMS_V0_GROUP1_t;

#define UBX_MON_COMMS_V0_VERSION             0x00

#define UBX_MON_COMMS_V0_TXERRORS_MEM        0x01
#define UBX_MON_COMMS_V0_TXERRORS_ALLOC      0x02

#define UBX_MON_COMMS_V0_PROTIDS_UBX         0x00
#define UBX_MON_COMMS_V0_PROTIDS_NMEA        0x01
#define UBX_MON_COMMS_V0_PROTIDS_RAW         0x03 // probably.. see UBX-MON-MSGPP
#define UBX_MON_COMMS_V0_PROTIDS_RTCM2       0x02
#define UBX_MON_COMMS_V0_PROTIDS_RTCM3       0x05
#define UBX_MON_COMMS_V0_PROTIDS_SPARTN      0x06
#define UBX_MON_COMMS_V0_PROTIDS_OTHER       0xff

#define UBX_MON_COMMS_V0_MIN_SIZE    ((int)(sizeof(UBX_MON_COMMS_V0_GROUP0_t) + UBX_FRAME_SIZE))
#define UBX_MON_COMMS_V0_SIZE(msg) \
    ((int)(sizeof(UBX_MON_COMMS_V0_GROUP0_t) + UBX_FRAME_SIZE + \
    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 1] * sizeof(UBX_MON_COMMS_V0_GROUP1_t))))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_MON_SPAN_VERSION_GET(msg)    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 0])

//! UBX-MON-RF (version 0, output) payload head
typedef struct UBX_MON_SPAN_V0_GROUP0_s
{
    uint8_t  version;
    uint8_t  numRfBlocks;
    uint8_t  reserved[2];
} UBX_MON_SPAN_V0_GROUP0_t;

//! UBX-MON-RF (version 0, output) payload repeated group
typedef struct UBX_MON_SPAN_V0_GROUP1_s
{
    uint8_t  spectrum[256];
    uint32_t span;
    uint32_t res;
    uint32_t center;
    uint8_t  pga;
    uint8_t  reserved[3];
} UBX_MON_SPAN_V0_GROUP1_t;

#define UBX_MON_SPAN_V0_VERSION                       0x00
#define UBX_MON_SPAN_BIN_CENT_FREQ(g1, ix) \
    ( (double)g1.center + (double)g1.span * (((double)(ix) - 128.0) / 256.0) )

#define UBX_MON_SPAN_V0_MIN_SIZE    ((int)(sizeof(UBX_MON_SPAN_V0_GROUP0_t) + UBX_FRAME_SIZE))
#define UBX_MON_SPAN_V0_SIZE(msg) \
    ((int)(sizeof(UBX_MON_SPAN_V0_GROUP0_t) + UBX_FRAME_SIZE + \
    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 1] * sizeof(UBX_MON_SPAN_V0_GROUP1_t))))

// ---------------------------------------------------------------------------------------------------------------------

typedef struct UBX_ESF_MEAS_V0_GROUP0_s
{
    uint32_t timeTag;
    uint16_t flags;
    uint16_t id;
} UBX_ESF_MEAS_V0_GROUP0_t;

typedef struct UBX_ESF_MEAS_V0_GROUP1_s
{
    uint32_t data;
} UBX_ESF_MEAS_V0_GROUP1_t;

typedef struct UBX_ESF_MEAS_V0_GROUP2_s
{
    uint32_t calibTtag;
} UBX_ESF_MEAS_V0_GROUP2_t;

#define UBX_ESF_MEAS_V0_FLAGS_TIMEMARKSENT_GET(f)     ( (uint8_t)(f) & 0x03 )
#define UBX_ESF_MEAS_V0_FLAGS_TIMEMARKSENT_NONE       0
#define UBX_ESF_MEAS_V0_FLAGS_TIMEMARKSENT_EXT0       1
#define UBX_ESF_MEAS_V0_FLAGS_TIMEMARKSENT_EXT1       2
#define UBX_ESF_MEAS_V0_FLAGS_CALIBTTAGVALID          0x0008
#define UBX_ESF_MEAS_V0_FLAGS_NUMMEAS_GET(f)          ( ((uint16_t)(f) >> 11) & 0x1f )
#define UBX_ESF_MEAS_V0_DATA_DATAFIELD_GET(f)         ( (uint32_t)(f) & 0x00ffffff )
#define UBX_ESF_MEAS_V0_DATA_DATATYPE_GET(f)          ( ((uint32_t)(f) >> 24) & 0x0000003f ) // same enum as UBX-ESF-STATUS.type it seems
#define UBX_ESF_MEAS_V0_CALIBTTAG_SCALE               1e-3

#define UBX_ESF_MEAS_V0_MIN_SIZE    ((int)(sizeof(UBX_ESF_MEAS_V0_GROUP0_t) + UBX_FRAME_SIZE))
#define UBX_ESF_MEAS_V0_SIZE(msg) /* argh.. nice message design! */ \
    ((int)(sizeof(UBX_ESF_MEAS_V0_GROUP0_t) + UBX_FRAME_SIZE + \
    (UBX_ESF_MEAS_V0_FLAGS_NUMMEAS_GET( *((uint16_t *)&((uint8_t *)(msg))[UBX_HEAD_SIZE + 4]) ) * sizeof(UBX_ESF_MEAS_V0_GROUP1_t) + \
    ( ((*((uint16_t *)&((uint8_t *)(msg))[UBX_HEAD_SIZE + 4]) & UBX_ESF_MEAS_V0_FLAGS_CALIBTTAGVALID) \
        == UBX_ESF_MEAS_V0_FLAGS_CALIBTTAGVALID) ? sizeof(UBX_ESF_MEAS_V0_GROUP2_t) : 0 ))))

// ---------------------------------------------------------------------------------------------------------------------

#define UBX_ESF_STATUS_VERSION_GET(msg)    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 4])

typedef struct UBX_ESF_STATUS_V2_GROUP0_s
{
    uint32_t iTOW;
    uint8_t  version;
    uint8_t  initStatus1;
    uint8_t  initStatus2;
    uint8_t  reserved0[5];
    uint8_t  fusionMode;
    uint8_t  reserved1[2];
    uint8_t  numSens;
} UBX_ESF_STATUS_V2_GROUP0_t;

typedef struct UBX_ESF_STATUS_V2_GROUP1_s
{
    uint8_t  sensStatus1;
    uint8_t  sensStatus2;
    uint8_t  freq;
    uint8_t  faults;
} UBX_ESF_STATUS_V2_GROUP1_t;

#define UBX_ESF_STATUS_V2_VERSION                                    0x02
#define UBX_ESF_STATUS_V2_ITOW_SCALE                                 1e-3
#define UBX_ESF_STATUS_V2_INITSTATUS1_WTINITSTATUS_GET(f)            ( (uint8_t)(f) & 0x03 )
#define UBX_ESF_STATUS_V2_INITSTATUS1_WTINITSTATUS_OFF               0
#define UBX_ESF_STATUS_V2_INITSTATUS1_WTINITSTATUS_INITALIZING       1
#define UBX_ESF_STATUS_V2_INITSTATUS1_WTINITSTATUS_INITIALIZED       2
#define UBX_ESF_STATUS_V2_INITSTATUS1_MNTALGSTATUS_GET(f)            ( ((uint8_t)(f) >> 2) & 0x07 )
#define UBX_ESF_STATUS_V2_INITSTATUS1_MNTALGSTATUS_OFF               0
#define UBX_ESF_STATUS_V2_INITSTATUS1_MNTALGSTATUS_INITALIZING       1
#define UBX_ESF_STATUS_V2_INITSTATUS1_MNTALGSTATUS_INITIALIZED1      2
#define UBX_ESF_STATUS_V2_INITSTATUS1_MNTALGSTATUS_INITIALIZED2      3
#define UBX_ESF_STATUS_V2_INITSTATUS1_INSINITSTATUS_GET(f)           ( ((uint8_t)(f) >> 5) & 0x07 )
#define UBX_ESF_STATUS_V2_INITSTATUS1_INSINITSTATUS_OFF              0
#define UBX_ESF_STATUS_V2_INITSTATUS1_INSINITSTATUS_INITALIZING      1
#define UBX_ESF_STATUS_V2_INITSTATUS1_INSINITSTATUS_INITIALIZED1     2
#define UBX_ESF_STATUS_V2_INITSTATUS1_INSINITSTATUS_INITIALIZED2     3
#define UBX_ESF_STATUS_V2_INITSTATUS2_IMUINITSTATUS_GET(f)           ( (uint8_t)(f) & 0x03 )
#define UBX_ESF_STATUS_V2_INITSTATUS2_IMUINITSTATUS_OFF              0
#define UBX_ESF_STATUS_V2_INITSTATUS2_IMUINITSTATUS_INITALIZING      1
#define UBX_ESF_STATUS_V2_INITSTATUS2_IMUINITSTATUS_INITIALIZED1     2
#define UBX_ESF_STATUS_V2_INITSTATUS2_IMUINITSTATUS_INITIALIZED2     3
#define UBX_ESF_STATUS_V2_FUSIONMODE_INIT                            0x00
#define UBX_ESF_STATUS_V2_FUSIONMODE_FUSION                          0x01
#define UBX_ESF_STATUS_V2_FUSIONMODE_SUSPENDED                       0x02
#define UBX_ESF_STATUS_V2_FUSIONMODE_DISABLED                        0x03
#define UBX_ESF_STATUS_V2_SENSSTATUS1_TYPE_GET(f)                    ( (uint8_t)(f) & 0x3f ) // same enum as UBX-ESF-MEAS.dataType it seems
#define UBX_ESF_STATUS_V2_SENSSTATUS1_USED                           0x40
#define UBX_ESF_STATUS_V2_SENSSTATUS1_READY                          0x80
#define UBX_ESF_STATUS_V2_SENSSTATUS2_CALIBSTATUS_GET(f)             ( (uint8_t)(f) & 0x03 )
#define UBX_ESF_STATUS_V2_SENSSTATUS2_CALIBSTATUS_NOTCALIB           0
#define UBX_ESF_STATUS_V2_SENSSTATUS2_CALIBSTATUS_CALIBRATING        1
#define UBX_ESF_STATUS_V2_SENSSTATUS2_CALIBSTATUS_CALIBRATED1        2
#define UBX_ESF_STATUS_V2_SENSSTATUS2_CALIBSTATUS_CALIBRATED2        3
#define UBX_ESF_STATUS_V2_SENSSTATUS2_TIMESTATUS_GET(f)              ( ((uint8_t)(f) >> 2) & 0x03 )
#define UBX_ESF_STATUS_V2_SENSSTATUS2_TIMESTATUS_NODATA              0
#define UBX_ESF_STATUS_V2_SENSSTATUS2_TIMESTATUS_FIRSTBYTE           1
#define UBX_ESF_STATUS_V2_SENSSTATUS2_TIMESTATUS_EVENT               2
#define UBX_ESF_STATUS_V2_SENSSTATUS2_TIMESTATUS_TIMETAG             3
#define UBX_ESF_STATUS_V2_FAULTS_BADMEAS                             0x01
#define UBX_ESF_STATUS_V2_FAULTS_BADTTAG                             0x02
#define UBX_ESF_STATUS_V2_FAULTS_MISSINGMEAS                         0x04
#define UBX_ESF_STATUS_V2_FAULTS_NOISYMEAS                           0x08

#define UBX_ESF_STATUS_V2_MIN_SIZE    ((int)(sizeof(UBX_ESF_STATUS_V2_GROUP0_t) + UBX_FRAME_SIZE))
#define UBX_ESF_STATUS_V2_SIZE(msg) \
    ((int)(sizeof(UBX_ESF_STATUS_V2_GROUP0_t) + UBX_FRAME_SIZE + \
    (((uint8_t *)(msg))[UBX_HEAD_SIZE + 15] * sizeof(UBX_ESF_STATUS_V2_GROUP1_t))))

/* ****************************************************************************************************************** */

//! Make a UBX message
/*!
    \param[in]   clsId        Message class
    \param[in]   msgId        Message ID
    \param[in]   payload      The message payload (can be NULL)
    \param[in]   payloadSize  Size of the message payload (ignored if \c payload is NULL)
    \param[out]  msg          Message buffer (must be at least \c payloadSize + #UBX_FRAME_SIZE)

    \note \c payload and \c msg may be the same buffer (or may overlap).

    \returns the message size (= \c payloadSize + #UBX_FRAME_SIZE)
*/
int ubxMakeMessage(const uint8_t clsId, const uint8_t msgId, const uint8_t *payload, const uint16_t payloadSize, uint8_t *msg);

typedef struct UBX_CFG_VALSET_MSG_s
{
    uint8_t msg[UBX_CFG_VALSET_V1_MAX_SIZE];
    int     size;
    char    info[200];
} UBX_CFG_VALSET_MSG_t;

//! Make series of UBX-CFG-VALSET messages
UBX_CFG_VALSET_MSG_t *ubxKeyValToUbxCfgValset(const UBLOXCFG_KEYVAL_t *kv, const int nKv, const bool ram, const bool bbr, const bool flash, int *nValset);

//! Get UBX message name
/*!
    Generates a name (string) in the form "UBX-CLSID-MSGID", where CLSID and MSGID are suitable stringifications of the
    class ID and message ID.

    \param[out] name     String to write the name to
    \param[in]  size     Size of \c name (incl. nul termination)
    \param[in]  msg      UBX message
    \param[in]  msgSize  Size of \c msg

    \returns true if message name was generated, false if \c name buffer was too small
*/
bool ubxMessageName(char *name, const int size, const uint8_t *msg, const int msgSize);

//! Get UBX message name
/*!
    Like ubxMessageName() but base in known class and message IDs.

    \param[out] name     String to write the name to
    \param[in]  size     Size of \c name (incl. nul termination)
    \param[in]  clsId    Class ID
    \param[in]  msgId    Message ID

    \returns true if message name was generated, false if \c name buffer was too small
*/
bool ubxMessageNameIds(char *name, const int size, const uint8_t clsId, const uint8_t msgId);

//! Get UBX message IDs
/*!
    \param[in]   name   Message name
    \param[out]  clsId  Class ID, or NULL
    \param[out]  msgId  Message ID, or NULL

    \returns true if \c name was found and \c clsId and \c msgId are valid
*/
bool ubxMessageClsId(const char *name, uint8_t *clsId, uint8_t *msgId);

//! Render message info string
bool ubxMessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize);

//! Stringify UBX gnssId
/*!
    \param[in]  gnssId  The GNSS ID (see #UBX_GNSSID_GPS etc.)
    \returns the string (name) of the GNSS ("GPS", "GLO", "BDS", "GAL", etc.) or "?" if gnssId is invalid
*/
const char *ubxGnssStr(const uint8_t gnssId);

//! Stringify UBX gnssId and SvId
/*!
    \param[in]  gnssId  The GNSS ID (see #UBX_GNSSID_GPS etc.)
    \param[in]  svId    The SV ID
    \returns the string (name) for the SV ("G12", "R04", "S157", etc.) or partial string
        if svId is out of range (e.g. "G?") or "?" if gnssId and svId is invalid
*/
const char *ubxSvStr(const uint8_t gnssId, const uint8_t svId);

//! Stringify UBX sigId
/*!
    \param[in]  gnssId  The GNSS ID (see #UBX_GNSSID_GPS etc.)
    \param[in]  sigId   The signal ID
    \returns the string (name) for the signal (e.g. "L1CA", "E5bI", etc.) or "?" for invalid sigId (or gnssId)
*/
const char *ubxSigStr(const uint8_t gnssId, const uint8_t sigId);

bool ubxMonVerToVerStr(char *str, const int size, const uint8_t *msg, const int msgSize);

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_UBX_H__
