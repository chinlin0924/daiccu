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
#include "processor/ccu.h"
#ifdef USE_SOCKETCAN
#include "socketcan/socketcan.h"
#define CAN_PREFIX_ socketCan
#else
#include "canusb/canusb.h"
#define CAN_PREFIX_ canUsb
#endif

#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#define CAN_PREFIX_Get CONCAT2E(CAN_PREFIX_,Get)
#define CAN_PREFIX_Open CONCAT2E(CAN_PREFIX_,Open)
#define CAN_PREFIX_Run CONCAT2E(CAN_PREFIX_,Run)
#define CAN_PREFIX_Close CONCAT2E(CAN_PREFIX_,Close)
#define CAN_PREFIX_Transmit CONCAT2E(CAN_PREFIX_,Transmit)

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>

#ifdef USE_X11
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#endif

#define _DEBUG

typedef struct {
    ccuProcessor *ccu;
    canDev *can;
} daiCcu;

static void processCcuCommand(void *handle, CcuCommands cmd, bool pressed)
{
#ifdef USE_X11
    Display *display;
    unsigned int keycode;
    display = XOpenDisplay(NULL);

    int key;
    switch (cmd) {
    case CcuUp:          key = XK_8; break;
    case CcuDown:        key = XK_2; break;
    case CcuLeft:        key = XK_4; break;
    case CcuRight:       key = XK_6; break;
    case CcuUpLeft:      key = XK_7; break;
    case CcuUpRight:     key = XK_9; break;
    case CcuDownLeft:    key = XK_1; break;
    case CcuDownRight:   key = XK_3; break;
    case CcuSelect:      key = XK_5; break;
    case CcuBack:        key = XK_BackSpace; break;
    case CcuClear:       key = XK_Delete; break;
    case CcuSeat:        key = XK_Control_L; break;
    case CcuFavorite:    key = XK_Control_R; break;
    default:             return;
    }

    /*generate input*/
    keycode = XKeysymToKeycode(display, key);

    XTestFakeKeyEvent(display, keycode, pressed, 0);
    XFlush(display);
    XCloseDisplay(display);
#endif

#ifdef _DEBUG
    printf("main: Process command: 0x%08x %s\n", cmd,
           pressed ? "pressed" : "released");
#endif
}

static void processCcuRotation(void *handle, int rotate)
{
#ifdef USE_X11
    int i = 0;
    unsigned int keycode;
    Display *display = XOpenDisplay(NULL);

    /* Left or right rotation? */
    if (rotate < 0) {
        keycode = XKeysymToKeycode(display, XK_Left);
        rotate = -rotate;
    } else {
        keycode = XKeysymToKeycode(display, XK_Right);
    }

    for (i = 0; i < rotate ; i++) {
        XTestFakeKeyEvent(display, keycode, True, 0);
        XTestFakeKeyEvent(display, keycode, False, 0);
        XFlush(display);
    }

    XCloseDisplay(display);
#endif

#ifdef _DEBUG
    printf("main: Process rotate: %d\n", rotate);
#endif
}


static void processCcuAtOnces(void *handle, uint32_t cmds, int rotate)
{
#ifdef _DEBUG
    printf("main: Process commands: 0x%08x, rotate: %d\n", cmds, rotate);
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
    /*printf("main: Transmitt CAN message: %d (%#x)\n", id, id);*/
    CAN_PREFIX_Transmit(canCcu->can, id, dlc, data);

    return true;
}

static void* ccuRun(void* handle)
{
    daiCcu *canCcu = handle;
    while (1) {
        if (!CAN_PREFIX_Run(canCcu->can))
            break;
    }
    /* need to return something because pthread library requires a thread function to*/
    return NULL;
}

static void readInput()
{
    bool waiting = true;
    char inputChar = 0;
    while (waiting) {
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
    ccu->ccuProcessAtOnces = processCcuAtOnces;

    /* Init CAN */
    can = CAN_PREFIX_Get();
    can->handle = &canCcu;
    can->canStateChanged = stateChanged;
    can->canMessageReceived = messageReceived;

    /* Init handles */
    canCcu.can = can;
    canCcu.ccu = ccu;

    if (!CAN_PREFIX_Open(canCcu.can, argc, argv)) {
        printf("main: Open CAN failed!\n");
        return -1;
    }

    pthread_t canRunThread;
    pthread_create(&canRunThread, NULL, ccuRun, (void*)&canCcu);

    readInput();

    CAN_PREFIX_Close(canCcu.can);

    return 0;
}
