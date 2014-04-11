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
#include "processor/networkmanager.h"

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
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifdef USE_X11
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#endif

typedef struct {
    ccuProcessor *ccu;
    canDev *can;
#ifdef WIN32
#define NEED_UPDATE_TIME 250
    DWORD lastUpdates[13];
    DWORD lastValue[13];
#endif
} daiCcu;

static void processCcuRotation(void *handle, int rotate);

static void processCcuCommand(void *handle, CcuCommands cmd, int value)
{
    daiCcu *canCcu = handle;

    if (cmd == CcuRotate) return processCcuRotation(handle, value);

#ifdef _DEBUG
    printf("main: Process CCU command: 0x%08x (%s) %s\n", cmd,
           ccuCommandToString(cmd), value ? "pressed" : "released");
#endif

#ifdef WIN32
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
    case CcuStar:       index = 12; key = VK_HOME;    break;
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
         cmd,value,currentTime - priv->lastUpdates[cmd], result ? "true" : "false");*/
    canCcu->lastValue[index] = value;
    if (!update)
        return;
    canCcu->lastUpdates[index] = currentTime;

    /* Send input */
    INPUT inputs[1];
    KEYBDINPUT keyinputs[1];

    keyinputs[0].wVk = key;
    keyinputs[0].dwFlags = (value == 0) ? KEYEVENTF_KEYUP : 0;

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki = keyinputs[0];

    if(!SendInput(1, inputs, sizeof(INPUT))) {
        printf("daiccu: Could not send key press!\n");
    }
#endif

    (void) canCcu;
#ifdef USE_X11
    Display *display;
    unsigned int keycode;
    display = XOpenDisplay(NULL);

    int key;
    switch (cmd) {
    case CcuUp:                 key = XK_KP_8; break;
    case CcuDown:               key = XK_KP_2; break;
    case CcuLeft:               key = XK_KP_4; break;
    case CcuRight:              key = XK_KP_6; break;
    case CcuUpLeft:             key = XK_KP_7; break;
    case CcuUpRight:            key = XK_KP_9; break;
    case CcuDownLeft:           key = XK_KP_1; break;
    case CcuDownRight:          key = XK_KP_3; break;
    case CcuSelect:             key = XK_KP_5; break;
    case CcuBack:               key = XK_BackSpace; break;
    case CcuClear:              key = XK_Delete; break;
    case CcuSeat:               key = XK_Control_L; break;
    case CcuStar:               key = XK_Control_R; break;
    case CcuMenu:               key = XK_Home; break;
    case NPad0_Pressed:         key = XK_0; break;
    case NPad1_Pressed:         key = XK_1; break;
    case NPad2_Pressed:         key = XK_2; break;
    case NPad3_Pressed:         key = XK_3; break;
    case NPad4_Pressed:         key = XK_4; break;
    case NPad5_Pressed:         key = XK_5; break;
    case NPad6_Pressed:         key = XK_6; break;
    case NPad7_Pressed:         key = XK_7; break;
    case NPad8_Pressed:         key = XK_8; break;
    case NPad9_Pressed:         key = XK_9; break;
    case NPadStar_Pressed:      key = XK_asterisk; break;
    case NPadPound_Pressed:     key = XK_numbersign; break;
    case NPadSend_Pressed:      key = XK_KP_Begin; break;
    case NPadEnd_Pressed:       key = XK_KP_End; break;
    case NPadClear_Pressed:     key = XK_Delete; break;
    case NPadFavorite_Pressed:  key = XK_Control_R; break;
    default:             return;
    }

    /*generate input*/
    keycode = XKeysymToKeycode(display, key);

    XTestFakeKeyEvent(display, keycode, value, 0);
    XFlush(display);
    XCloseDisplay(display);
#endif
}

static void processCcuRotation(void *handle, int rotate)
{
    (void) handle;

#ifdef _DEBUG
    printf("main: Process CCU rotate: %d\n", rotate);
#endif

#ifdef WIN32
    DWORD key;
    int i = 0;
    INPUT inputs[2];
    KEYBDINPUT keyinputs[2];

    /* Left or right rotation? */
    if(rotate < 0) {
        key = VK_LEFT;
        rotate = -rotate;
    } else {
        key = VK_RIGHT;
    }

    for (i = 0; i < rotate; i++) {
        keyinputs[0].wVk = key;
        keyinputs[0].dwFlags = 0;

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki = keyinputs[0];

        keyinputs[1].wVk = key;
        keyinputs[1].dwFlags = KEYEVENTF_KEYUP;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki = keyinputs[1];

        if(!SendInput(2, inputs, sizeof(INPUT))) {
            printf("daiccu: Could not send key press!\n");
        }
    }
#endif

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
}

