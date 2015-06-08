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
#include "touchpad.h"

#include "can.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TPAD_RQ_HU1_DEFAULT 0x0000001FFFFFFFF0 /* default parameter set */
#define TPAD_RQ_HU1_HAPTICS_MASK  0x4000000000

#define TPAD_RQ_HU2_DEFAULT 0xFFFFFFFFFFFFFFFF /* all enabled */

#define TPAD_RQ_HU3_DEFAULT 0x001F7FFFC0FFFFFF
#define TPAD_RQ_HU3_CHARSET_SHIFT 24
#define TPAD_RQ_HU3_DIR_MAX_SHIFT 53

const char* tpadGestureToString(TPadGestures gesture)
{
    switch (gesture) {
    case TPadNoGesture:     return "none";
    case TPadDragNorth:     return "drag north";
    case TPadDragNorthEast: return "drag north-east";
    case TPadDragEast:      return "drag east";
    case TPadDragSouthEast: return "drag south-east";
    case TPadDragSouth:     return "drag south";
    case TPadDragSouthWest: return "drag south-west";
    case TPadDragWest:      return "drag west";
    case TPadDragNorthWest: return "drag north-west";
    case TPadRotateCW:      return "rotate clockwise";
    case TPadRotateCCW:     return "rotate counter clockwise";
    case TPadZoomIn:        return "zoom in";
    case TPadZoomOut:       return "zoom out";
    case TPadHold:          return "hold";
    case TPadSingleClick:   return "single click";
    case TPadDoubleClick:   return "double click";
    case TPadLongPress:     return "long press";
    case TPadRelease:       return "release";
    default:                return "unknown";
    }
}

const char* tpadModeToString(TPadModes mode)
{
    switch (mode) {
    case TPadModeMenuControl:   return "menu control";
    case TPadModeCharacters:    return "character recognition";
    case TPadModeFavorite:      return "fav mode";
    case TPadModeNumPad:        return "numpad";
    case TPadModeNA:            return "not available";
    default:                    return "unknown";
    }
}


const char* tpadMaxDirectionToString(TPadMaxDirections maxDir)
{
    switch (maxDir) {
    case TPadMaxDir_4:  return "4 directions";
    case TPadMaxDir_8:  return "8 directions";
    case TPadMaxDir_16: return "16 directions";
    case TPadMaxDirNA:  return "not available";
    default:            return "unknown";
    }
}

const char* tpadCharacterSetToString(TPadCharSets set) {
    switch (set) {
    case TPadCharSet_UTF_8:                     return "All Characters";
    case TPadCharSet_ISO_IEC_8859_15:           return "Western European";
    case TPadCharSet_KOI8_R:                    return "Russian";
    case TPadCharSet_ISO_IEC_8859_2:            return "Central and Eastern European";
    case TPadCharSet_ISO_IEC_8859_9:            return "Turkish";
    case TPadCharSet_ISO_IEC_8859_7:            return "Greek and Coptic";
    case TPadCharSet_JIS:                       return "Japanese";
    case TPadCharSet_SHIFT_JIS:                 return "Shifted Japanese";
    case TPadCharSet_EUC_JP:                    return "Japanese (Extended Unix Code)";
    case TPadCharSet_EUC_KR:                    return "Hangul";
    case TPadCharSet_GB18030_2055:              return "Simplified Chinese";
    case TPadCharSet_BIG5:                      return "Traditional Chinese";
    case TPadCharSet_HKSCS_2008:                return "Hongkong";
    case TPadCharSet_UTF_8_ARABIC_PARTS:        return "Arabic";
    case TPadCharSet_ISO_8859_2_8859_9_8859_15: return "ISO/IEC 8859-2 + ISO/IEC 8859-9 + ISO/IEC 8859-15";
    case TPadCharSet_LATIN:                     return "Latin";
    case TPadCharSet_CYRILLIC:                  return "Cyrillic";
    case TPadCharSet_CYRILLIC_LATIN:            return "Cyrillic + Latin";
    case TPadCharSet_ARABIC:                    return "Arabic";
    case TPadCharSet_ARABIC_LATIN:              return "Arabic + Latin";
    case TPadCharSet_JP_FULL:                   return "JP Full";
    case TPadCharSet_JP_HIRAGANA_ONLY:          return "JP Hiragana only";
    case TPadCharSet_JP_KATAKANA_ONLY:          return "JP Katakana only";
    case TPadCharSet_JP_KANJI_ONLY:             return "JP Kanji only";
    case TPadCharSet_HANGUL:                    return "Hangul";
    case TPadCharSet_HANGUL_LATIN:              return "Hangul + Latin";
    case TPadCharSet_REDUCED_LATIN_1:           return "Reduced Latin 1 (A-Z; a-z; 0-9 + Special)";
    case TPadCharSet_REDUCED_LATIN_2:           return "Reduced Latin 2 (A-Z; 0-9 + Special)";
    case TPadCharSet_REDUCED_LATIN_3:           return "Reduced Latin 3 (A-Z; 0-9)";
    case TPadCharSet_NUMBERS_ONLY:              return "Only Numbers (0-9)";
    case TPadCharSet_THAI:                      return "Thai";
    case TPadCharSet_THAI_SPECIAL:              return "Thai + Special";
    case TPadCharSet_THAI_LATIN_SPECIAL:        return "Thai + Latin + Special";
    case TPadCharSet_HEBREW:                    return "Hebrew";
    case TPadCharSet_HEBREW_SPECIAL:            return "Hebrew + Special";
    case TPadCharSet_HEBREW_LATIN_SPECIAL:      return "Hebrew + Latin + Special";
    case TPadCharSetNA:                         return "not available";
    default:                                    return "unknown";
    }
}

