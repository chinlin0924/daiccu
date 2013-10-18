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
#include "ccu.h"

#include "../can.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define CCU_ROTATE_MAX 9999
#define CCU_ROTATE_MID 4999
#define CCU_STATE_ID 0xFE

#define DirectionPush    0x80

enum {
    DirectionNone      = 0x00,
    DirectionUp        = 0x01,
    DirectionUpRight   = 0x02,
    DirectionRight     = 0x04,
    DirectionDownRight = 0x08,
    DirectionDown      = 0x10,
    DirectionDownLeft  = 0x20,
    DirectionLeft      = 0x40,
    DirectionUpLeft    = 0x80
};

enum {
    ButtonBack  = 0x02,
    ButtonSeat  = 0x04,
    ButtonClear = 0x20,
    ButtonStar  = 0x40
};

const char* ccuCommandToString(CcuCommands cmd)
{
    switch (cmd) {
    case CcuUp:          return "up";
    case CcuDown:        return "down";
    case CcuLeft:        return "left";
    case CcuRight:       return "right";
    case CcuUpLeft:      return "up left";
    case CcuUpRight:     return "up right";
    case CcuDownLeft:    return "down left";
    case CcuDownRight:   return "down right";
    case CcuSelect:      return "select";
    case CcuBack:        return "back";
    case CcuClear:       return "clear";
    case CcuSeat:        return "seat";
    case CcuFavorite:    return "favorite";
    default:             return "unknown";
    }
}

static const char nmKeepAliveData[8] = { 0xFD, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x0E };

ccuProcessor* ccuProcessorGet()
{
    ccuProcessor *ccu = malloc(sizeof(ccuProcessor));

    /* Init values*/
    ccu->running            = false;
    ccu->processWhilePressed = false;
    ccu->stateId            = CCU_STATE_ID;
    ccu->lastPosition       = CCU_ROTATE_MID;
    ccu->lastCmdsMask       = 0;
    ccu->lastPushs          = 0;
    ccu->lastDirections     = 0;
    ccu->lastButtons        = 0;
    ccu->lastKeepAlive      = 0;

    /* Init callbacks*/
    ccu->handle             = 0;
    ccu->ccuProcessCommand  = 0;
    ccu->ccuProcessRotation = 0;
    ccu->ccuProcessMultipleCommands = 0;

    return ccu;
}

bool ccuProcessorSendKeepAliveMessage(ccuProcessor *ccu, void* handle)
{
    return canTransmit(handle, 0x43f, 8, nmKeepAliveData);
}

bool ccuProcessorSendStateMessage(ccuProcessor* ccu, void* handle) 
{
    char ccuStateMessageData[8];

    ccuStateMessageData[0] = 0x01;
    /* Detents_Stat - Number of detents state (0 - 9999, SNA = 0xFFFF)*/
    ccuStateMessageData[1] = CCU_ROTATE_MAX >> 8;   /* MAX*/
    ccuStateMessageData[2] = CCU_ROTATE_MAX & 0xFF; /* MAX*/
    /* StPosn_Stat - Start position at activation state (0 - 9999, SNA = 0xFFFF)*/
    ccuStateMessageData[3] = ccu->lastPosition >> 8;
    ccuStateMessageData[4] = ccu->lastPosition & 0xFF;
    /* Local_Posn_Rq - Local position in model request (0 - 9999, SNA = 0xFFFF)*/
    ccuStateMessageData[5] = ccu->lastPosition >> 8;
    ccuStateMessageData[6] = ccu->lastPosition & 0xFF;
    /* CTRL_C_Stat_Id - Central control state ID (0 - 254, SNA = 0xFF)*/
    ccuStateMessageData[7] = ccu->stateId;

    return canTransmit(handle, 0x1bb, 8, ccuStateMessageData);
}

#define ccuProcessorProcessMask(cur, cmd, mask) \
    ((cur) & (mask)) ? cmd : 0

#define ccuProcessorProcessBinary(cmd, mask) \
    isPressed = current & (mask) ? 1 : 0; \
    if ((ccu->processWhilePressed && isPressed) || (changed & (mask))) \
        ccu->ccuProcessCommand(ccu->handle, cmd, isPressed);

