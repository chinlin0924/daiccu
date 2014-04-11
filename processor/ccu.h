/******************************************************************************
 **
 ** Copyright (C) 2012 - 2013 Matthias Benesch <twoof7@freenet.de>. All rights
 ** reserved.
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the
 ** Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 ** Boston, MA 02111-1307, USA.
 **
 ******************************************************************************/
#ifndef CCUPROCESSOR_H
#define CCUPROCESSOR_H

#include <stdint.h>
#include <stdbool.h>

#if(defined __cplusplus)
extern "C"
{
#endif

/* Rotary */

#define CCU_ROTARY_SNA 0xFFFF
#define CCU_ROTARY_MAX 9999

/* Commands */

typedef enum {

    /* CCU */
#define CCU_COMMANDS_OFFSET 0
    /* Directions */
    CcuUp                   = 0,
    CcuUpRight              = 1,
    CcuRight                = 2,
    CcuDownRight            = 3,
    CcuDown                 = 4,
    CcuDownLeft             = 5,
    CcuLeft                 = 6,
    CcuUpLeft               = 7,
    CcuRotate               = 8,
    /* Hardkeys */
    CcuSelect               = 9,
    CcuBack                 = 10,
    CcuClear                = 11,
    CcuStar                 = 12,
    CcuOn                   = 13,
    CcuRadioDisc            = 14,
    CcuTelNav               = 15,
    CcuSeat                 = 16,
#define CCU_COMMANDS_MAX_STAR1 16
    CcuPhone                = 17,
    CcuRadio                = 18,
    CcuNavi                 = 19,
    CcuMedia                = 20,
    CcuCar                  = 21,
    CcuMenu                 = 22,
    CcuMute                 = 23,
#define CCU_COMMANDS_MAX_STAR2 23

    /* Number Pad */
#define NPAD_COMMANDS_OFFSET 32
    NPad0_Touched           = 32,
    NPad0_Pressed           = 33,
    NPad1_Touched           = 34,
    NPad1_Pressed           = 35,
    NPad2_Touched           = 36,
    NPad2_Pressed           = 37,
    NPad3_Touched           = 38,
    NPad3_Pressed           = 39,
    NPad4_Touched           = 40,
    NPad4_Pressed           = 41,
    NPad5_Touched           = 42,
    NPad5_Pressed           = 43,
    NPad6_Touched           = 44,
    NPad6_Pressed           = 45,
    NPad7_Touched           = 46,
    NPad7_Pressed           = 47,
    NPad8_Touched           = 48,
    NPad8_Pressed           = 49,
    NPad9_Touched           = 50,
    NPad9_Pressed           = 51,
    NPadSend_Touched        = 52,
    NPadSend_Pressed        = 53,
    NPadEnd_Touched         = 54,
    NPadEnd_Pressed         = 55,
    NPadFavorite_Touched    = 56,
    NPadFavorite_Pressed    = 57,
    NPadClear_Touched       = 58,
    NPadClear_Pressed       = 59,
    NPadPound_Touched       = 60,
    NPadPound_Pressed       = 61,
    NPadStar_Touched        = 62,
    NPadStar_Pressed        = 63
#define NPAD_COMMANDS_MAX 63

} CcuCommands;

/* Functional */

typedef void (*ccuProcessCommandProc)(void *handle, CcuCommands cmd, int value);

typedef struct {
    uint8_t stateId;
    uint16_t lastPosition;
    uint32_t lastCmdsMask;
    uint32_t lastNPadMask;

    void *handle;
    int version;
    bool processWhilePressed;
    uint16_t maxPosition;
    uint16_t startPosition;

    ccuProcessCommandProc ccuProcessCommand;
} ccuProcessor;

ccuProcessor* ccuProcessorGet(int argc, char **argv);
bool ccuProcessorProcess(ccuProcessor *ccu, uint16_t id, uint8_t dlc,
                         const uint8_t *data);

/* To string */
const char* ccuCommandToString(CcuCommands cmd);

/* Configuration */

bool ccuDisableRotaryRange(ccuProcessor *ccu);
bool ccuSetRotaryRange(ccuProcessor *ccu, int start, int max);
bool ccuProcessorSendStateMessage(ccuProcessor *ccu);

#if(defined __cplusplus)
}
#endif
#endif
