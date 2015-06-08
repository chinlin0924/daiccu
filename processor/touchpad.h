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
#ifndef TPADPROCESSOR_H
#define TPADPROCESSOR_H

#include <stdbool.h>
#include <stdint.h>

#if(defined __cplusplus)
extern "C"
{
#endif

/* Definitions */

typedef enum {
    TPadGestureNA       = -1,
    TPadNoGesture       = 0,
    /* Dynamic Gestures */
    TPadDragNorth       = 1,
    TPadDragNorthEast   = 2,
    TPadDragEast        = 3,
    TPadDragSouthEast   = 4,
    TPadDragSouth       = 5,
    TPadDragSouthWest   = 6,
    TPadDragWest        = 7,
    TPadDragNorthWest   = 8,
    TPadRotateCW        = 9,
    TPadRotateCCW       = 10,
    TPadZoomIn          = 11,
    TPadZoomOut         = 12,
    TPadHold            = 13,
    /* Stationary Gestures */
    TPadSingleClick     = 14,
    TPadDoubleClick     = 15,
    TPadLongPress       = 16,
    TPadRelease         = 17
} TPadGestures;

typedef enum {
    TPadModeMenuControl = 0,
    TPadModeCharacters  = 1,
    TPadModeNumPad      = 2,
    TPadModeFavorite    = 3,
    TPadModeNA          = 0xF
} TPadModes;

typedef enum {
    TPadMaxDir_4     = 0x0,
    TPadMaxDir_8     = 0x1,
    TPadMaxDir_16    = 0x2,
    TPadMaxDirNA     = 0x7
} TPadMaxDirections;

typedef enum {
    TPadCharSet_UTF_8                       = 0,
    TPadCharSet_ISO_IEC_8859_15             = 1,
    TPadCharSet_KOI8_R                      = 2,
    TPadCharSet_ISO_IEC_8859_2              = 3,
    TPadCharSet_ISO_IEC_8859_9              = 4,
    TPadCharSet_ISO_IEC_8859_7              = 5,
    TPadCharSet_JIS                         = 6,
    TPadCharSet_SHIFT_JIS                   = 7,
    TPadCharSet_EUC_JP                      = 8,
    TPadCharSet_EUC_KR                      = 9,
    TPadCharSet_GB18030_2055                = 10,
    TPadCharSet_BIG5                        = 11,
    TPadCharSet_HKSCS_2008                  = 12,
    TPadCharSet_UTF_8_ARABIC_PARTS          = 13,
    TPadCharSet_ISO_8859_2_8859_9_8859_15   = 14,
    TPadCharSet_LATIN                       = 15,
    TPadCharSet_CYRILLIC                    = 16,
    TPadCharSet_CYRILLIC_LATIN              = 17,
    TPadCharSet_ARABIC                      = 18,
    TPadCharSet_ARABIC_LATIN                = 19,
    TPadCharSet_JP_FULL                     = 20,
    TPadCharSet_JP_HIRAGANA_ONLY            = 21,
    TPadCharSet_JP_KATAKANA_ONLY            = 22,
    TPadCharSet_JP_KANJI_ONLY               = 23,
    TPadCharSet_HANGUL                      = 24,
    TPadCharSet_HANGUL_LATIN                = 25,
    TPadCharSet_REDUCED_LATIN_1             = 26,
    TPadCharSet_REDUCED_LATIN_2             = 27,
    TPadCharSet_REDUCED_LATIN_3             = 28,
    TPadCharSet_NUMBERS_ONLY                = 29,
    TPadCharSet_THAI                        = 30,
    TPadCharSet_THAI_SPECIAL                = 31,
    TPadCharSet_THAI_LATIN_SPECIAL          = 32,
    TPadCharSet_HEBREW                      = 33,
    TPadCharSet_HEBREW_SPECIAL              = 34,
    TPadCharSet_HEBREW_LATIN_SPECIAL        = 35,
#define TPAD_CHARSET_MAX 35
    TPadCharSetNA                           = 0x3F
} TPadCharSets;

typedef enum {
    TPadCharListNew      = 0,
    TPadCharListEntry    = 1,
    TPadCharListComplete = 2,
    TPadCharListNo       = 3
} TPadCharListStates;

typedef enum {
    TPadCharInputNo     = 0,
    TPadCharInputStart  = 1,
    TPadCharInputEnd    = 2,
    TPadCharInputCancel = 3
} TPadCharStates;

/* Functional */

typedef void (*tpadProcessTouchProc)(void *handle, int id, int x, int y,
                                     int fingers);

typedef void (*tpadProcessGestureProc)(void *handle, int id,
                                       TPadGestures gesture, int fingers,
                                       int x, int y, int speed);

typedef void (*tpadProcessCharacterProc) (void *handle, int id,
                                          TPadCharListStates state,
                                          int entry, unsigned int character,
                                          TPadCharSets charSet,
                                          int probability);

typedef struct {
    uint8_t id;
    int x, y;
    bool touched;
    uint8_t storedGesture[8];
    TPadGestures lastStatGesture;
    TPadGestures lastDragGesture;
    TPadGestures lastRotateGesture;
    TPadGestures lastZoomGesture;
    TPadGestures lastHoldGesture;
    long lastConfigMsg;

    void *handle;
    TPadModes mode;
    bool haptics;
    TPadMaxDirections maxDir;
    TPadCharSets charSet;

    tpadProcessTouchProc tpadProcessTouch;
    tpadProcessGestureProc tpadProcessGesture;
    tpadProcessCharacterProc tpadProcessCharacter;
} tpadProcessor;

tpadProcessor* tpadProcessorGet(int argc, char **argv);
bool tpadProcessorProcess(tpadProcessor *tpad, uint16_t id, uint8_t dlc,
                          const uint8_t *data);

/* To string */

const char* tpadGestureToString(TPadGestures gesture);
const char* tpadModeToString(TPadModes mode);
const char* tpadMaxDirectionToString(TPadMaxDirections maxDir);
const char* tpadCharacterStateToString(TPadCharStates state);
const char* tpadCharacterSetToString(TPadCharSets charSet);
const char* tpadCharacterListStateToString(TPadCharListStates state);

/* Configuration */

bool tpadSetMode(tpadProcessor *tpad, TPadModes mode);
bool tpadSetHaptics(tpadProcessor *tpad, bool enabled);
bool tpadSetMaxDirections(tpadProcessor *tpad, TPadMaxDirections maxDir);
bool tpadSetCharacterSet(tpadProcessor *tpad, TPadCharSets charSet);

bool tpadProcessorSendTouchConfigureMessage1(tpadProcessor *tpad);
bool tpadProcessorSendTouchConfigureMessage2(tpadProcessor *tpad);
bool tpadProcessorSendTouchConfigureMessage3(tpadProcessor *tpad);

#if(defined __cplusplus)
}
#endif
#endif
