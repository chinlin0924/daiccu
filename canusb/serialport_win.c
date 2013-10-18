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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

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
    char *portName = "COM6";
    DCB settings = {0};
    COMMTIMEOUTS timeouts = {0};

    if (argc > 1)
        portName = argv[1];
    printf("SerialPort: Open serialport: %s\n", portName);

    port->serial = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, 0, NULL);
    if (port->serial == INVALID_HANDLE_VALUE) {
        printf("SerialPort: Serialport could not be opened!\n");
        return false;
    }

    /* Set timeouts in milliseconds*/
    timeouts.ReadIntervalTimeout         = 50;
    timeouts.ReadTotalTimeoutConstant    = 50;
    timeouts.ReadTotalTimeoutMultiplier  = 0;
    timeouts.WriteTotalTimeoutConstant   = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    if (!SetCommTimeouts(port->serial, &timeouts)) {
        printf("SerialPort: Setting serialport timeouts failed!\n");
        return false;
    }

    /* Initialize the DCBlength member.*/
    settings.DCBlength = sizeof(DCB);

    /* Get the default port setting information.*/
    if (!GetCommState(port->serial, &settings)) {
        printf("SerialPort: Getting serialport statefailed!\n");
        return false;
    }

    /* Configure serial port*/
    settings.BaudRate     = CBR_115200;
    settings.ByteSize     = 8;
    settings.StopBits     = ONESTOPBIT;
    settings.Parity       = NOPARITY;
    settings.fInX         = FALSE;
    settings.fOutX        = FALSE;
    settings.fOutxCtsFlow = FALSE;
    settings.fRtsControl  = RTS_CONTROL_DISABLE;
    if (!SetCommState(port->serial, &settings)) {
        printf("SerialPort: Setting serialport state failed!\n");
        return false;
    }

    /* Inform handler*/
    if (port->serialPortStateChanged)
        port->serialPortStateChanged(port->handle, true);

    return true;
}

void serialPortClose(serialPort* port)
{
    /* Inform handler*/
    if (port->serialPortStateChanged)
        port->serialPortStateChanged(port->handle, false);

    /* Close port*/
    CloseHandle(port->serial);

    /* Free memory*/
    free(port);
}

bool serialPortRun(serialPort* port)
{
    uint8_t data[512];
    unsigned long readBytes = 0;

    /* Read from serial port*/
    if (!ReadFile(port->serial, data, 512, &readBytes, 0)) {
        printf("Could not read. Error code: %d\n",(int)GetLastError());
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
    DWORD bytesWritten = 0;
    int count = 0;

    while(count < len) {
        if(!WriteFile((HANDLE) port->serial,data + count,len - count,&bytesWritten,0)) {
            printf("SerialPort: Send message failed!\n");
            return -1;
        }

        count += bytesWritten;
    }

    return count;
}
