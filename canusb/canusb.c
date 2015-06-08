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
#include "canusb.h"

#include <stdio.h>
#include <string.h>

#ifndef strndup
char *strndup(const char *s, size_t n)
{
    char *ret = malloc(n+1);
    strncpy(ret, s, n);
    ret[n] = '\0';
    return ret;
}
#endif

enum {
    NotReady    = 0,
    Ready       = 1,
    Configuring = 2,
    Configured  = 3,
    Opening     = 4,
    Closing     = 5,
    Open        = 6
} CanStatus;

#ifdef _DEBUG
static void replaceCharInString(char *string, const char replacementChar,
                                const char charToBeReplaced) 
{
    char *pointerToCharOccurence;
    pointerToCharOccurence = strchr(string, (int)charToBeReplaced);
    while (pointerToCharOccurence != NULL) {
        *pointerToCharOccurence = replacementChar;
        pointerToCharOccurence = strchr(string, (int)charToBeReplaced);
    }
}
#endif

static const char* toCanUsbBaudRate(const uint32_t baudRate)
{
    switch (baudRate) {
    case 10:     return "S0\r";
    case 20:     return "S1\r";
    case 50:     return "S2\r";
    case 100:    return "S3\r";
    case 125:    return "S4\r";
    case 250:    return "S5\r";
    case 500:    return "S6\r";
    case 800:    return "S7\r";
    case 1000:   return "S8\r";
    default:     return "S4\r";
    }
}

static bool canUsbWrite(canDev *can, const char *msg)
{
    canUsb *dev = can->dev;
    int tx = -1;
    int len = strlen(msg);
    int count = 0;

#ifdef _DEBUG
    char printableMsg[1024];
    strcpy(printableMsg, msg);
    replaceCharInString(printableMsg, ';', '\r');
    printf("CanUsb: Write msg: %s\n", printableMsg);
#endif

    while (true) {
        tx = serialPortTransmit(dev->port, (msg + count), len - count);
        if (tx == -1) {
            printf("CanUsb: Writing to Serial Port failed!");
            return false;
        }
        count += tx;
        if (count == len) {
            break;
        }
    }

    return true;
}

static void canUsbConfigure(canDev *can)
{
    canUsb *dev = can->dev;

    printf("CanUsb: Configure CAN.\n");

    dev->status = Configuring;
    if (!canUsbWrite(can, toCanUsbBaudRate(can->baudRate))) {
        dev->status = Ready;
        printf("CanUsb: Configure CAN failed!\n");
    }
}

static void canUsbOpen_(canDev *can)
{
    canUsb *dev = can->dev;

    printf("CanUsb: Open CAN.\n");

    dev->status = Opening;
    if (!canUsbWrite(can, "O\r")) {
        dev->status = Configured;
        printf("CanUsb: Open CAN failed!\n");
    }
}

