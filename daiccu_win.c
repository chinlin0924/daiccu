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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#include "canusb/canusb.h"
#include "processor/ccu.h"

#define NEED_UPDATE_TIME 250

typedef struct {
    ccuProcessor *ccu;
    canDev *can;
    DWORD lastUpdates[13];
    DWORD lastValue[13];
} daiCcu;

static void processCcuCommand(void *handle, CcuCommands cmd, int value)
{
    daiCcu *canCcu = handle;

    WORD key;
    int index;
    switch (cmd) {
    case CcuUp:         index =  0; key = VK_NUMPAD8; break;
    case CcuDown:       index =  1; key = VK_NUMPAD2; break;
    case CcuLeft:       index =  2; key = VK_NUMPAD4; break;
    case CcuRight:      index =  3; key = VK_NUMPAD6; break;
    case CcuUpLeft:     index =  4; key = VK_NUMPAD7; break;
    case CcuUpRight:    index =  5; key = VK_NUMPAD9; break;
    case CcuDownLeft:   index =  6; key = VK_NUMPAD1; break;
    case CcuDownRight:  index =  7; key = VK_NUMPAD3; break;
    case CcuSelect:     index =  8; key = VK_NUMPAD5; break;
    case CcuBack:       index =  9; key = VK_BACK;    break;
    case CcuClear:      index = 10; key = VK_DELETE;  break;
    case CcuSeat:       index = 11; key = VK_MENU;    break;
    case CcuFavorite:   index = 12; key = VK_HOME;    break;
    default:             return;
    }

    /* Is update required */
    bool update = false;
    DWORD currentTime = GetTickCount();
    /*has the button been released or pressed?*/
    update = (canCcu->lastValue[index] != value);
    /*has the time limit expired?*/
    update = update || ((currentTime - canCcu->lastUpdates[index] > NEED_UPDATE_TIME));
    /* printf("main: Update required for command %d and value %d and time %d : %s\n",
         *cmd,value,currentTime - priv->lastUpdates[cmd], result ? "true" : "false");*/
    if(update == true) {
        canCcu->lastUpdates[index] = currentTime;
    }
    canCcu->lastValue[index] = value;
    if (!update)
        return;

    /* Send input */
    INPUT inputs[1];
    KEYBDINPUT keyinputs[1];

    keyinputs[0].wVk = key;
    keyinputs[0].dwFlags = (value == 0) ? KEYEVENTF_KEYUP : 0;

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki = keyinputs[0];

    if(!SendInput(1, inputs, sizeof(INPUT))) {
        printf("daiccu: Could not send key press.\n");
    }

#ifdef _DEBUG
    printf("main: Process command: %d %s\n", cmd,
           value ? "pressed" : "released");
#endif
}

static void processCcuRotation(void* handle, int value)
{
    DWORD key;
    int i = 0;
    INPUT inputs[2];
    KEYBDINPUT keyinputs[2];

    /* Left or right rotation? */
    if(value < 0) {
        key = VK_LEFT;
        value = -value;
    } else {
        key = VK_RIGHT;
    }

    for (i = 0; i < value; i++) {
        keyinputs[0].wVk = key;
        keyinputs[0].dwFlags = 0;

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki = keyinputs[0];

        keyinputs[1].wVk = key;
        keyinputs[1].dwFlags = KEYEVENTF_KEYUP;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki = keyinputs[1];

        if(!SendInput(2, inputs, sizeof(INPUT))) {
            printf("daiccu: Could not send key press.\n");
        }
    }
#ifdef _DEBUG
    printf("main: Process rotate: %d\n", value);
#endif
}

static void processMultipleCommands(void *handle, uint32_t cmds)
{
#ifdef _DEBUG
    printf("main: Process multiple commands: 0x%08x\n", cmds);
#endif
}

static void stateChanged(void *handle, bool open)
{
    daiCcu *canCcu = handle;
    if (open) {
        ccuProcessorSendKeepAliveMessage(canCcu->ccu, canCcu);
        ccuProcessorSendStateMessage(canCcu->ccu, canCcu);
    }
}

static void messageReceived(void *handle, uint16_t id, uint8_t dlc,
                     const uint8_t *bytes)
{
    daiCcu *canCcu = handle;
    ccuProcessorProcess(canCcu->ccu, id, dlc, bytes);
}

bool canTransmit(void *handle, const uint32_t id, const uint8_t dlc,
                 const char *data)
{
    daiCcu *canCcu = handle;
    canUsbTransmit(canCcu->can, id, dlc, data);

    return true;
}

DWORD WINAPI ccuRun(LPVOID handle)
{
    daiCcu *canCcu = handle;
    while (1) {
        if (!canUsbRun(canCcu->can))
            break;
    }
    return 1;
}

static void readInput() {
    bool waiting = true;
    char inputChar = 0;
    while (waiting) {
        // you can add your own key handling here.
        // Keys can be typed in the console window
        inputChar = getchar();
        switch (inputChar) {
            case 'q': {
                waiting = false;
                break;
            }
        }
    }
}

int main(int argc, char **argv)
{
    daiCcu canCcu;
    ccuProcessor *ccu = 0;
    canDev *can = 0;

    /* Init CCU */
    ccu = ccuProcessorGet();
    ccu->handle = &canCcu;
    ccu->ccuProcessCommand = processCcuCommand;
    ccu->ccuProcessRotation = processCcuRotation;
    ccu->ccuProcessMultipleCommands = processMultipleCommands;

    /* Enable longpress */
    ccu->processWhilePressed = true;

    /* Init CAN */
    can = canUsbGet();
    can->handle = &canCcu;
    can->canStateChanged = stateChanged;
    can->canMessageReceived = messageReceived;

    /* Init handles */
    canCcu.can = can;
    canCcu.ccu = ccu;

    if (!canUsbOpen(canCcu.can, argc, argv)) {
        printf("main: Open CAN failed!\n");
        return -1;
    }

    CreateThread(0, 0, ccuRun, (void*) &canCcu, 0, NULL);

    readInput();

    canUsbClose(canCcu.can);

    return 0;
}