const char* tpadCharacterListStateToString(TPadCharListStates state) {
    switch (state) {
    case TPadCharListNew:      return "new list";
    case TPadCharListEntry:    return "list entry";
    case TPadCharListComplete: return "complete list";
    case TPadCharListNo:       return "no list";
    default:                   return "unknown";
    }
}

const char* tpadCharacterStateToString(TPadCharStates state) {
    switch (state) {
    case TPadCharInputNo:      return "no input";
    case TPadCharInputStart:   return "start input";
    case TPadCharInputEnd:     return "end input";
    case TPadCharInputCancel:  return "cancel input";
    default:                   return "unknown";
    }
}

tpadProcessor* tpadProcessorGet(int argc, char **argv)
{
    tpadProcessor *tpad = malloc(sizeof(tpadProcessor));

    (void) argc;
    (void) argv;

    /* Default settings */
    tpad->mode      = TPadModeMenuControl;
    tpad->haptics   = true;
    tpad->maxDir    = TPadMaxDirNA;
    tpad->charSet   = TPadCharSet_UTF_8;

    /* Init values */
    tpad->id                = 0xFF;
    tpad->x                 = -1;
    tpad->y                 = -1;
    tpad->touched           = false;
    tpad->storedGesture[0]  = 0xFF;
    tpad->lastStatGesture   = TPadGestureNA;
    tpad->lastDragGesture   = TPadGestureNA;
    tpad->lastRotateGesture = TPadGestureNA;
    tpad->lastZoomGesture   = TPadGestureNA;
    tpad->lastHoldGesture   = TPadGestureNA;
    tpad->lastConfigMsg     = 0;

    /* Init callbacks */
    tpad->handle                    = 0;
    tpad->tpadProcessTouch          = 0;
    tpad->tpadProcessGesture        = 0;
    tpad->tpadProcessCharacter      = 0;

    return tpad;
}

#define tpadProcessorGesture(lastGesture) \
    if (gesture > TPadNoGesture) \
        tpad->tpadProcessGesture(tpad->handle, tpad->id, gesture, fingers, \
                                 x, y, speed); \
    else if (gesture == TPadNoGesture && lastGesture > TPadNoGesture) \
        tpad->tpadProcessGesture(tpad->handle, tpad->id, lastGesture, \
                                 0, -1, -1, 0); \
    lastGesture = gesture; \

static bool tpadProcessorDynGestures(tpadProcessor *tpad, const uint8_t *data);