static int canUsbProcessData(canDev *can, char *msg)
{
    canUsb *dev = can->dev;
    char *tmp;
    uint16_t id;
    uint8_t dlc;
    uint8_t *bytes = 0;

#ifdef _DEBUG
    char printableMsg[1024];
    strcpy(printableMsg,msg);
    replaceCharInString(printableMsg, ';', '\r');
    printf("CanUsb: Process Msg: %s\n", printableMsg);
#endif

    switch (msg[0]) {
    case '\a': {
        /* Check for BELL*/
        /*printf("CanUsb: Error received\n");*/

        /* Change status*/
        switch (dev->status) {
        case Configuring: {
            dev->status = Ready;
            printf("CanUsb: Configure CAN failed! Try again...\n");
            break;
        }
        case Opening: {
            dev->status = Configured;
            printf("CanUsb: Open CAN failed! Try again...\n");
            break;
        }
        case Closing: {
            dev->status = Open;
            printf("CanUsb: Close CAN failed!\n");
            break;
        }
        case Configured: {
            printf("CanUsb: Received error in 'Configured' status.\n");
            break;
        }
        case Open: {
            printf("CanUsb: Send CAN message failed.\n");
            break;
        }
        case Ready: {
            /*printf("CanUsb: Received error in 'Ready' status\n");*/
            break;
        }
        default:
            break;
        }

        return 1;
    }
    case '\r': {
        /* OK received*/
        printf("CanUsb: OK received\n");

        /* Change status*/
        switch(dev->status) {
        case Configuring: {
            dev->status = Configured;
            printf("CanUsb: Configure CAN succeed.\n");
            canUsbOpen_(can);
            break;
        }
        case Opening: {
            dev->status = Open;
            printf("CanUsb: Open CAN succeed.\n");
            if (can->canStateChanged) {
                can->canStateChanged(can->handle, true);
            }
            break;
        }
        case Closing: {
            dev->status = Configured;
            printf("CanUsb: Close CAN succeed.\n");
            if (can->canStateChanged) {
                can->canStateChanged(can->handle, false);
            }
            break;
        }
        default:
            break;
        }

        return 1;
    }
    case '?': {
        /* Check for '?'*/
        printf("CanUsb: Question mark received\n");
        return 1;
    }
    }

    /* Search for CR*/
    char* end = strchr(msg, '\r');
    if (!end) {
        /*printf("CanUsb: Message not complete! MSG: %s\n", msg);*/
        return -1;
    }

    /* Parse message*/
    switch (msg[0]) {
    case 't': {
        char *ptr = msg + 1;

        if (dev->status < Open) {
            break;
        }

        /* Parse CAN message*/
        if ((end - msg) < 5) {
#ifdef _DEBUG
            printf("CanUsb: Malformed message received! MSG: %s\n", printableMsg);
#else
            printf("CanUsb: Malformed message received!\n");
#endif
            break;
        }

        /* Parse id*/
        tmp = strndup(ptr, 3);
        id = strtol(tmp, NULL, 16);
        free(tmp);
        ptr += 3;

        /* Parse dlc*/
        tmp = strndup(ptr, 1);
        dlc = atoi(tmp);
        free(tmp);
        ptr++;

        /* Check dlc*/
        if ((end - ptr) != dlc * 2) {
#ifdef _DEBUG
            printf("CanUsb: DLC not correct! MSG: %s\n", printableMsg);
#else
            printf("CanUsb: DLC not correct!\n");
#endif
            break;
        }

        /* Parse bytes*/
        if (dlc) {
            int count = 0;
            bytes = malloc(dlc);
            if (!bytes) {
                printf("CanUsb: Malloc failed!\n");
                return end - msg + 1;
            }
            while (count < dlc) {
                tmp = strndup(ptr, 2);
                bytes[count] = strtol(tmp, NULL, 16);
                free(tmp);
                ptr += 2;
                count++;
            }
        }

        if (can->canMessageReceived) {
            can->canMessageReceived(can->handle, id, dlc, bytes);
        }

        free(bytes);
        break;
    }
    case 'V': {
        tmp = strndup(msg + 1, 4);
        printf("CanUsb: Version received: %s\n", tmp);
        free(tmp);
        canUsbConfigure(can);
        break;
    }
    case 'z': {
        /*printf("CanUsb: Send CAN message succeed.\n");*/
        break;
    }
    default:
#ifdef _DEBUG
        printf("CanUsb: Unknown message received! MSG: %s\n", printableMsg);
#else
        printf("CanUsb: Unknown message received!\n");
#endif
    }

    return end - msg + 1;
}

static void canUsbReceivedData(void *handle, unsigned int len,
                               const uint8_t *data)
{
    canDev *can = handle;
    canUsb *dev = can->dev;
    unsigned int pos = 0;
    int bytesProcessed;

    if (dev->rxBufLen + len > BUFFER_SIZE - 1) {
        printf("CanUsb: Buffer is full! Discard message...\n");
        dev->rxBufLen = 0;
        return;
    }

    memcpy(dev->rxBuf + dev->rxBufLen, data, len);
    dev->rxBufLen += len;
    dev->rxBuf[dev->rxBufLen] = '\0';

    /* Process data*/
    /*printf("CanUsb: Processing messages of a total length of %d\n", len);*/
    while (pos < dev->rxBufLen) {
        bytesProcessed = canUsbProcessData(can, dev->rxBuf + pos);
        /*printf("CanUsb: Processed %d characters.\n", bytesProcessed);*/
        /* if bytes were processed, these bytes can be erased from buffer.*/
        if (bytesProcessed == -1) {
            dev->rxBufLen -= pos;
            memmove(dev->rxBuf, dev->rxBuf + pos, dev->rxBufLen + 1);
            return;
        }
        pos += bytesProcessed;
    }
    /* all data from buffer is processed and can be deleted;*/
    /*printf("CanUsb: Processed all data from buffer.\n");*/
    dev->rxBufLen = 0;
}

