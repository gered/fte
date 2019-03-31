/*    h_catbs.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_HILIT_CATBS

#define hsBS_Normal 1

// this is for viewing only, do not try to edit or anything.

int Hilit_CATBS(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine *Line, hlState &State, hsState *StateMap, int *ECol) {
    ChColor *Colors = BF->Mode->fColorize->Colors;
    HILIT_VARS(Colors[CLR_Normal], Line);
    int CL = 0; //(LN == BF->VToR(BF->CP.Row)) ? 1 : 0;
    
    for (i = 0; i < Line->Count;) {
        IF_TAB() else {
            switch (State) {
            default:
            case hsBS_Normal:
                Color = Colors[CLR_Normal];
                while (!CL && len >= 2 && p[1] == '\b') {
                    if (len > 2 && p[0] == p[2]) { // bold
                        Color = Colors[CLR_Keyword];
                        NextChar();
                        NextChar();
                        C -= 2;
                    } else if (p[0] == '_') { // underline
                        Color = Colors[CLR_Symbol];
                        NextChar();
                        NextChar();
                        C -= 2;
                        break;
                    } else
                        break;
                }
                ColorNext();
                continue;
            }
        }
    }
    *ECol = C;
    return 0;
}
#endif
