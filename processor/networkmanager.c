#include "networkmanager.h"
#include "can.h"

static const char nmKeepAliveDataStar0[8] = { 0xFD, 0x00, 0x00, 0x00,
                                              0x00, 0x00, 0x00, 0x0E };

static const char nmKeepAliveDataStar12[8] = { 0x00, 0x00, 0x00, 0x00,
                                               0x03, 0x00, 0x00, 0x00 };

bool networkManagerSendKeepAlive(void *handle, int version)
{
    return (version == 0)
        ? canTransmit(handle, 0x43f, 8, nmKeepAliveDataStar0)
        : canTransmit(handle, 0x57f, 8, nmKeepAliveDataStar12);
}
