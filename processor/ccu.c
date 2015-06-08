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
#include "ccu.h"

#include "can.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* CAN @Star 0 */

#define CAN_ACTN_0_RD_MENUKEY 0x01
#define CAN_ACTN_0_RETURNKEY  0x02
#define CAN_ACTN_0_MCMSKEY    0x04
#define CAN_ACTN_0_TN_MENUKEY 0x08
#define CAN_ACTN_0_ONKEY      0x10
#define CAN_ACTN_0_DELETEKEY  0x20
#define CAN_ACTN_0_STARKEY    0x40

#define CAN_ACTN_1_CTRL_C_C   0x80

#define CAN_ACTN_2_CTRL_C_N   0X01
#define CAN_ACTN_2_CTRL_C_NE  0X02
#define CAN_ACTN_2_CTRL_C_E   0X04
#define CAN_ACTN_2_CTRL_C_SE  0X08
#define CAN_ACTN_2_CTRL_C_S   0X10
#define CAN_ACTN_2_CTRL_C_SW  0X20
#define CAN_ACTN_2_CTRL_C_W   0X40
#define CAN_ACTN_2_CTRL_C_NW  0X80

/* CAN @Star 1 & 2 */

#define CAN_STAT1_0_ONKEY     0X01
#define CAN_STAT1_0_PHONEKEY  0X02
#define CAN_STAT1_0_RADIOKEY  0X04
#define CAN_STAT1_0_SEATKEY   0X08
#define CAN_STAT1_0_NAVIKEY   0X10
#define CAN_STAT1_0_MEDIAKEY  0X20
#define CAN_STAT1_0_CARKEY    0X40
#define CAN_STAT1_0_RETURNKEY 0X80

#define CAN_STAT1_1_ECO       0X03

#define CAN_STAT1_2_CTRL_C_C  0X01
#define CAN_STAT1_2_CTRL_C_N  0X02
#define CAN_STAT1_2_CTRL_C_NE 0X04
#define CAN_STAT1_2_CTRL_C_E  0X08
#define CAN_STAT1_2_CTRL_C_SE 0X10
#define CAN_STAT1_2_CTRL_C_S  0X20
#define CAN_STAT1_2_CTRL_C_SW 0X40
#define CAN_STAT1_2_CTRL_C_W  0X80

#define CAN_STAT1_3_CTRL_C_NW 0X01
#define CAN_STAT1_3_VOLUME_RQ 0X0E
#define CAN_STAT1_3_MENUKEY   0X20
#define CAN_STAT1_3_MUTEKEY   0X40

#define CAN_STAT1_5_EJECT     0XC0

const char* ccuCommandToString(CcuCommands cmd)
{
    switch (cmd) {
    case CcuUp:                     return "up";
    case CcuDown:                   return "down";
    case CcuLeft:                   return "left";
    case CcuRight:                  return "right";
    case CcuUpLeft:                 return "up left";
    case CcuUpRight:                return "up right";
    case CcuDownLeft:               return "down left";
    case CcuDownRight:              return "down right";
    case CcuRotate:                 return "rotate";
    case CcuSelect:                 return "select";
    case CcuBack:                   return "back";
    case CcuClear:                  return "clear";
    case CcuStar:                   return "star";
    case CcuOn:                     return "ON";
    case CcuRadioDisc:              return "radio/disc";
    case CcuTelNav:                 return "tel/navi";
    case CcuSeat:                   return "seat";
    case CcuPhone:                  return "phone";
    case CcuRadio:                  return "radio";
    case CcuNavi:                   return "navi";
    case CcuMedia:                  return "media";
    case CcuCar:                    return "car";
    case CcuMenu:                   return "menu";
    case CcuMute:                   return "mute";
    case CcuEject:                  return "eject";
    case CcuECO:                    return "ECO switch";
    case CcuVolume:                 return "volume";
    case NPad0_Touched:             return "0 touched";
    case NPad0_Pressed:             return "0 pressed";
    case NPad1_Touched:             return "1 touched";
    case NPad1_Pressed:             return "1 pressed";
    case NPad2_Touched:             return "2 touched";
    case NPad2_Pressed:             return "2 pressed";
    case NPad3_Touched:             return "3 touched";
    case NPad3_Pressed:             return "3 pressed";
    case NPad4_Touched:             return "4 touched";
    case NPad4_Pressed:             return "4 pressed";
    case NPad5_Touched:             return "5 touched";
    case NPad5_Pressed:             return "5 pressed";
    case NPad6_Touched:             return "6 touched";
    case NPad6_Pressed:             return "6 pressed";
    case NPad7_Touched:             return "7 touched";
    case NPad7_Pressed:             return "7 pressed";
    case NPad8_Touched:             return "8 touched";
    case NPad8_Pressed:             return "8 pressed";
    case NPad9_Touched:             return "9 touched";
    case NPad9_Pressed:             return "9 pressed";
    case NPadStar_Touched:          return "* touched";
    case NPadStar_Pressed:          return "* pressed";
    case NPadPound_Touched:         return "# touched";
    case NPadPound_Pressed:         return "# pressed";
    case NPadSend_Touched:          return "send touched";
    case NPadSend_Pressed:          return "send pressed";
    case NPadEnd_Touched:           return "end touched";
    case NPadEnd_Pressed:           return "end pressed";
    case NPadFavorite_Touched:      return "favorite touched";
    case NPadFavorite_Pressed:      return "favorite pressed";
    case NPadClear_Touched:         return "clear touched";
    case NPadClear_Pressed:         return "clear pressed";
    case TPadBack_Touched:          return "back touched";
    case TPadBack_Pressed:          return "back pressed";
    case TPadBkGrndAudio_Touched:   return "background audio touched";
    case TPadBkGrndAudio_Pressed:   return "background audio pressed";
    case TPadMenu_Touched:          return "menu touched";
    case TPadMenu_Pressed:          return "menu pressed";
    case TPadSensorArea_Touched:    return "sensor area touched";
    case TPadSensorArea_Pressed:    return "sensor area pressed";
    case StWhlPTT:                  return "push-to-talk";
    default:                        return "unknown";
    }
}

