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
#ifndef CANUSB_H
#define CANUSB_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "can.h"
#include "serialport.h"

#define BUFFER_SIZE 1024

#if(defined __cplusplus)
extern "C"
{
#endif

typedef struct {
    int                   status;
    serialPort*           port;
    unsigned int          rxBufLen;
    char                  rxBuf[BUFFER_SIZE];
} canUsb;

canDev* canUsbGet(void);

bool canUsbOpen(canDev *can, int argc, char** argv);
void canUsbClose(canDev *can);
bool canUsbRun(canDev *can);

bool canUsbTransmit(canDev *can, const uint32_t id, const uint8_t dlc,
                    const char *data);

#if(defined __cplusplus)
}
#endif
#endif