static bool ccuProcessorProcessActuation(ccuProcessor *ccu, uint8_t dlc,
                                         const uint8_t *data)
{
    bool isPressed = false;
    uint8_t current;
    uint8_t changed;

    if(dlc != 4) {
        printf("Ccu: dlc has not the expected value of 4, but %d instead\n", dlc);
        return false;
    }

    /*printf("Ccu: Processing data...\n");*/

    /* CCU process commands at once */
    if(ccu->ccuProcessMultipleCommands) {
        uint32_t cmdsMask =
                  ccuProcessorProcessMask(data[2], CcuUp,        DirectionUp)
                | ccuProcessorProcessMask(data[2], CcuDown,      DirectionDown)
                | ccuProcessorProcessMask(data[2], CcuLeft,      DirectionLeft)
                | ccuProcessorProcessMask(data[2], CcuRight,     DirectionRight)
                | ccuProcessorProcessMask(data[2], CcuUpLeft,    DirectionUpLeft)
                | ccuProcessorProcessMask(data[2], CcuUpRight,   DirectionUpRight)
                | ccuProcessorProcessMask(data[2], CcuDownLeft,  DirectionDownLeft)
                | ccuProcessorProcessMask(data[2], CcuDownRight, DirectionDownRight)
                | ccuProcessorProcessMask(data[1], CcuSelect,    DirectionPush)
                | ccuProcessorProcessMask(data[0], CcuBack,      ButtonBack)
                | ccuProcessorProcessMask(data[0], CcuSeat,      ButtonSeat)
                | ccuProcessorProcessMask(data[0], CcuClear,     ButtonClear)
                | ccuProcessorProcessMask(data[0], CcuFavorite,  ButtonStar);

        /*printf("Ccu: Button mask is: %04x\n", cmdsMask);*/
        if (cmdsMask != ccu->lastCmdsMask || ccu->processWhilePressed) {
            ccu->ccuProcessMultipleCommands(ccu->handle, cmdsMask);
            ccu->lastCmdsMask = cmdsMask;
        }
    }

    /* CCU process every command */
    if(!ccu->ccuProcessCommand)
        return true;

    /* CCU Buttons*/
    current = data[0];
    /*printf("Ccu: Button data is: %02x\n", data[0]);*/
    if (current != ccu->lastButtons || ccu->processWhilePressed) {
        changed = ccu->lastButtons ^ current;
        ccuProcessorProcessBinary(CcuBack,      ButtonBack);
        ccuProcessorProcessBinary(CcuSeat,      ButtonSeat);
        ccuProcessorProcessBinary(CcuClear,     ButtonClear);
        ccuProcessorProcessBinary(CcuFavorite,  ButtonStar);
        ccu->lastButtons = current;
    }

    /* CCU Pushs*/
    current = data[1];
    /*printf("Ccu: Select data is: %02x\n", data[1]);*/
    if (current != ccu->lastPushs || ccu->processWhilePressed) {
        changed = ccu->lastPushs ^ current;
        ccuProcessorProcessBinary(CcuSelect,    DirectionPush);
        ccu->lastPushs = current;
    }

    /* CCU Directions*/
    current = data[2];
    /*printf("Ccu: Direction data is: %02x\n", data[2]);*/
    if (current != ccu->lastDirections || ccu->processWhilePressed) {
        changed = ccu->lastDirections ^ current;
        ccuProcessorProcessBinary(CcuUp,        DirectionUp);
        ccuProcessorProcessBinary(CcuDown,      DirectionDown);
        ccuProcessorProcessBinary(CcuLeft,      DirectionLeft);
        ccuProcessorProcessBinary(CcuRight,     DirectionRight);
        ccuProcessorProcessBinary(CcuUpLeft,    DirectionUpLeft);
        ccuProcessorProcessBinary(CcuUpRight,   DirectionUpRight);
        ccuProcessorProcessBinary(CcuDownLeft,  DirectionDownLeft);
        ccuProcessorProcessBinary(CcuDownRight, DirectionDownRight);
        ccu->lastDirections = current;
    }

    return true;
}

static bool ccuProcessorProcessState(ccuProcessor *ccu, uint8_t dlc,
                                     const uint8_t *data)
{
    uint8_t id = data[7];
    uint16_t position = (data[5] << 8) + (unsigned char)data[6];

    if(dlc != 8) {
        printf("Ccu: dlc has not the expected value of 8, but %d instead\n", dlc);
        return false;
    }

    if (id != ccu->stateId) {
        printf("Ccu: Central Control State ID mismatch!\n");
        return false;
    }

    if (position == ccu->lastPosition)
        return true;

    if (position == 0xFFFF) {
        printf("Ccu: Position signal not available!\n");
        return false;
    }

    /*printf("Ccu: Processing rotation with value %d.\n", position - ccu->lastPosition);*/
    ccu->ccuProcessRotation(ccu->handle, position - ccu->lastPosition);
    ccu->lastPosition = position;

    return true;
}

bool ccuProcessorProcess(ccuProcessor *ccu, uint16_t id, uint8_t dlc,
                         const uint8_t *data)
{
    bool res;
    switch(id) {
    case 0xfd: {
        res = ccuProcessorProcessActuation(ccu, dlc, data);
        break;
    }
    case 0xfb: {
        res = ccuProcessorProcessState(ccu, dlc, data);
        if(!ccuProcessorSendStateMessage(ccu, ccu->handle)) {
            printf("Ccu: Sent state message failed.\n");
        }
        break;
    }
    default: res = false;
    }

    long currentTime = time(NULL);
    if(currentTime - ccu->lastKeepAlive >= 1) {
        if(!ccuProcessorSendKeepAliveMessage(ccu, ccu->handle)) {
            printf("Ccu: Sent keep-alive message failed.\n");
        } else {
            ccu->lastKeepAlive = currentTime;
        }
    }

    return res;
}