ccuProcessor* ccuProcessorGet(int argc, char **argv)
{
    ccuProcessor *ccu = malloc(sizeof(ccuProcessor));
    int i;

    /* Default settings */
    ccu->version                = 0;
    ccu->processWhilePressed    = false;
    ccu->maxPosition            = CCU_ROTARY_SNA;
    ccu->startPosition          = CCU_ROTARY_MAX >> 1;

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--star0"))
            ccu->version = 0;

        if (!strcmp(argv[i], "--star1"))
            ccu->version = 1;

        if (!strcmp(argv[i], "--star2"))
            ccu->version = 2;
    }

    /* Init values */
    ccu->stateId                = 0xFF;
    ccu->lastPosition           = ccu->startPosition;
    ccu->lastCmdsMask           = 0;
    ccu->lastNPadMask           = 0;
    ccu->lastTPadMask           = 0;

    /* Init callbacks */
    ccu->handle                 = 0;
    ccu->ccuProcessCommand      = 0;

    return ccu;
}

static bool ccuProcessorSendStateMessageStar0(ccuProcessor *ccu)
{
    char ccuStateMessageData[8];

    ccuStateMessageData[0] = 0x01;
    /* Detents_Stat - Number of detents state (0 - 9999, SNA = 0xFFFF)*/
    if (ccu->maxPosition == CCU_ROTARY_SNA) {
        ccuStateMessageData[1] = CCU_ROTARY_MAX >> 8;
        ccuStateMessageData[2] = CCU_ROTARY_MAX & 0xFF;
    } else {
        ccuStateMessageData[1] = ccu->maxPosition >> 8;
        ccuStateMessageData[2] = ccu->maxPosition & 0xFF;
    }

    /* StPosn_Stat - Start position at activation state (0 - 9999, SNA = 0xFFFF)*/
    ccuStateMessageData[3] = ccu->startPosition >> 8;
    ccuStateMessageData[4] = ccu->startPosition & 0xFF;

    /* Local_Posn_Rq - Local position in model request (0 - 9999, SNA = 0xFFFF)*/
    ccuStateMessageData[5] = ccu->lastPosition >> 8;
    ccuStateMessageData[6] = ccu->lastPosition & 0xFF;

    /* CTRL_C_Stat_Id - Central control state ID (0 - 254, SNA = 0xFF)*/
    ccuStateMessageData[7] = ccu->stateId;

    return canTransmit(ccu->handle, 0x1bb, 8, ccuStateMessageData);
}

