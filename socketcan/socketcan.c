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
#include "socketcan.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

canDev* socketCanGet(void)
{
    canDev *can             = malloc(sizeof(canDev));
    can->dev                = 0;
    can->handle             = 0;
    can->canMessageReceived = 0;
    can->canStateChanged    = 0;

    return can;
}

bool socketCanRun(canDev* can)
{
    struct can_frame frame;
    size_t bytes_read;

    /* Read a message back from the CAN bus */
    bytes_read = read(*((int*)can->dev), &frame, sizeof(frame));

    if (bytes_read < 0) {
        printf("SocketCan: CAN raw socket read failed!\n");
        return false;
    }

    /* Paranoid check ... */
    if (bytes_read < sizeof(frame)) {
        printf("SocketCan: Incomplete CAN frame.\n");
        return false;
    }

    /* Notify all */
    if (can->canMessageReceived)
        can->canMessageReceived(can->handle, frame.can_id, frame.can_dlc,
                             frame.data);

    return true;
}

bool socketCanOpen(canDev* can, int argc, char** argv)
{
    /* set default network*/
    char *iface = "can0";
    int *device = 0;

    if (argc > 1)
        iface = argv[1];
    printf("SocketCan: Open CAN interface: %s\n", iface);

    /* Create the socket  */
    device = malloc(sizeof(int));
    *device = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (*device < 0) {
        printf("SocketCan: Unable to create a CAN interface: %s!\n",
               strerror(errno));
        goto error;
    }

    /* Locate the CAN interface */
    struct ifreq ifr;
    strcpy(ifr.ifr_name, iface);
    if (ioctl(*device, SIOCGIFINDEX, &ifr) < 0) {
        printf("SocketCan: Unable to locate CAN interface %s: %s!\n", iface,
               strerror(errno));
        goto error;
    }

    /* Select the CAN interface and bind the socket to it  */
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(*device, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("SocketCan: Unable to bind CAN interface to socket: %s!\n",
               strerror(errno));
        goto error;
    }
    can->dev = (void*) device;

    /* Update the interface state */
    if (can->canStateChanged)
        can->canStateChanged(can->handle, true);
    return true;

error:
    free(device);
    return false;
}

void socketCanClose(canDev* can)
{
    /* Inform handler */
    if (can->canStateChanged)
        can->canStateChanged(can->handle, false);

    /* Close socket */
    if (can->dev) {
        close(*((int*)can->dev));
        free(can->dev);
        can->dev = 0;
    }
}


bool socketCanTransmit(canDev* can, const uint32_t id, const uint8_t dlc,
                       const char *data)
{
    /* Prepare frame */
    int i, len, bytes_sent;
    struct can_frame frame;

    frame.can_id = id;
    frame.can_dlc = dlc;
    for (i = 0; i < dlc; i++) {
        frame.data[i] = data[i];
    }

    /* Send frame */
    len = sizeof(frame);
    bytes_sent = write(*((int*)can->dev), &frame, len);
    if (bytes_sent < 0) {
        int result = errno;
        printf("SocketCan: Unable to send CAN message: %s!\n",
               strerror(result));
        return false;
    }
    if (len != bytes_sent) {
        printf("SocketCan: Unable to send complete CAN message (sent %d/%d bytes)!\n",
               bytes_sent, len);
        return false;
    }

    return true;
}
