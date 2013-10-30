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
    ccu->lastKeepAlive      = 0;

    /* Init callbacks*/
    ccu->handle             = 0;
    ccu->ccuProcessCommand  = 0;
    ccu->ccuProcessRotation = 0;
    ccu->ccuProcessAtOnces = 0;

    return ccu;
}

bool ccuProcessorSendKeepAliveMessage(ccuProcessor *ccu, void* handle)
{
    (void)ccu;
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

#define ccuProcessorProcessBinary(cmd) \
    isPressed = (current & cmd) ? true : false; \
    if ((ccu->processWhilePressed && isPressed) || (changed & cmd)) \
        ccu->ccuProcessCommand(ccu->handle, cmd, isPressed);

static bool ccuProcessorProcessActuation(ccuProcessor *ccu, uint8_t dlc,
                                         const uint8_t *data)
{
    bool isPressed;
    uint32_t current;
    uint32_t changed;

    if(dlc != 4) {
        printf("Ccu: dlc has not the expected value of 4, but %d instead\n", dlc);
        return false;
    }

    /* Compile CCU commands mask */
	current  = data[0];
	current |= (data[1] & CcuSelect);
	current |= (data[2] << 8);
    changed  = ccu->lastCmdsMask ^ current;
    ccu->lastCmdsMask = current;

    /*printf("Ccu: command mask is: %04x\n", current);*/

    if (!changed && !ccu->processWhilePressed)
        return true;

    /* Process commands at once */
    if(ccu->ccuProcessAtOnces)
        ccu->ccuProcessAtOnces(ccu->handle, current, 0);

    /* Process every command */
    if(ccu->ccuProcessCommand) {
        ccuProcessorProcessBinary(CcuBack);
        ccuProcessorProcessBinary(CcuSeat);
        ccuProcessorProcessBinary(CcuClear);
        ccuProcessorProcessBinary(CcuFavorite);
        ccuProcessorProcessBinary(CcuSelect);
        ccuProcessorProcessBinary(CcuUp);
        ccuProcessorProcessBinary(CcuDown);
        ccuProcessorProcessBinary(CcuLeft);
        ccuProcessorProcessBinary(CcuRight);
        ccuProcessorProcessBinary(CcuUpLeft);
        ccuProcessorProcessBinary(CcuUpRight);
        ccuProcessorProcessBinary(CcuDownLeft);
        ccuProcessorProcessBinary(CcuDownRight);
    }

    return true;
}

static bool ccuProcessorProcessState(ccuProcessor *ccu, uint8_t dlc,
                                     const uint8_t *data)
{
    uint8_t id = data[7];
    uint16_t position = (data[5] << 8) + (unsigned char)data[6];
    int rotate;

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

    /* Get Rotation */
    rotate = position - ccu->lastPosition;
    ccu->lastPosition = position;

    /*printf("Ccu: Process rotation: %d.\n", rotate);*/

    /* Process commands at once */
    if(ccu->ccuProcessAtOnces)
        ccu->ccuProcessAtOnces(ccu->handle, ccu->lastCmdsMask, rotate);

    /* Process rotate */
    if(ccu->ccuProcessAtOnces)
        ccu->ccuProcessRotation(ccu->handle, rotate);

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