static bool ccuProcessorSendStateMessageStar12(ccuProcessor *ccu)
{
    char ccuStateMessageData[8];

    ccuStateMessageData[0] = ccu->stateId;

    /* Detents_Stat - Number of detents state (0 - 9999, SNA = 0xFFFF) */
    if (ccu->maxPosition == CCU_ROTARY_SNA) {
        ccuStateMessageData[2] = CCU_ROTARY_MAX >> 8;
        ccuStateMessageData[1] = CCU_ROTARY_MAX & 0xFF;
    } else {
        ccuStateMessageData[2] = ccu->maxPosition >> 8;
        ccuStateMessageData[1] = ccu->maxPosition & 0xFF;
    }

    /* Local_Posn_Rq - Local position in model request (0 - 9999, SNA = 0xFFFF) */
    ccuStateMessageData[4] = ccu->lastPosition >> 8;
    ccuStateMessageData[3] = ccu->lastPosition & 0xFF;

    /* StPosn_Rq - Start position at activation state (0 - 9999, SNA = 0xFFFF) */
    ccuStateMessageData[6] = ccu->startPosition >> 8;
    ccuStateMessageData[5] = ccu->startPosition & 0xFF;

    ccuStateMessageData[7] = 0; /* Padding */

    return canTransmit(ccu->handle, 0x1ef, 8, ccuStateMessageData);
}

bool ccuProcessorSendStateMessage(ccuProcessor *ccu)
{
    if (ccu->version == 0) {
        return ccuProcessorSendStateMessageStar0(ccu);
    } else {
        return ccuProcessorSendStateMessageStar12(ccu);
    }
}

bool ccuDisableRotaryRange(ccuProcessor *ccu)
{
    ccu->stateId++;
    if (ccu->stateId == 0xFF) ccu->stateId = 0x00;
    ccu->maxPosition = CCU_ROTARY_SNA;
    ccu->startPosition = CCU_ROTARY_MAX >> 1;
    ccu->lastPosition = ccu->startPosition;

    return ccuProcessorSendStateMessage(ccu);
}

bool ccuSetRotaryRange(ccuProcessor *ccu, int start, int max)
{
    if (start < 0 || start > CCU_ROTARY_MAX || start > max ||
        max < 0 || max > CCU_ROTARY_MAX)
        return false;

    ccu->stateId++;
    if (ccu->stateId == 0xFF) ccu->stateId = 0x00;
    ccu->maxPosition = max;
    ccu->lastPosition = start;
    ccu->startPosition = start;

    return ccuProcessorSendStateMessage(ccu);
}

#define ccu2mask(cmd) (1 << cmd)

#define ccuProcessorProcessFlags(byte, mask, cmd) \
    if (byte & mask) current |= (1 << cmd);

#define ccuProcessorProcess2Flags(byte, mask, cmd) \
    if (byte & mask && mask != (byte & mask)) current |= (1 << cmd);

static void ccuProcessorCommands(ccuProcessor *ccu, int offset, int max,
                                 uint32_t current, uint32_t changed)
{
    int cmd;
    bool isActiv;
    uint32_t mask = 1;

    for (cmd = offset; cmd <= max; cmd++) {
        isActiv = (current & mask) ? true : false;
        if ((ccu->processWhilePressed && isActiv) || (changed & mask))
            ccu->ccuProcessCommand(ccu->handle, cmd, isActiv);
        mask <<= 1;
    }
}

static bool ccuProcessorActuation(ccuProcessor *ccu, uint8_t dlc,
                                  const uint8_t *data)
{
    uint32_t current;
    uint32_t changed;

    if (dlc != 4) {
        printf("Ccu: dlc has not the expected value of 4, but %d instead\n", dlc);
        return false;
    }

    /*printf("Ccu: Actuation: %02x %02x %02x %02x\n",
           data[0], data[1], data[2], data[3]);*/

    /* Compile CCU directions mask */
    current = data[2];

    /* Compile CCU commands mask */
    ccuProcessorProcessFlags(data[0], CAN_ACTN_0_RD_MENUKEY, CcuRadioDisc);
    ccuProcessorProcessFlags(data[0], CAN_ACTN_0_RETURNKEY,  CcuBack);
    ccuProcessorProcessFlags(data[0], CAN_ACTN_0_MCMSKEY,    CcuSeat);
    ccuProcessorProcessFlags(data[0], CAN_ACTN_0_TN_MENUKEY, CcuTelNav);
    ccuProcessorProcessFlags(data[0], CAN_ACTN_0_ONKEY,      CcuOn);
    ccuProcessorProcessFlags(data[0], CAN_ACTN_0_DELETEKEY,  CcuClear);
    ccuProcessorProcessFlags(data[0], CAN_ACTN_0_STARKEY,    CcuStar);

    ccuProcessorProcessFlags(data[1], CAN_ACTN_1_CTRL_C_C,   CcuSelect);

    changed = ccu->lastCmdsMask ^ current;
    ccu->lastCmdsMask = current;

    /*printf("Ccu: CCU mask is: %04x (%04x)\n", current, changed);*/

    if (!changed && !ccu->processWhilePressed)
        return true;

    /* Process command */
    if (ccu->ccuProcessCommand)
        ccuProcessorCommands(ccu, CCU_COMMANDS_OFFSET, CCU_COMMANDS_MAX_STAR1,
                             current, changed);

    return true;
}

