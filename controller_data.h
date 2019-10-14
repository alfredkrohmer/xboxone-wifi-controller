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
 * extracted pieces from here:
 * https://github.com/360Controller/360Controller/blob/8dbc5dbd76ec42e039220238f743c2e82be0ad10/WirelessOneController/WirelessOneDongle.h
 */


#include <stdint.h>


#define PAIR_REQUEST 0x01
#define PAIR_RESPONSE 0x02


struct ControllerFrame
{
    uint8_t command;
    uint8_t message;
    uint8_t sequence;
    uint8_t length;
} __attribute__((packed));

struct LedModeData
{
    uint8_t unknown;
    uint8_t mode;
    uint8_t brightness;
} __attribute__((packed));


struct Buttons
{
    uint32_t unknown : 2;
    uint32_t start : 1;
    uint32_t back : 1;
    uint32_t a : 1;
    uint32_t b : 1;
    uint32_t x : 1;
    uint32_t y : 1;
    uint32_t dpadUp : 1;
    uint32_t dpadDown : 1;
    uint32_t dpadLeft : 1;
    uint32_t dpadRight : 1;
    uint32_t bumperLeft : 1;
    uint32_t bumperRight : 1;
    uint32_t stickLeft : 1;
    uint32_t stickRight : 1;
} __attribute__((packed));

struct InputData
{
    Buttons buttons;
    uint16_t triggerLeft;
    uint16_t triggerRight;
    int16_t stickLeftX;
    int16_t stickLeftY;
    int16_t stickRightX;
    int16_t stickRightY;
} __attribute__((packed));