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
#include "serialport.h"
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

serialPort* serialPortGet()
{
    serialPort *port = malloc(sizeof(serialPort));
    port->serial = 0;
    port->handle = 0;
    port->serialPortReceived = 0;
    port->serialPortStateChanged = 0;
    return port;
}

bool serialPortOpen(serialPort* port, int argc, char** argv)
{
    /* set default port*/
    char *portName = "/dev/ttyUSB0";
    int* device = 0;

    if (argc > 1)
        portName = argv[1];
    printf("SerialPort: Open serialport: %s\n", portName);

    /* open serialport*/
    device = malloc(sizeof(int));
    *device = open(portName, O_RDWR | O_NOCTTY | O_NDELAY);
    if (*device == -1) {
        printf("SerialPort: Serialport could not be opened!\n");
        goto error;
    }

    fcntl(*device, F_SETFL, O_RDWR);

    /* configure serialport */
    struct termios configuration;

    memset(&configuration, 0, sizeof(configuration));

    cfsetospeed(&configuration, B115200);
    cfsetispeed(&configuration, B115200);
    configuration.c_cc[VMIN]  = 1;
    configuration.c_cc[VTIME] = 5;
    configuration.c_cflag = configuration.c_cflag & ~(PARENB | PARODD);
    configuration.c_cflag = configuration.c_cflag & ~CSTOPB;
    configuration.c_cflag = configuration.c_cflag | CS8;
    configuration.c_iflag = configuration.c_iflag & ~(IXON | IXOFF | IXANY);

    if(tcsetattr(*device, TCSANOW, &configuration) != 0) {
        printf("SerialPort: Could not configure serial port!\n");
        goto error;
    }
    port->serial = (void*) device;

    /* inform handler*/
    if (port->serialPortStateChanged)
        port->serialPortStateChanged(port->handle, true);

    return true;

error:
    free(device);
    return false;
}

void serialPortClose(serialPort* port)
{
    /* Inform handler*/
    if (port->serialPortStateChanged)
        port->serialPortStateChanged(port->handle, false);

    /* Close port*/
    if (port->serial)
        close(*(int*)(port->serial));

    /* Free memory*/
    free(port->serial);

    printf("SerialPort: Serial port closed.\n");
}

bool serialPortRun(serialPort* port)
{
    uint8_t data[512];
    long readBytes = 0;

    /* Read from serial port*/
    readBytes = read(*(int*)(port->serial), data, 512);
    if (readBytes < 0) {
        perror("Could not read. Error code:\n");
        return false;
    }

    /* Bytes read???*/
    if (!readBytes)
        return true;

    /* Inform handler*/
    if (port->serialPortReceived)
        port->serialPortReceived(port->handle, readBytes, data);

    return true;
}

int serialPortTransmit(serialPort* port, const char* data, const int len)
{
    int count = 0;
    int bytesWritten = 0;
    while(count < len) {
        bytesWritten = write(*(int*)(port->serial), data, len);
        if(bytesWritten < 0) {
            printf("SerialPort: Send message failed!\n");
            return -1;
        }
        count += bytesWritten;
    }

    return count;
}