static void canUsbStateChanged(void *handle, bool active)
{
    canDev *can = handle;
    canUsb *dev = can->dev;

    if (active) {
        dev->status = Ready;
        /* Update the interface state.*/
        printf("CanUsb: Serial port was turned on.\n");
        canUsbWrite(can, "\r\r\rC\rV\r");
    } else {
        dev->status = NotReady;
        /* Update the interface state.*/
        printf("CanUsb: Serial port was turned off.\n");
    }
}

canDev* canUsbGet(int argc, char **argv)
{
    int i;
    canDev *can = malloc(sizeof(canDev));
    canUsb *dev = malloc(sizeof(canUsb));

    dev->port     = 0;
    dev->status   = NotReady;
    dev->port     = 0;
    dev->rxBufLen = 0;
    dev->dev      = 0;

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--canusb")) {
            i++;
            if (i < argc) {
                dev->dev = argv[i];
                printf("CanUsb: Use device: %s\n", dev->dev);
            } else {
                printf("CanUsb: Usage %s --canusb <device>\n", argv[0]);
                return 0;
            }
        }
    }

    can->dev                = dev;
    can->baudRate           = 125;
    can->handle             = 0;
    can->canMessageReceived = 0;
    can->canStateChanged    = 0;

    return can;
}

bool canUsbOpen(canDev *can)
{
    canUsb *dev = can->dev;

    printf("CanUsb: Open CANUSB\n");

    /* Check if already running*/
    if (dev->port) {
        printf("CanUsb: Serial port already initialized!\n");
        return false;
    }

    /* Get serial port*/
    dev->port = serialPortGet();
    dev->port->handle = can;
    dev->port->serialPortReceived = canUsbReceivedData;
    dev->port->serialPortStateChanged = canUsbStateChanged;

    /* Open serial port*/
    if (serialPortOpen(dev->port, dev->dev)) {
        printf("CanUsb: Listening for data\n");
        return true;
    } else {
        printf("CanUsb: Device failed to open!\n");
        return false;
    }
}

void canUsbClose(canDev *can)
{
    canUsb *dev = can->dev;

    printf("CanUsb: Close CANUSB\n");

    if (dev->port) {
        /* Close CAN*/
        if (dev->status == Open) {
            canUsbWrite(can, "C\r");
        }

        /* Close serial port*/
        serialPortClose(dev->port);
        dev->port = 0;
    }
    dev->status = NotReady;

    if (can->canStateChanged) {
        can->canStateChanged(can->handle, false);
    }

    free(dev);
    free(can);
}


bool canUsbTransmit(canDev *can, const uint32_t id, const uint8_t dlc,
                    const char *data)
{
    canUsb *dev = can->dev;
    int len = 5 + (2 * dlc) + 1;
    char msg[len + 1];
    int count = 0;

    if (dev->status != Open) {
        printf("CanUsb: CAN not open!\n");
        return false;
    }

    /* Build message*/
    sprintf(msg, "t%03x%1d", id, dlc);
    while (count < dlc) {
        sprintf(&msg[5 + (2 * count)], "%02x", (unsigned char)data[count]);
        count++;
    }
    msg[len - 1] = '\r';
    msg[len] = '\0';

    /*printf("CanUsb: Transmit CAN message %s\n",msg);*/

    /* Send message*/
    if (!canUsbWrite(can, msg)) {
        printf("CanUsb: Transmit CAN message failed!\n");
        return false;
    }

    return true;
}


bool canUsbRun(canDev *can)
{
    canUsb *dev = can->dev;
    return serialPortRun(dev->port);
}