static bool ccuProcessorStat1(ccuProcessor *ccu, uint8_t dlc,
                              const uint8_t *data)
{
    uint32_t current = 0;
    uint32_t changed;

    if (dlc != 6 && dlc != 8) {
        printf("Ccu: dlc has not the expected value of 6 or 8 but %d instead.\n", dlc);
        return false;
    }

    /*printf("Ccu: Stat 1: %02x %02x %02x %02x %02x %02x\n",
           data[0], data[1], data[2], data[3], data[4], data[5]);*/

    // Process Volume request
    current = (data[3] & CAN_STAT1_3_VOLUME_RQ) >> 1;
    if (current != 3 && current != 0x7) {
        if (ccu->ccuProcessCommand)
            ccu->ccuProcessCommand(ccu->handle, CcuVolume, ((int)current) - 3);
    }

    /* Compile CCU directions mask */
    current = data[2] >> 1;
    if (data[3] & 0x01) current |= 0x80;

    /* Compile CCU commands mask */
    ccuProcessorProcessFlags(data[0], CAN_STAT1_0_ONKEY,     CcuOn);
    ccuProcessorProcessFlags(data[0], CAN_STAT1_0_PHONEKEY,  CcuPhone);
    ccuProcessorProcessFlags(data[0], CAN_STAT1_0_RADIOKEY,  CcuRadio);
    ccuProcessorProcessFlags(data[0], CAN_STAT1_0_SEATKEY,   CcuSeat);
    ccuProcessorProcessFlags(data[0], CAN_STAT1_0_NAVIKEY,   CcuNavi);
    ccuProcessorProcessFlags(data[0], CAN_STAT1_0_MEDIAKEY,  CcuMedia);
    ccuProcessorProcessFlags(data[0], CAN_STAT1_0_CARKEY,    CcuCar);
    ccuProcessorProcessFlags(data[0], CAN_STAT1_0_RETURNKEY, CcuBack);

    ccuProcessorProcess2Flags(data[1], CAN_STAT1_1_ECO,      CcuECO);

    ccuProcessorProcessFlags(data[2], CAN_STAT1_2_CTRL_C_C,  CcuSelect);

    ccuProcessorProcessFlags(data[3], CAN_STAT1_3_MENUKEY,   CcuMenu);
    ccuProcessorProcessFlags(data[3], CAN_STAT1_3_MUTEKEY,   CcuMute);

    ccuProcessorProcess2Flags(data[5], CAN_STAT1_5_EJECT,    CcuEject);

    changed = ccu->lastCmdsMask ^ current;
    ccu->lastCmdsMask = current;

    /*printf("Ccu: CCU mask is: %08x (%08x)\n", current, changed);*/

    if (!changed && !ccu->processWhilePressed)
        return true;

    /* Process every command */
    if (ccu->ccuProcessCommand)
        ccuProcessorCommands(ccu, CCU_COMMANDS_OFFSET, CCU_COMMANDS_MAX_STAR2,
                             current, changed);

    return true;
}

