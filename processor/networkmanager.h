#ifndef STAR_H
#define STAR_H

#include <stdbool.h>

#if(defined __cplusplus)
extern "C"
{
#endif

bool networkManagerSendKeepAlive(void *handle, int version);

#if(defined __cplusplus)
}
#endif
#endif
