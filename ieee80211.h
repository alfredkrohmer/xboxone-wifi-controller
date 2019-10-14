/*
 * Copyright (C) 2019 Medusalix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * extracted and modified pieces from here:
 * https://github.com/360Controller/360Controller/blob/8dbc5dbd76ec42e039220238f743c2e82be0ad10/WirelessOneController/WirelessOneMT76.h
 */


#include <stdint.h>


// WLAN frame types
#define FT_MGMT 0x00
#define FT_CTRL 0x01
#define FT_DATA 0x02

// WLAN frame subtypes
#define FT_ASSOC_REQ 0x00
#define FT_ASSOC_RESP 0x01
#define FT_REASSOC_REQ 0x02
#define FT_PROBE_REQ 0x04
#define FT_PROBE_RESP 0x05
#define FT_DISASSOC 0x0a
#define FT_PAIR 0x07
#define FT_BEACON 0x08
#define FT_QOS_DATA 0x08
#define FT_QOS_NULL 0x0c
#define FT_ACK 0x0d


struct QosFrame
{
    uint16_t qosControl;
} __attribute__((packed));

struct AssociationResponseFrame
{
    uint16_t capabilityInfo;
    uint16_t statusCode;
    uint16_t associationId;
    uint16_t ssid;
} __attribute__((packed));

struct BeaconFrame
{
    uint64_t timestamp;
    uint16_t interval;
    uint16_t capabilityInfo;
    uint16_t ssid;
} __attribute__((packed));

struct PairingFrame
{
    uint8_t unknown;
    uint8_t type;
} __attribute__((packed));

struct FrameControl
{
    uint32_t protocolVersion : 2;
    uint32_t type : 2;
    uint32_t subtype : 4;
    uint32_t toDs : 1;
    uint32_t fromDs : 1;
    uint32_t moreFragments : 1;
    uint32_t retry : 1;
    uint32_t powerManagement : 1;
    uint32_t moreData : 1;
    uint32_t protectedFrame : 1;
    uint32_t order : 1;
} __attribute__((packed));

struct WlanFrame
{
    FrameControl frameControl;
    uint16_t duration;

    uint8_t destination[6];
    uint8_t source[6];
    uint8_t bssId[6];

    uint16_t sequenceControl;
} __attribute__((packed));