static bool tpadProcessorGesture1(tpadProcessor *tpad, uint8_t dlc,
                                  const uint8_t *data)
{
    int gesture;
    int fingers;
    int x, y, speed = 0;

    if(dlc != 8) {
        printf("Tpad: dlc has not the expected value of 8, but %d instead.\n", dlc);
        return false;
    }

    /*printf("Tpad: Gesture 1: %02x %02x %02x %02x %02x %02x %02x %02x\n",
           data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);*/

    /* State id */
    tpad->id = data[0];
    if (tpad->id == 0xFF) return true;

    /* Barycenter coordinate */
    tpad->x = data[2];
    tpad->x |= ((uint16_t)(data[3] & 0x3)) << 8;
    if (tpad->x == 1023) tpad->x = -1; /* Signal Not Available */

    tpad->y = (data[3] >> 2) & 0x3F;
    tpad->y |= ((uint16_t)(data[4] & 0x0F)) << 6;
    if (tpad->y == 1023) tpad->y = -1; /* Signal Not Available */

    /* Number of fingers */
    fingers = data[1] & 0xF;
    if (fingers == 0) {
        if (tpad->touched) {
            tpad->touched = false;
            if (tpad->tpadProcessTouch)
                tpad->tpadProcessTouch(tpad->handle, tpad->id, tpad->x, tpad->y,
                                       0);
        }
    } else if (fingers == 14 /* INV_TOUCH */
               || fingers == 0xF /* Signal Not Available */) {
        /* do nothing */
    } else {
        tpad->touched = true;
        if (tpad->tpadProcessTouch)
            tpad->tpadProcessTouch(tpad->handle, tpad->id, tpad->x, tpad->y,
                                   fingers);
    }

    if (!tpad->tpadProcessGesture) return true;

    /* Process stored dynamic gestures */
    if (tpad->storedGesture[0] == tpad->id)
        tpadProcessorDynGestures(tpad, tpad->storedGesture);

    /* Process stationary gesture */

    /* Stationary gesture */
    gesture = (data[1] >> 4) & 0x0F;
    switch (gesture) {
    case 0:  gesture = TPadNoGesture;   fingers = 0; break; /* NO_GEST */
    case 1:  gesture = TPadSingleClick; fingers = 1; break; /* SG_CLICK_1 */
    case 2:  gesture = TPadSingleClick; fingers = 2; break; /* SG_CLICK_2 */
    case 3:  gesture = TPadDoubleClick; fingers = 1; break; /* DB_CLICK_1 */
    case 4:  gesture = TPadDoubleClick; fingers = 2; break; /* DB_CLICK_2 */
    case 5:  gesture = TPadLongPress;   fingers = 1; break; /* LPSD_1 */
    case 6:  gesture = TPadLongPress;   fingers = 2; break; /* LPSD_2 */
    case 7:  gesture = TPadRelease;     fingers = 1; break; /* REL_1 */
    case 8:  gesture = TPadRelease;     fingers = 2; break; /* REL_2 */
    default: gesture = TPadGestureNA;   fingers = 0;
    }

    /* Gesture coordinate */
    x = (data[4] >> 4) & 0x0F;
    x |= ((uint16_t) (data[5] & 0x3F)) << 4;
    if (x == 1023) x = -1; /* Signal Not Available */

    y = (data[5] >> 6) & 0x3;
    y |= ((uint16_t) (data[6])) << 2;
    if (y == 1023) y = -1; /* Signal Not Available */

    tpadProcessorGesture(tpad->lastStatGesture);

    return true;
}

static bool tpadProcessorGesture2(tpadProcessor *tpad, uint8_t dlc,
                                  const uint8_t *data)
{

    if (dlc != 8) {
        printf("Tpad: dlc has not the expected value of 8, but %d instead.\n", dlc);
        return false;
    }

    /*printf("Tpad: Gesture 2: %02x %02x %02x %02x %02x %02x %02x %02x\n",
           data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);*/

    /* Mode */
    if ((data[2] & 0xF0) == 0xF0)
        return true;

    /* State id */
    if (data[0] == 0xFF) return true;

    if (data[0] == tpad->id) {
        /* invalidate stored gesture */
        tpad->storedGesture[0] = 0xFF;
        tpadProcessorDynGestures(tpad, data);
    } else {
        /* store gesture and parse later */
        memcpy(tpad->storedGesture, data, 8);
    }

    return true;
}

