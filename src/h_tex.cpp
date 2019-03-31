/*    h_tex.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_HILIT_TEX

#define hsTEX_Normal  0
#define hsTEX_Tag     1
#define hsTEX_Comment 2
#define hsTEX_Special 3

int Hilit_TEX(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine *Line, hlState &State, hsState *StateMap, int *ECol) {
    ChColor *Colors = BF->Mode->fColorize->Colors;
    HILIT_VARS(Colors[CLR_Normal], Line);
    int j;

    for (i = 0; i < Line->Count;) {
        IF_TAB() else {
            switch (State) {
            case hsTEX_Normal:
                Color = Colors[CLR_Normal];
                if (*p == '%') {
                    State = hsTEX_Comment;
                    Color = Colors[CLR_Comment];
                    goto hilit;
                } else if (*p == '\\') {
                    State = hsTEX_Tag;
                    Color = Colors[CLR_Tag];
                    ColorNext();
                    continue;
                } else if (*p == '{' || *p == '}' || *p == '$' || *p == '&' || *p == '|') {
                    State = hsTEX_Special;
                    Color = Colors[CLR_Special];
                    ColorNext();
                    State = hsTEX_Normal;
                    continue;
                }
                goto hilit;
            case hsTEX_Tag:
                Color = Colors[CLR_Tag];
                if (isalpha(*p)) {
                    j = 0;
                    while (((i + j) < Line->Count) &&
                           (isalnum(Line->Chars[i+j])/* ||
                           (Line->Chars[i + j] == '_')*/)
                          ) j++;
                    if (BF->GetHilitWord(j, &Line->Chars[i], Color, 0)) {
                    }
                    if (StateMap)
                        memset(StateMap + i, State, j);
                    if (B)
                        MoveMem(B, C - Pos, Width, Line->Chars + i, Color, j);
                    i += j;
                    len -= j;
                    p += j;
                    C += j;
                    Color = Colors[CLR_Normal];
                    State = hsTEX_Normal;
                    continue;
                }
                ColorNext();
                Color = Colors[CLR_Normal];
                State = hsTEX_Normal;
                continue;
            case hsTEX_Comment:
                Color = Colors[CLR_Comment];
                goto hilit;
            default:
                State = hsTEX_Normal;
                Color = Colors[CLR_Normal];
            hilit:
                ColorNext();
                continue;
            }
        }
    }
    if (State == hsTEX_Comment)
        State = hsTEX_Normal;
    *ECol = C;
    return 0;
}
#endif