static bool ccuProcessorStat2(ccuProcessor *ccu, uint8_t dlc,
                              const uint8_t *data)
{
    uint64_t current;
    uint64_t changed;

    if (dlc != 6) {
        printf("Ccu: dlc has not the expected value of 6, but %d instead\n", dlc);
        return false;
    }

    /*printf("Ccu: Stat 2: %02x %02x %02x %02x %02x %02x\n",
           data[0], data[1], data[2], data[3], data[4], data[5]);*/

    if (!ccu->ccuProcessCommand)
        return true;

    /* Process num pad command */
    current  = data[0];
    current |= data[1] << 8;
    current |= data[2] << 16;
    current |= data[3] << 24;

    /* Ensure that pressed buttons remain touched */
    current |= (current & 0xAAAAAAAAA) >> 1;

    changed = ccu->lastNPadMask ^ current;
    ccu->lastNPadMask = current;

    /*printf("Ccu: NPad mask is: %08x (%08x)\n", current, changed);*/

    if (changed || ccu->processWhilePressed)
        ccuProcessorCommands(ccu, NPAD_COMMANDS_OFFSET, NPAD_COMMANDS_MAX,
                             current, changed);

    /* Process touch pad command */
    current = data[4];

    /* Ensure that pressed buttons remain touched */
    current |= (current & 0xAAAAAAAAA) >> 1;

    changed = ccu->lastTPadMask ^ current;
    ccu->lastTPadMask = current;

    /*printf("Ccu: TPad mask is: %02x (%02x)\n", current, changed);*/

    if (changed || ccu->processWhilePressed)
        ccuProcessorCommands(ccu, TPAD_COMMANDS_OFFSET, TPAD_COMMANDS_MAX,
                             current, changed);

    return true;
}

static bool ccuProcessorStat(ccuProcessor *ccu, uint8_t dlc,
                             const uint8_t *data)
{
    uint8_t id;
    uint16_t position;
    int rotate;

    if (dlc != 8) {
        printf("Ccu: dlc has not the expected value of 8, but %d instead\n", dlc);
        return false;
    }

    /*printf("Ccu: Stat: %02x %02x %02x %02x %02x %02x %02x %02x\n",
           data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);*/

    if (ccu->version == 0) {
        id = data[7];
        position = (data[5] << 8) + (unsigned char)data[6];
    } else {
        id = data[0];
        position = (data[4] << 8) + (unsigned char)data[3];
    }

    /* Check state ID */
    if (id == 0xFF) { /* Signal Not Available */
        if (ccu->stateId == 0xFF) {
            /* Init state ID */
            ccu->stateId = 0;
            ccuProcessorSendStateMessage(ccu);
        } else {
            printf("Ccu: State ID signal not available!\n");
        }
        return true;
    }

    if (id != ccu->stateId) {
        if (ccu->stateId != 0xFF)
            printf("Ccu: State ID mismatch!\n");
        return true;
    }

    /* Check position */
    if (position == ccu->lastPosition)
        return true;

    if (position == CCU_ROTARY_SNA) { /* Signal Not Available */
        printf("Ccu: Position signal not available!\n");
        return true;
    }

    /* Get Rotation */
    rotate = position - ccu->lastPosition;
    ccu->lastPosition = position;

    /*printf("Ccu: Stat rotation: %d.\n", rotate);*/

    /* Process rotate */
    if (ccu->ccuProcessCommand)
        ccu->ccuProcessCommand(ccu->handle, CcuRotate, rotate);

    /* Reset rotary range if needed */
    if (ccu->maxPosition == CCU_ROTARY_SNA &&
            (position < 100 || position > (CCU_ROTARY_MAX - 100)))
        ccuDisableRotaryRange(ccu);

    return true;
}

static bool ccuProcessorStar0(ccuProcessor *ccu, uint16_t id, uint8_t dlc,
                              const uint8_t *data)
{
    bool res = false;

    switch (id) {
    case 0xfd: {
        res = ccuProcessorActuation(ccu, dlc, data);
        break;
    }
    case 0xfb:
    {
        res = ccuProcessorStat(ccu, dlc, data);
        if (!ccuProcessorSendStateMessageStar0(ccu)) {
            printf("Ccu: Sent state message failed.\n");
        }
        break;
    }
    }

    return res;
}

static bool ccuProcessorStar12(ccuProcessor *ccu, uint16_t id, uint8_t dlc,
                               const uint8_t *data)
{
    bool res = false;

    switch (id) {
    case 0x1f3: {
        res = ccuProcessorStat1(ccu, dlc, data);
        break;
    }
    case 0x1f5: {
        res = ccuProcessorStat2(ccu, dlc, data);
        break;
    }
    case 0x1f7:
    {
        res = ccuProcessorStat(ccu, dlc, data);
        if (!ccuProcessorSendStateMessageStar12(ccu)) {
            printf("Ccu: Sent state message failed.\n");
        }
        break;
    }
    }

    return res;
}

bool ccuProcessorProcess(ccuProcessor *ccu, uint16_t id, uint8_t dlc,
                         const uint8_t *data)
{
    return (ccu->version == 0)
        ? ccuProcessorStar0(ccu, id, dlc, data)
        : ccuProcessorStar12(ccu, id, dlc, data);
}