static bool tpadProcessorDynGestures(tpadProcessor *tpad, const uint8_t *data)
{
    TPadGestures gesture;
    uint8_t fingers;
    uint8_t value;
    int x = tpad->x;
    int y = tpad->y;
    int speed;

    if (!tpad->tpadProcessGesture) return true;

    /* Process drag gesture */
    value = data[1] & 0x1F;
    speed = ((uint16_t)data[3]) | (((uint16_t)(data[4] & 0x03)) << 8);
    if (value == 0) { /* NO_DRAG */
        gesture = TPadNoGesture;
        fingers = 0;
    } else if (value == 0x1F) { /* Signal Not Available */
        gesture = TPadGestureNA;
        fingers = 0;
    } else { /* Calculate gesture */
        if ((value % 2) == 0) {
            gesture = value >> 1;
            fingers = 2;
        } else {
            gesture = (value >> 1) + 1;
            fingers = 1;
        }
    }
    tpadProcessorGesture(tpad->lastDragGesture);

    /* Process rotate gesture */
    value = (data[1] >> 5) & 0x7;
    speed = ((uint16_t)((data[4] >> 2) & 0x3F))
            | (((uint16_t)(data[5] & 0x0F)) << 6);
    switch (value) {
    case 0:  gesture = TPadNoGesture; fingers = 0; break; /* NO_ROTATE */
    case 1:  gesture = TPadRotateCW;  fingers = 1; break; /* ROTATE_CW_1 */
    case 2:  gesture = TPadRotateCW;  fingers = 2; break; /* ROTATE_CW_2 */
    case 3:  gesture = TPadRotateCCW; fingers = 1; break; /* ROTATE_CCW_1 */
    case 4:  gesture = TPadRotateCCW; fingers = 2; break; /* ROTATE_CCW_2 */
    default: gesture = TPadGestureNA; fingers = 0;
    }
    tpadProcessorGesture(tpad->lastRotateGesture);

    /* Process zoom gesture */
    value = data[2] & 0x3;
    speed = ((uint16_t)((data[5] >> 4) & 0x0F))
            | (((uint16_t)(data[6] & 0x3F)) << 4);
    switch (value) {
    case 0:  gesture = TPadNoGesture; fingers = 0; break; /* NO_ZOOM */
    case 1:  gesture = TPadZoomIn;    fingers = 2; break; /* ZOOM_IN */
    case 2:  gesture = TPadZoomOut;   fingers = 2; break; /* ZOOM_OUT */
    default: gesture = TPadGestureNA; fingers = 0;
    }
    tpadProcessorGesture(tpad->lastZoomGesture);

    /* Process hold gesture */
    value = (data[2] >> 2) & 0x3;
    speed = 0;
    switch (value) {
    case 0:  gesture = TPadNoGesture; fingers = 0; break; /* NO_HOLD */
    case 1:  gesture = TPadHold;      fingers = 1; break; /* HOLD_1 */
    case 2:  gesture = TPadHold;      fingers = 2; break; /* HOLD_2 */
    default: gesture = TPadGestureNA; fingers = 0;
    }
    tpadProcessorGesture(tpad->lastHoldGesture);

    return true;
}

