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

typedef enum {
    CcuNone      = 0x0000,
    CcuBack      = 0x0002,
    CcuSeat      = 0x0004,
    CcuClear     = 0x0020,
    CcuFavorite  = 0x0040,
    CcuSelect    = 0x0080,
    CcuUp        = 0x0100,
    CcuUpRight   = 0x0200,
    CcuRight     = 0x0400,
    CcuDownRight = 0x0800,
    CcuDown      = 0x1000,
    CcuDownLeft  = 0x2000,
    CcuLeft      = 0x4000,
    CcuUpLeft    = 0x8000
} CcuCommands;

typedef void (*ccuProcessCommandProc)(void *handle, CcuCommands cmd, bool pressed);
typedef void (*ccuProcessRotationProc)(void *handle, int rotate);
typedef void (*ccuProcessAtOncesProc)(void *handle, uint32_t cmds, int rotate);

typedef struct {
    bool running;
    uint8_t stateId;
    uint16_t lastPosition;
    uint32_t lastCmdsMask;
    long lastKeepAlive;

    void *handle;
    bool processWhilePressed;
    ccuProcessCommandProc ccuProcessCommand;
    ccuProcessRotationProc ccuProcessRotation;
    ccuProcessAtOncesProc ccuProcessAtOnces;
} ccuProcessor;

ccuProcessor* ccuProcessorGet(void);
bool ccuProcessorProcess(ccuProcessor *ccu, uint16_t id, uint8_t dlc,
                         const uint8_t *data);

const char* ccuCommandToString(CcuCommands cmd);

bool ccuProcessorSendKeepAliveMessage(ccuProcessor *ccu, void *handle);
bool ccuProcessorSendStateMessage(ccuProcessor *ccu, void *handle);

#if(defined __cplusplus)
}
#endif
#endif
