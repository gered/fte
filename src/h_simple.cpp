/*    h_simple.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_HILIT_SIMPLE

int Hilit_SIMPLE(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine *Line, hlState &State, hsState *StateMap, int *ECol) {
    EColorize *col = BF->Mode->fColorize;
    ChColor *Colors = col->Colors;
    HMachine *hm = col->hm;
    HILIT_VARS(Colors[CLR_Normal], Line);
    HState *st = 0;
    HTrans *tr = 0;
    int t, cc;
    int quotech = 0;
    int matchFlags;
    int matchLen;
    int nextState;
    char *match;

    if (hm == 0 || col == 0 || hm->stateCount == 0)
        return 0;

    if (State >= hm->stateCount)
        State = 0;
    st = hm->state + State;
    Color = Colors[st->color];

    /*{
        fprintf(stderr, "ColMode:%s, State:%d\n", col->Name, State);
        for (int s = 0; s < hm->stateCount; s++) {
            fprintf(stderr,
                    "State:%d, transCount:%d, firstTrans:%d, options:%d, color:%d, nextState:%d\n",
                    s,
                    hm->state[s].transCount,
                    hm->state[s].firstTrans,
                    hm->state[s].options,
                    hm->state[s].color,
                    hm->state[s].nextState);
        }
        for (int t = 0; t < hm->transCount; t++) {
            fprintf(stderr,
                    "Trans:%d, matchLen:%d, matchFlags:%d, nextState:%d, color:%d\n",
                    t,
                    hm->trans[t].matchLen,
                    hm->trans[t].matchFlags,
                    hm->trans[t].nextState,
                    hm->trans[t].color);
        }
        //exit(1);
        sleep(5);
    }*/

    for (i = 0; i < Line->Count; ) {
        if (quotech) {
            quotech = 0;
        } else {
            for (t = 0; t < st->transCount; t++) {
                tr = hm->trans + st->firstTrans + t;
                matchLen = tr->matchLen;
                matchFlags = tr->matchFlags;
                match = tr->match;
                nextState = tr->nextState;

                //fprintf(stderr,
                //        "line:%d, char:%d, len:%d, state:%d, tr:%d, st->transCount:%d, st->firstTrans, nextState:%d, matchFlags:%d\n",
                //        LN, i, len, State, t, st->transCount, st->firstTrans, nextState, matchFlags);

               if (len < matchLen)
                    continue;

                if ((matchFlags & (MATCH_SET | MATCH_NOTSET)) == 0) {
                    if (matchFlags & MATCH_NO_CASE) {
                        if (memicmp(match, p, matchLen))
                            continue;
                    } else {
                        for (cc = 0; cc < matchLen; cc++)
                            if (p[cc] != match[cc])
                                goto next_trans;
                    }
                } else if (matchFlags & MATCH_SET) {
                    if (!WGETBIT(match, *p))
                        continue;
                } else if (matchFlags & MATCH_NOTSET) {
                    if (WGETBIT(match, *p))
                        continue;
                }

                if ((i > 0) && (matchFlags & MATCH_MUST_BOL))
                    continue;

                if ((len != matchLen) && (matchFlags & MATCH_MUST_EOL))
                    continue;

                if (matchFlags & MATCH_NOGRAB) {
                    State = nextState;
                    if (State >= hm->stateCount)
                        State = 0;
                    st = hm->state + State;
                    //fprintf(stderr, "nograb\n");
                } else {
                    if (matchFlags & MATCH_TAGASNEXT) {
                        State = nextState;
                        if (State >= hm->stateCount)
                            State = 0;
                        st = hm->state + State;
                    }
                    Color = Colors[tr->color];
                    for (cc = 0; cc < matchLen; cc++)
                        IF_TAB()
                        else
                            ColorNext();
                    if (!(matchFlags & MATCH_TAGASNEXT)) {
                        State = nextState;
                        if (State >= hm->stateCount)
                            State = 0;
                        st = hm->state + State;
                    }
                    if (len > 0) {
                        if (matchFlags & MATCH_QUOTECH)
                            quotech = 1;
                    } else if (len == 0) {
                        if (matchFlags & MATCH_QUOTEEOL)
                            goto end_parse; /* see note below !! */
                    }
                }
                //fprintf(stderr, "next state\n");
                goto next_state;
            next_trans: /* */;
            }
            if (st->wordChars != 0) {
                int j;
                hlState MState = State;

                j = 0;
                while (((i + j) < Line->Count) &&
                       (WGETBIT(st->wordChars, Line->Chars[i + j]))) j++;

                //GP (fix)
                Color = Colors[st->color];

                if (j == 0) {
                    if (st->nextKwdNoCharState != -1) {
                        State = st->nextKwdNoCharState;
                        if (State >= hm->stateCount)
                            State = 0;
                        st = hm->state + State;
                        Color = Colors[st->color];
                        goto next_state;
                    }
                } else {
                    if (st->GetHilitWord(j, &Line->Chars[i], Color ) ||
                        BF->GetHilitWord(j, &Line->Chars[i], Color,  BFI( BF, BFI_MatchCase ) ? 0 : 1))
                    {
                        if (st->nextKwdMatchedState != -1)
                            State = st->nextKwdMatchedState;
                    } else {
                        if (st->nextKwdNotMatchedState != -1) {
                            State = st->nextKwdNotMatchedState;
                            if (st->options & STATE_NOGRAB) {
                                if (State >= hm->stateCount)
                                    State = 0;
                                st = hm->state + State;
                                Color = Colors[st->color];
                                goto next_state;
                            }
                        }
                    }
                    
                    if (State >= hm->stateCount)
                        State = 0;
                    
                    // highlight/tag as next state
                    if (st->options & STATE_TAGASNEXT) {
                        MState = State;
                        st = hm->state + State;
                        Color = Colors[st->color];
                    }
                    
                    if (StateMap)
                        memset(StateMap + i, MState, j);
                    if (B)
                        MoveMem(B, C - Pos, Width, Line->Chars + i, Color, j);
                    i += j;
                    len -= j;
                    p += j;
                    C += j;

                    if (!(st->options & STATE_TAGASNEXT)) {
                        st = hm->state + State;
                        Color = Colors[st->color];
                    }
                    goto next_state;
                }
            }
        }
        Color = Colors[st->color];
        IF_TAB()
        else
            ColorNext();
    next_state: /* */;
    }

    /* check if there are any matches for EOL */
    /* NOTE: this is skipped when Q option is used above. !! */
    for (t = 0; t < st->transCount; t++) {
        tr = hm->trans + st->firstTrans + t;
        matchLen = tr->matchLen;
        matchFlags = tr->matchFlags;
        match = tr->match;
        nextState = tr->nextState;
        
        if ((i > 0) && (matchFlags & MATCH_MUST_BOL))
            continue;

        //cant match eol beyond eol.
        //if ((len != matchLen) && (matchFlags & MATCH_MUST_EOL))
        //continue;

        if (matchLen == 0) {
            State = nextState;
            if (State >= hm->stateCount)
                State = 0;
            break;
        }
    }
end_parse: ;
    *ECol = C;
    return 0;
}

#endif