static void stateChanged(void *handle, bool open)
{
    daiCcu *canCcu = handle;
    if (open) {
        networkManagerSendKeepAlive(handle, canCcu->ccu->version);
    }
}

static void messageReceived(void *handle, uint16_t id, uint8_t dlc,
                     const uint8_t *bytes)
{
    daiCcu *canCcu = handle;

    if (ccuProcessorProcess(canCcu->ccu, id, dlc, bytes))
        return;
}

bool canTransmit(void *handle, const uint32_t id, const uint8_t dlc,
                 const char *data)
{
    daiCcu *canCcu = handle;
    /*printf("main: Transmitt CAN message: %d (%#x)\n", id, id);*/
    CAN_PREFIX_Transmit(canCcu->can, id, dlc, data);

    return true;
}

#ifdef WIN32
DWORD WINAPI ccuRun(LPVOID handle)
#else
static void* ccuRun(void* handle)
#endif
{
    daiCcu *canCcu = handle;
    long currentTime, lastKeepAlive = 0;

    while (1) {
        if (!CAN_PREFIX_Run(canCcu->can))
            break;

        /* send cyclic keep alive message */
        currentTime = time(NULL);
        if (currentTime - lastKeepAlive >= 1) {
            if (!networkManagerSendKeepAlive(handle, canCcu->ccu->version)) {
                printf("main: Sent keep-alive message failed!\n");
            } else {
                lastKeepAlive = currentTime;
            }
        }
    }

    return 0;
}

/* settings */

void showCurrentSettings(daiCcu *canCcu)
{
    ccuProcessor *ccu = canCcu->ccu;

    printf("Current settings:\n");
    printf("1: Max rotary range: %d (1 - 9999, 0 to disable range)\n",
           ccu->maxPosition == 0xFFFF ? 0 : ccu->maxPosition);
}

static int getnumber()
{
    int number;
    return (scanf("%d", &number) == 1) ? number : -1;
}

#define DAICCU_SETTINGS "%d: %s\n"

void showSettingsMenu(daiCcu *canCcu)
{
    ccuProcessor *ccu = canCcu->ccu;
    int number = -1;

    do {
        showCurrentSettings(canCcu);

        number = getnumber();
        switch (number) {

        /* CCU */

        case 1: {
            int max, start;
            printf("Set rotatary range.\n");

            printf("Max (1 - 9999, 0 to disable range):\n");
            max = getnumber();
            if (max == -1)
                break;
            if (max == 0) {
                ccuDisableRotaryRange(ccu);
                break;
            }

            printf("Start (0 - %d):\n", max);
            start = getnumber();
            if (start == -1)
                break;
            if (start > max)
                break;
            if (!ccuSetRotaryRange(ccu, start, max))
                printf("Set rotation failed!\n");
            break;
        }

        default:
            printf("Wrong input: %d\n", number);
            return;
        }
    } while (1);

    showCurrentSettings(canCcu);
}

static void readInput(daiCcu *canCcu)
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
        case 's': {
            showSettingsMenu(canCcu);
            break;
        }
        }
    }
}

/* main */

int main(int argc, char **argv)
{
    daiCcu canCcu;
    ccuProcessor *ccu = 0;
    canDev *can = 0;

    /* Init CCU */
    ccu = ccuProcessorGet(argc, argv);
    if (!ccu) return -1;
    ccu->handle = &canCcu;
    ccu->ccuProcessCommand = processCcuCommand;
#ifdef WIN32
    /* Enable longpress */
    ccu->processWhilePressed = true;
#endif

#ifdef _DEBUG
    printf("main: Running @Star%d architecture\n", ccu->version);
#endif

    /* Init CAN */
    can = CAN_PREFIX_Get(argc, argv);
    if (!can) return -1;
    can->handle = &canCcu;
    can->canStateChanged = stateChanged;
    can->canMessageReceived = messageReceived;
    can->baudRate = (ccu->version > 0) ? 250 : 125;

    /* Init handles */
    canCcu.can = can;
    canCcu.ccu = ccu;

    if (!CAN_PREFIX_Open(canCcu.can)) {
        printf("main: Open CAN failed!\n");
        return -1;
    }

#ifdef WIN32
    CreateThread(0, 0, ccuRun, (void*) &canCcu, 0, NULL);
#else
    pthread_t canRunThread;
    pthread_create(&canRunThread, NULL, ccuRun, (void*)&canCcu);
#endif

    readInput(&canCcu);


    CAN_PREFIX_Close(canCcu.can);

    return 0;
}