static bool tpadProcessorCharacters(tpadProcessor *tpad, uint8_t dlc,
                                    const uint8_t *data)
{
    uint8_t id;
    unsigned int character;
    TPadCharSets charSet;
    uint8_t probability;
    /*TPadCharStates state;*/
    TPadCharListStates listState;
    uint8_t listEntry;

    if (dlc != 8) {
        printf("Tpad: dlc has not the expected value of 8, but %d instead.\n", dlc);
        return false;
    }

    /*printf("Tpad: Characters: %02x %02x %02x %02x %02x %02x %02x %02x\n",
           data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);*/

    /* State id */
    id = data[0];
    if (id == 0xFF) return true;

    /* List state */
    listState = (TPadCharListStates)((data[5] >> 6) & 0x3);
    if (listState == TPadCharListNo) return true;

    /* Character (little endian) */
    character = (uint32_t) data[1];
    character |= ((uint32_t) data[2]) << 8;
    character |= ((uint32_t) data[3]) << 16;
    character |= ((uint32_t) data[4]) << 24;

    /* Character set */
    charSet = (TPadCharSets)(data[5] & 0x3F);

    /* List probability */
    probability = data[6] & 0x7F;

    /* List entry */
    listEntry = data[7] & 0x0F;

    /* Character input state */
    /*state = (TPadCharStates)((data[7] >> 4) & 0x07);*/

    if (tpad->tpadProcessCharacter)
        tpad->tpadProcessCharacter(tpad->handle, id, listState, listEntry,
                                   character, charSet, probability);

    return true;
}

bool tpadProcessorProcess(tpadProcessor *tpad, uint16_t id, uint8_t dlc,
                          const uint8_t *data)
{
    bool res = false;
    switch (id) {
    case 0x0c6: {
        res = tpadProcessorGesture1(tpad, dlc, data);
        break;
    }
    case 0x0c8: {
        res = tpadProcessorGesture2(tpad, dlc, data);
        break;
    }
    case 0x0ca: {
        res = tpadProcessorCharacters(tpad, dlc, data);
        break;
    }
    }

    /* send cyclic configuration messages */
    if (res) {
        long currentTime = time(NULL);
        if (currentTime - tpad->lastConfigMsg >= 1) {
            tpadProcessorSendTouchConfigureMessage1(tpad);
            tpadProcessorSendTouchConfigureMessage2(tpad);
            tpadProcessorSendTouchConfigureMessage3(tpad);
            tpad->lastConfigMsg = currentTime;
        }
    }

    return res;
}

bool tpadSetMode(tpadProcessor *tpad, TPadModes mode)
{
    tpad->mode = mode;
    tpadProcessorSendTouchConfigureMessage1(tpad);
    return true;
}

bool tpadSetHaptics(tpadProcessor *tpad, bool enabled)
{
    tpad->haptics = enabled;
    tpadProcessorSendTouchConfigureMessage1(tpad);
    return true;
}

bool tpadSetMaxDirections(tpadProcessor *tpad, TPadMaxDirections maxDir)
{
    tpad->maxDir = maxDir;
    tpadProcessorSendTouchConfigureMessage3(tpad);
    return true;
}

bool tpadSetCharacterSet(tpadProcessor *tpad, TPadCharSets charSet)
{
    tpad->charSet = charSet;
    tpadProcessorSendTouchConfigureMessage3(tpad);
    return true;
}

static void longToArray(uint64_t num, uint8_t *data)
{
    int i = 0;
    for (i = 0; i < 8; i++) {
        data[i] = num & 0xFF;
        num >>= 8;
    }
}

bool tpadProcessorSendTouchConfigureMessage1(tpadProcessor *tpad)
{
    char data[8];

    uint64_t config = TPAD_RQ_HU1_DEFAULT;
    config |= tpad->mode;
    if (tpad->haptics)
        config |= TPAD_RQ_HU1_HAPTICS_MASK;

    longToArray(config, (uint8_t*)data);
    return canTransmit(tpad->handle, 0x471, 8, data);
}

bool tpadProcessorSendTouchConfigureMessage2(tpadProcessor *tpad)
{
    char data[8];

    longToArray(TPAD_RQ_HU2_DEFAULT, (uint8_t*)data);
    return canTransmit(tpad->handle, 0x2ec, 8, data);
}

bool tpadProcessorSendTouchConfigureMessage3(tpadProcessor *tpad)
{
    char data[8];

    uint64_t config = TPAD_RQ_HU3_DEFAULT;
    config |= ((uint64_t) tpad->charSet) << TPAD_RQ_HU3_CHARSET_SHIFT;
    config |= ((uint64_t) tpad->maxDir) << TPAD_RQ_HU3_DIR_MAX_SHIFT;

    longToArray(config, (uint8_t*)data);
    return canTransmit(tpad->handle, 0x3ee, 8, data);
}
