/*    e_regex.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "e_regex.h"

//#define DEBUG

static int RegCount = 0;

#ifdef DEBUG
static void RxDump(int N, RxNode *n);
#endif

static  RxNode *NewNode(int aWhat) {
    RxNode *N = (RxNode *) malloc(sizeof(RxNode));

    if (N) {
        memset(N, 0, sizeof(RxNode));
        N->fWhat = (short)aWhat;
    }
    return N;
}

static RxNode *NewChar(char Ch) {
    RxNode *A = NewNode(RE_CHAR);

    if (A) {
        A->fChar = (char *)malloc(1);
        A->fLen = 1;
        A->fChar[0] = Ch;
    }
    return A;
}

static RxNode *NewEscape(const char **const Regexp) {
    char Ch = **Regexp;
    ++*Regexp;
    switch (Ch) {
      case 0: return 0;
      case 'a': Ch = '\a'; break;
      case 'b': Ch = '\b'; break;
      case 'f': Ch = '\f'; break;
      case 'n': Ch = '\n'; break;
      case 'r': Ch = '\r'; break;
      case 't': Ch = '\t'; break;
      case 'v': Ch = '\v'; break;
      case 'e': Ch = 27; break;
      case 's': return NewNode(RE_WSPACE);
      case 'S': return NewNode(RE_NWSPACE);
      case 'U': return NewNode(RE_UPPER);
      case 'L': return NewNode(RE_LOWER);
      case 'w': return NewNode(RE_WORD);
      case 'W': return NewNode(RE_NWORD);
      case 'd': return NewNode(RE_DIGIT);
      case 'D': return NewNode(RE_NDIGIT);
      case 'C': return NewNode(RE_CASE);
      case 'c': return NewNode(RE_NCASE);
      case 'N':
        {
            unsigned int N = 0;
            unsigned int A = 0;
            if (**Regexp == 0) return 0;
            N = toupper(**Regexp) - 48; if (N > 9) return 0;
            (*Regexp)++;
            A = N * 100;
            if (**Regexp == 0) return 0;
            N = toupper(**Regexp) - 48; if (N > 9) return 0;
            (*Regexp)++;
            A = A + N * 10;
            if (**Regexp == 0) return 0;
            N = toupper(**Regexp) - 48; if (N > 9) return 0;
            (*Regexp)++;
            A = A + N;
            Ch = (char)A;
        }
        break;
    case 'o':
        {
            unsigned int N = 0;
            unsigned int A = 0;
            if (**Regexp == 0) return 0;
            N = toupper(**Regexp) - 48; if (N > 7) return 0;
            (*Regexp)++;
            A = N * 64;
            if (**Regexp == 0) return 0;
            N = toupper(**Regexp) - 48; if (N > 7) return 0;
            (*Regexp)++;
            A = A + N * 8;
            if (**Regexp == 0) return 0;
            N = toupper(**Regexp) - 48; if (N > 7) return 0;
            (*Regexp)++;
            A = A + N;
            Ch = (char)A;
        }
        break;
    case 'x':
        {
            unsigned int N = 0;
            unsigned int A = 0;
            if (**Regexp == 0) return 0;
            N = toupper(**Regexp) - 48; if (N > 9) N = N + 48 - 65 + 10; if (N > 15) return 0;
            (*Regexp)++;
            A = N << 4;
            if (**Regexp == 0) return 0;
            N = toupper(**Regexp) - 48; if (N > 9) N = N + 48 - 65 + 10; if (N > 15) return 0;
            (*Regexp)++;
            A = A + N;
            Ch = (char)A;
        }
        break;
    }
    return NewChar(Ch);
}


#define NNN 32        // 8 * 32 = 256 (match set)

#define SETOP(set,n) \
    do { \
      set[(unsigned char)(n) >> 3] |= (unsigned char)(1 << ((unsigned char)(n) & 7)); \
    } while (0)

static RxNode *NewSet(const char ** const Regexp) {
    unsigned char set[NNN];
    int s = 0;
    int c = 0;
    unsigned int i, xx;
    unsigned char Ch, C1 = 0, C2 = 0;
    int doset = 0;

    memset(set, 0, sizeof(set));
    s = 1;
    if (**Regexp == '^') {
        s = 0;
        ++*Regexp;
    }
    c = 0;

    while (**Regexp) {
        switch (Ch = *((*Regexp)++)) {
          case ']':
            if (doset == 1) return 0;
            {
                RxNode *N = NewNode(s?RE_INSET:RE_NOTINSET);
                N->fChar = (char *) malloc(sizeof(set));
                N->fLen = sizeof(set);
                if (N->fChar == 0) return 0;
                memcpy(N->fChar, (char *) set, sizeof(set));
                return N;
            }
          case '\\':
            switch (Ch = *((*Regexp)++)) {
              case 0: return 0;
              case 'a': Ch = '\a'; break;
              case 'b': Ch = '\b'; break;
              case 'f': Ch = '\f'; break;
              case 'n': Ch = '\n'; break;
              case 'r': Ch = '\r'; break;
              case 't': Ch = '\t'; break;
              case 'v': Ch = '\v'; break;
              case 'e': Ch = 27; break;
              case 'N':
                  {
                      unsigned int N = 0;
                      unsigned int A = 0;
                      if (**Regexp == 0) return 0;
                      N = toupper(**Regexp) - 48; if (N > 9) return 0;
                      (*Regexp)++;
                      A = N * 100;
                      if (**Regexp == 0) return 0;
                      N = toupper(**Regexp) - 48; if (N > 9) return 0;
                      (*Regexp)++;
                      A = A + N * 10;
                      if (**Regexp == 0) return 0;
                      N = toupper(**Regexp) - 48; if (N > 9) return 0;
                      (*Regexp)++;
                      A = A + N;
                      Ch = (unsigned char)A;
                  }
                  break;
            case 'o':
                {
                    unsigned int N = 0;
                    unsigned int A = 0;
                    if (**Regexp == 0) return 0;
                    N = toupper(**Regexp) - 48; if (N > 7) return 0;
                    (*Regexp)++;
                    A = N * 64;
                    if (**Regexp == 0) return 0;
                    N = toupper(**Regexp) - 48; if (N > 7) return 0;
                    (*Regexp)++;
                    A = A + N * 8;
                    if (**Regexp == 0) return 0;
                    N = toupper(**Regexp) - 48; if (N > 7) return 0;
                    (*Regexp)++;
                    A = A + N;
                    Ch = (unsigned char)A;
                }
                break;
            case 'x':
                {
                    unsigned int N = 0;
                    unsigned int A = 0;
                    if (**Regexp == 0) return 0;
                    N = toupper(**Regexp) - 48; if (N > 9) N = N + 48 - 65 + 10; if (N > 15) return 0;
                    (*Regexp)++;
                    A = N << 4;
                    if (**Regexp == 0) return 0;
                    N = toupper(**Regexp) - 48; if (N > 9) N = N + 48 - 65 + 10; if (N > 15) return 0;
                    (*Regexp)++;
                    A = A + N;
                    Ch = (unsigned char)A;
                }
                break;
            case 's':
                c += 4;
                SETOP(set, '\n');
                SETOP(set, '\t');
                SETOP(set, ' ');
                SETOP(set, '\r');
                continue;
            case 'S':
                for (xx = 0; xx <= 255; xx++) {
                    if (xx != ' ' && xx != '\t' && xx != '\n' && xx != '\r') {
                        c++;
                        SETOP(set, xx);
                    }
                }
                continue;
            case 'w':
                for (xx = 0; xx <= 255; xx++) {
                    if (isalnum(xx)) {
                        c++;
                        SETOP(set, xx);
                    }
                }
                break;
            case 'W':
                for (xx = 0; xx <= 255; xx++) {
                    if (!isalnum(xx)) {
                        c++;
                        SETOP(set, xx);
                    }
                }
                break;
            case 'd':
                for (xx = 0; xx <= 255; xx++) {
                    if (isdigit(xx)) {
                        c++;
                        SETOP(set, xx);
                    }
                }
                break;
            case 'D':
                for (xx = 0; xx <= 255; xx++) {
                    if (!isdigit(xx)) {
                        c++;
                        SETOP(set, xx);
                    }
                }
                break;
            case 'U':
                for (xx = 'A'; xx <= 'Z'; xx++) {
                    c++;
                    SETOP(set, xx);
                }
                continue;
            case 'L':
                for (xx = 'a'; xx <= 'z'; xx++) {
                    c++;
                    SETOP(set, xx);
                }
                continue;
            }
            break;
        }
        if (doset == 0 && ((**Regexp) == '-')) {
            doset = 1;
            C1 = Ch;
            ++*Regexp;
            continue;
        } else if (doset == 1) {
            C2 = Ch;
            if (C2 < C1) return 0;
            for(i = C1; i <= C2; i++) SETOP(set, i);
            doset = 0;
            continue;
        }
        c++;
        SETOP(set, Ch);
    }
    return 0;
}

static int AddNode(RxNode **F, RxNode **N, RxNode *A) {
    if (A) {
        if (*F) {
            (*N)->fNext = A;
            A->fPrev = (*N);
            *N = A;
        } else {
            (*N) = (*F) = A;
            A->fPrev = A->fNext = 0;
        }
        return 1;
    }
    return 0;
}

static int CountWidth(RxNode *N) {
    int w = 0;

    while (N) {
        if (N->fWhat < 32) w += 0;
        else if (N->fWhat >= 32 && N->fWhat < 64)
            w += 1;
        N = N->fNext;
    }
    return w;
}

static int MakeSub(RxNode **F, RxNode **N, char What) {
    //printf("MakeSub: %c\n", What);
    if (*N) {
        RxNode *No;
        RxNode *New;
        RxNode *Jump, *Skip;
        RxNode *Last = (*N);

        if (Last->fWhat & RE_GROUP) {
            RxNode *P = Last->fPrev;
            int C = 1;

            while ((C > 0) && P) {
                //puts("backtracking...-----");
                //RxDump(0, P);
                if (P->fWhat & RE_GROUP) {
                    if (P->fWhat & RE_CLOSE) C++;
                    else C--;
                }
                Last = P;
                if (C == 0) break;
                P = P->fPrev;
            }
            //printf("P = %s, c = %d", P ? "ok":"null", C);
            if (C != 0) return 0;
        }
        assert(Last);
        if (What != '?' && What != '|')
            if (CountWidth(Last) == 0) {
                //                puts("FAILED count");
                return 0;
            }
        switch (What) {
        case '?':    /* BRANCH x NOTHING */
            New = NewNode(RE_BRANCH | RE_GREEDY | What);
            No = NewNode(RE_NOTHING);
            if (!New || !No) return 0;
            No->fPrev = *N;
            if (*N)
                (*N)->fNext = No;
            New->fNext = Last;
            New->fPrev = Last->fPrev;
            Last->fPrev = New;
            if (New->fPrev) {
                New->fPrev->fNext = New;
            } else {
                *F = New;
            }
            New->fPtr = No;
            No->fPtr = New;
            *N = No;
            //puts("BRANCH ?");
            break;

        case '*':
        case '@':
            New = NewNode(RE_BRANCH | What | ((What == '*') ? RE_GREEDY : 0));
            Jump = NewNode(RE_JUMP);
            No = NewNode(RE_NOTHING);

            if (!New || !No || !Jump) return 0;
            No->fPrev = Jump;
            Jump->fNext = No;
            Jump->fPrev = *N;
            if (*N)
                (*N)->fNext = Jump;
            New->fNext = Last;
            New->fPrev = Last->fPrev;
            Last->fPrev = New;
            if (New->fPrev) {
                New->fPrev->fNext = New;
            } else {
                *F = New;
            }
            New->fPtr = No;
            No->fPtr = New;
            Jump->fPtr = New;
            *N = No;
            //puts("BRANCH *");
            break;

        case '#':
        case '+':
            New = NewNode(RE_BRANCH | What | ((What == '+') ? RE_GREEDY : 0));
            Skip = NewNode(RE_JUMP);
            Jump = NewNode(RE_JUMP);
            No = NewNode(RE_NOTHING);

            if (!New || !No || !Jump) return 0;
            No->fPrev = Jump;
            Jump->fPrev = *N;
            Jump->fNext = No;

            Skip->fNext = New;
            New->fPrev = Skip;
            if (*N)
                (*N)->fNext = Jump;
            New->fNext = Last;
            Skip->fPrev = Last->fPrev;
            Last->fPrev = New;
            if (Skip->fPrev) {
                Skip->fPrev->fNext = Skip;
            } else {
                *F = Skip;
            }
            New->fPtr = No;
            No->fPtr = New;
            Jump->fPtr = New;
            Skip->fPtr = Last;
            *N = No;
            //puts("BRANCH +");
            break;
        case '|':
            New = NewNode(RE_BRANCH | RE_GREEDY | What);
            Jump = NewNode(RE_BREAK);
            No = NewNode(RE_NOTHING);

            if (!New || !No || !Jump) return 0;
            No->fPrev = Jump;
            Jump->fNext = No;
            Jump->fPrev = *N;
            if (*N)
                (*N)->fNext = Jump;
            New->fNext = Last;
            New->fPrev = Last->fPrev;
            Last->fPrev = New;
            if (New->fPrev) {
                New->fPrev->fNext = New;
            } else {
                *F = New;
            }
            New->fPtr = No;
            No->fPtr = New;
            Jump->fPtr = New;
            *N = No;
            //puts("BRANCH |");
            break;
        }
        return 1;
    }
    return 0;
}

#define CHECK(n) do { if ((n) == 0) { return 0;} } while (0)

static RxNode *RxComp(const char **const Regexp) {
    RxNode *F = 0;
    RxNode *N = 0;
    int C;
    char Ch;

    while (**Regexp) {
        //        puts(*Regexp);
        switch (Ch = (*(*Regexp)++)) {
        case '?':
        case '*':
        case '+':
        case '@':
        case '#':
        case '|':
            CHECK(MakeSub(&F, &N, Ch));
            break;
        case '}':
        case ')':
            return F;
        case '{':
            CHECK(AddNode(&F, &N, NewNode(RE_GROUP | RE_OPEN)));
            CHECK(AddNode(&F, &N, RxComp(Regexp)));
            while (N->fNext) N = N->fNext;
            CHECK(AddNode(&F, &N, NewNode(RE_GROUP | RE_CLOSE)));
            break;
        case '(':
            C = ++RegCount;
            CHECK(AddNode(&F, &N, NewNode(RE_GROUP | RE_OPEN | RE_MEM | C)));
            CHECK(AddNode(&F, &N, RxComp(Regexp)));
            while (N->fNext) N = N->fNext;
            CHECK(AddNode(&F, &N, NewNode(RE_GROUP | RE_CLOSE | RE_MEM | C)));
            break;
        case '\\':CHECK(AddNode(&F, &N, NewEscape(Regexp)));     break;
        case '[': CHECK(AddNode(&F, &N, NewSet(Regexp)));        break;
        case '^': CHECK(AddNode(&F, &N, NewNode(RE_ATBOL)));     break;
        case '$': CHECK(AddNode(&F, &N, NewNode(RE_ATEOL)));     break;
        case '.': CHECK(AddNode(&F, &N, NewNode(RE_ANY)));       break;
        case '<': CHECK(AddNode(&F, &N, NewNode(RE_ATBOW)));     break;
        case '>': CHECK(AddNode(&F, &N, NewNode(RE_ATEOW)));     break;
        default:
            --*Regexp;
            CHECK(AddNode(&F, &N, NewChar(**Regexp)));
            ++*Regexp;
            break;
        }
    }
    return F;
}

RxNode *RxOptimize(RxNode *rx) {
    return rx;
}

RxNode *RxCompile(const char *Regexp) {
    RxNode *n = 0, *x;
    if (Regexp == 0) return 0;
    RegCount = 0;
    n = RxComp(&Regexp);
    if (n == 0) return 0;
    n = RxOptimize(n);
    x = n;
    while (x->fNext) x = x->fNext;
    x->fNext = NewNode(RE_END);
    return n;
}

void RxFree(RxNode *n) {
    RxNode *p;

    while (n) {
        p = n;
        n = n->fNext;
        switch (p->fWhat) {
        case RE_INSET:
        case RE_NOTINSET:
        case RE_CHAR:
            free(p->fChar);
            break;
        default:
            break;
        }
        free(p);
    }
}

#define ChClass(x) (((((x) >= 'A') && ((x) <= 'Z')) || (((x) >= 'a') && ((x) <= 'z')) || (((x) >= '0') && ((x) <= '9')))?1:0)

static RxMatchRes *match;
static const char *bop;
static const char *eop;
static int flags = RX_CASE;
static const char *rex;

int RxMatch(RxNode *rx) {
    RxNode *n = rx;

    //printf(">>");
    while (n) {
        //printf("%-50.50s\n", rex);
        //RxDump(1, n);
        switch (n->fWhat) {
        case RE_NOTHING:
            break;
        case RE_CASE:
            flags |= RX_CASE;
            break;
        case RE_NCASE:
            flags &= ~RX_CASE;
            break;
        case RE_ATBOL:
            if (rex != bop) return 0;
            break;
        case RE_ATEOL:
            if (rex != eop) return 0;
            break;
        case RE_ANY:
            if (rex == eop) return 0;
            rex++;
            break;
        case RE_WSPACE:
            if (rex == eop) return 0;
            if (*rex != ' ' && *rex != '\n' && *rex != '\r' && *rex != '\t') return 0;
            rex++;
            break;
        case RE_NWSPACE:
            if (rex == eop) return 0;
            if (*rex == ' ' || *rex == '\n' || *rex == '\r' || *rex == '\t') return 0;
            rex++;
            break;
        case RE_WORD:
            if (rex == eop) return 0;
            if (!isalnum(*rex)) return 0;
            rex++;
            break;
        case RE_NWORD:
            if (rex == eop) return 0;
            if (isalnum(*rex)) return 0;
            rex++;
            break;
        case RE_DIGIT:
            if (rex == eop) return 0;
            if (!isdigit(*rex)) return 0;
            rex++;
            break;
        case RE_NDIGIT:
            if (rex == eop) return 0;
            if (isdigit(*rex)) return 0;
            rex++;
            break;
        case RE_UPPER:
            if (rex == eop) return 0;
            if (!isupper(*rex)) return 0;
            rex++;
            break;
        case RE_LOWER:
            if (rex == eop) return 0;
            if (!islower(*rex)) return 0;
            rex++;
            break;
        case RE_ATBOW:
            if (rex >= eop) return 0;
            if (rex > bop) {
                if ((ChClass(*rex) != 1) || (ChClass(*(rex-1)) != 0)) return 0;
            }
            break;
        case RE_ATEOW:
            if (rex <= bop) return 0;
            if (rex < eop) {
                if ((ChClass(*rex) != 0) || (ChClass(*(rex-1)) != 1)) return 0;
            }
            break;
        case RE_CHAR:
            if (rex == eop) return 0;
            if (flags & RX_CASE) {
                if (*n->fChar != *rex) return 0;
                if (memcmp(rex, n->fChar, n->fLen) != 0) return 0;
            } else {
                for (int i = 0; i < n->fLen; i++)
                    if (toupper(rex[i]) != toupper(n->fChar[i]))
                        return 0;
            }
            rex += n->fLen;
            break;
        case RE_INSET:
            if (rex == eop) return 0;
            if ((n->fChar[(unsigned char)(*rex) >> 3] & (1 << ((unsigned char)(*rex) & 7))) == 0) return 0;
            rex++;
            break;
        case RE_NOTINSET:
            if (rex == eop) return 0;
            if (n->fChar[(unsigned char)(*rex) >> 3] & (1 << ((unsigned char)(*rex) & 7))) return 0;
            rex++;
            break;
        case RE_JUMP:
            n = n->fPtr;
            continue;
        case RE_END:
            return 1;
        case RE_BREAK:
            n = n->fNext;
            if (n->fNext == 0) break;
            n = n->fNext;
            if (n->fWhat & RE_BRANCH) {
                while ((n->fWhat & RE_BRANCH) && n->fPtr && ((n->fWhat & 0xFF) == '|'))
                    n = n->fPtr->fNext;
            }
            if (n->fWhat & RE_GROUP) {
                int C = 1;
                n = n->fNext;
                while ((C > 0) && n) {
                    if (n->fWhat & RE_GROUP) {
                        if (n->fWhat & RE_OPEN) C++;
                        else C--;
                    }
                    if (C == 0) break;
                    n = n->fNext;
                }
            }
            break;
        default:
            if (n->fWhat & RE_GROUP) {
                if (n->fWhat & RE_MEM) {
                    const char *save = rex;
                    int b = n->fWhat & 0xFF;
                    int fl = flags;

                    if (RxMatch(n->fNext) == 0) {
                        flags = fl;
                        if (n->fWhat & RE_OPEN)
                            match->Open[b] = -1;
                        else
                            match->Close[b] = -1;
                        return 0;
                    }

                    if (n->fWhat & RE_OPEN) {
                        //                        if (match->Open[b] == -1)
                        match->Open[b] = (int) (save - bop);
                    } else {
                        //                        if (match->Close[b] == -1)
                        match->Close[b] = (int) (save - bop);
                    }
                    return 1;
                }
            } else if (n->fWhat & RE_BRANCH) {
                const char *save = rex;
                int fl = flags;

                if ((n->fWhat & RE_GREEDY) == 0) {
                    if (RxMatch(n->fPtr) == 1) return 1;
                    flags = fl;
                    rex = save;
                } else {
                    if (RxMatch(n->fNext) == 1) return 1;
                    flags = fl;
                    rex = save;
                    n = n->fPtr;
                    continue;
                }
            }
            break;
        }
        n = n->fNext;
    }
    /* NOTREACHED */
    assert(1 == 0 /* internal regexp error */);
    return 0;
}

int RxTry(RxNode *rx, const char *s) {
    int fl = flags;
    rex = s;
    for (int i = 0; i < NSEXPS; i++)
        match->Open[i] = match->Close[i] = -1;
    if (RxMatch(rx)) {
        match->Open[0] = (int) (s - bop);
        match->Close[0] = (int) (rex - bop);
        return 1;
    }
    flags = fl;
    return 0;
}

int RxExec(RxNode *Regexp, const char *Data, int Len, const char *Start, RxMatchRes *Match, unsigned int RxOpt) {
    char Ch;
    if (Regexp == 0) return 0;

    match = Match;
    bop = Data;
    eop = Data + Len;

    flags = RxOpt;

    for (int i = 0; i < NSEXPS; i++) Match->Open[i] = Match->Close[i] = -1;

    switch (Regexp->fWhat) { // this should be more clever
    case RE_ATBOL:     // match is anchored
        return RxTry(Regexp, Start);
    case RE_CHAR:    // search for a character to match
        Ch = Regexp->fChar[0];
        if (Start == eop)
            break;
        if (flags & RX_CASE) {
            while (1) {
                while (Start < eop && *Start != Ch)
                    Start++;
                if (Start == eop)
                    break;
                if (RxTry(Regexp, Start))
                    return 1;
                if (++Start == eop)
                    break;
            }
        } else {
            Ch = toupper(Ch);
            while (1) {
                while (Start < eop && toupper(*Start) != Ch)
                    Start++;
                if (Start == eop)
                    break;
                if (RxTry(Regexp, Start))
                    return 1;
                if (++Start == eop)
                    break;
            }
        }
        break;
    default:         // (slow)
        do {
            if (RxTry(Regexp, Start)) return 1;
        } while (Start++ < eop);
        break;
    }
    return 0;
}

#define FLAG_UP_CASE     1
#define FLAG_DOWN_CASE   2
#define FLAG_UP_NEXT     4
#define FLAG_DOWN_NEXT   8

static int add(int *len, char **s, const char *a, int alen, int &flag) {
    int NewLen = *len + alen;
    int i;

    NewLen = NewLen * 2;

    if (alen == 0)
        return 0;

    if (*s) {
        *s = (char *) realloc(*s, NewLen);
        assert(*s);
        memcpy(*s + *len, a, alen);
    } else {
        *s = (char *) malloc(NewLen);
        assert(*s);
        memcpy(*s, a, alen);
        *len = 0;
    }
    if (flag & FLAG_UP_CASE) {
        char *p = *s + *len;

        for (i = 0; i < alen; i++) {
            *p = (char)toupper(*p);
            p++;
        }
    } else if (flag & FLAG_DOWN_CASE) {
        char *p = *s + *len;

        for (i = 0; i < alen; i++) {
            *p = (char)tolower(*p);
            p++;
        }
    }
    if (flag & FLAG_UP_NEXT) {
        char *p = *s + *len;

        *p = (char)toupper(*p);
        flag &= ~FLAG_UP_NEXT;
    } else if (flag & FLAG_DOWN_NEXT) {
        char *p = *s + *len;

        *p = (char)tolower(*p);
        flag &= ~FLAG_DOWN_NEXT;
    }
    *len += alen;
    return 0;
}

int RxReplace(const char *rep, const char *Src, int /*len*/, RxMatchRes match, char **Dest, int *Dlen) {
    int dlen = 0;
    char *dest = 0;
    char Ch;
    int n;
    int flag = 0;

    *Dest = 0;
    *Dlen = 0;
    //    add(&dlen, &dest, Src, match.Open[0]);
    while (*rep) {
        switch (Ch = *rep++) {
            //        case '&':
            //            add(&dlen, &dest, Src + match.Open[0], match.Close[0] - match.Open[0], flag);
            //            break;
        case '\\':
            switch (Ch = *rep++) {
            case '0':
            case '1': case '2': case '3':
            case '4': case '5': case '6':
            case '7': case '8': case '9':
                n = Ch - 48;

                if (match.Open[n] != -1 && match.Close[n] != -1) {
                    add(&dlen, &dest, Src + match.Open[n], match.Close[n] - match.Open[n], flag);
                } else return -1;
                break;
            case 0:
                if (dest) free(dest);
                return -1; // error
            case 'r': Ch = '\r'; add(&dlen, &dest, &Ch, 1, flag); break;
            case 'n': Ch = '\n'; add(&dlen, &dest, &Ch, 1, flag); break;
            case 'b': Ch = '\b'; add(&dlen, &dest, &Ch, 1, flag); break;
            case 'a': Ch = '\a'; add(&dlen, &dest, &Ch, 1, flag); break;
            case 't': Ch = '\t'; add(&dlen, &dest, &Ch, 1, flag); break;
            case 'U': flag |= FLAG_UP_CASE; break;
            case 'u': flag |= FLAG_UP_NEXT; break;
            case 'L': flag |= FLAG_DOWN_CASE; break;
            case 'l': flag |= FLAG_DOWN_NEXT; break;
            case 'E':
            case 'e': flag &= ~(FLAG_UP_CASE | FLAG_DOWN_CASE); break;
            case 'x':
                {
                    int N = 0;
                    int A = 0;

                    if (*rep == 0) { free(dest); return 0; }
                    N = toupper(*rep) - 48; if (N > 9) N = N + 48 - 65 + 10; if (N > 15) return 0;
                    rep++;
                    A = N << 4;
                    if (*rep == 0) { free(dest); return 0; }
                    N = toupper(*rep) - 48; if (N > 9) N = N + 48 - 65 + 10; if (N > 15) return 0;
                    rep++;
                    A = A + N;
                    Ch = (char)A;
                }
                add(&dlen, &dest, &Ch, 1, flag);
                break;
            case 'd':
                {
                    int N = 0;
                    int A = 0;

                    if (*rep == 0) { free(dest); return 0; }
                    N = toupper(*rep) - 48; if (N > 9) { free(dest); return 0; }
                    rep++;
                    A = N * 100;
                    if (*rep == 0) { free(dest); return 0; }
                    N = toupper(*rep) - 48; if (N > 9) { free(dest); return 0; }
                    rep++;
                    A = N * 10;
                    if (*rep == 0) { free(dest); return 0; }
                    N = toupper(*rep) - 48; if (N > 9) { free(dest); return 0; }
                    rep++;
                    A = A + N;
                    Ch = (char)A;
                }
                add(&dlen, &dest, &Ch, 1, flag);
                break;
            case 'o':
                {
                    int N = 0;
                    int A = 0;

                    if (*rep == 0) { free(dest); return 0; }
                    N = toupper(*rep) - 48; if (N > 7) { free(dest); return 0; }
                    rep++;
                    A = N * 64;
                    if (*rep == 0) { free(dest); return 0; }
                    N = toupper(*rep) - 48; if (N > 7) { free(dest); return 0; }
                    rep++;
                    A = N * 8;
                    if (*rep == 0) { free(dest); return 0; }
                    N = toupper(*rep) - 48; if (N > 7) { free(dest); return 0; }
                    rep++;
                    A = A + N;
                    Ch = (char)A;
                }
                add(&dlen, &dest, &Ch, 1, flag);
                break;
            default:
                add(&dlen, &dest, &Ch, 1, flag);
                break;
            }
            break;
        default:
            add(&dlen, &dest, &Ch, 1, flag);
            break;
        }
    }
    //    add(&dlen, &dest, Src + match.Close[0], len - match.Close[0]);
    *Dlen = dlen;
    *Dest = dest;
    return 0;
}

#ifdef DEBUG

static void RxDump(int N, RxNode *n) {
    while (n) {
        for (int i = 0; i < N; i++) printf("    ");
        switch (n->fWhat) {
        case RE_NOTHING:   printf("NOTHING\n"); break;
        case RE_CHAR:      printf("CHAR '%.1s'\n", n->fChar); break;
        case RE_ATBOL:     printf("^\n"); break;
        case RE_ATEOL:     printf("$\n"); break;
        case RE_ANY:       printf(".\n"); break;
        case RE_INSET:     printf("[\n"/*, n->fChar*/); break;
        case RE_NOTINSET:  printf("[^\n"/*, n->fChar*/); break;
        case RE_ATBOW:     printf("<\n"); break;
        case RE_ATEOW:     printf(">\n"); break;
        case RE_WSPACE:    printf("WSPACE\n"); break;
        case RE_NWSPACE:   printf("NWSPACE\n"); break;
        case RE_UPPER:     printf("UPPER\n"); break;
        case RE_LOWER:     printf("LOWER\n"); break;
        case RE_JUMP:      printf("JUMP\n"); break;
        case RE_BREAK:     printf("BREAK\n"); break;
        case RE_END:       printf("END\n"); break;
        default:
            if (n->fWhat & RE_GROUP) {
                if (n->fWhat & RE_MEM) {
                    if (n->fWhat & RE_OPEN)  printf("(  %d\n", n->fWhat & 0xFF);
                    if (n->fWhat & RE_CLOSE) printf(")  %d\n", n->fWhat & 0xFF);
                } else {
                    if (n->fWhat & RE_OPEN)  printf("{\n");
                    if (n->fWhat & RE_CLOSE) printf("}\n");
                }
            } else if (n->fWhat & RE_BRANCH) {
                if (n->fWhat & RE_GREEDY) {
                    printf("%c\n", n->fWhat & 0xFF);
                } else {
                    printf("%c\n", n->fWhat & 0xFF);
                }
            } else {
                printf("???????????????\n");
            }
            break;
        }
        n = n->fNext;
    }
}

#define TEST(rc,rx,st) \
    strcpy(line,st); \
    assert((a = RxCompile(rx)) != 0); \
    puts("\n--- " rx " -- " st " -- "); \
    RxDump(0,a);\
    assert(rc == RxExec(a, line, strlen(line), line, &b)); \
    RxFree(a);

int main() {
    RxNode *a;
    RxMatchRes b;
    char line[1024];

    TEST(1, "a", "a");
    TEST(0, "b", "a");
    TEST(1, "aaaa", "aaaa");
    TEST(0, "bbbb", "aaaa");
    TEST(1, ".", "a");
    TEST(0, ".", "");
    TEST(1, "a..", "axx");
    TEST(0, "a..", "b..");
    TEST(1, "a?b", "ab");
    TEST(1, "a?b", "xb");
    TEST(0, "a?C", "xb");
    TEST(1, "{aa}?b", "aab");
    TEST(1, "{aa}?b", "xab");
    TEST(0, "{aa}?C", "xxb");
    TEST(1, "^aa", "aa");
    TEST(0, "^aa", "baa");
    TEST(1, "^aa$" ,"aa");
    TEST(0, "^aa$", "baab");
    TEST(1, "a*b", "aaab");
    TEST(0, "a*b", "aaaa");
    TEST(1, "{aa}*b", "aaab");
    TEST(0, "{aa}*b", "aaaa");
    TEST(1, "b+", "bb");
    TEST(1, "b+", "b");
    TEST(0, "b+", "a");
    TEST(1, "^b+$", "b");
    TEST(0, "^b+$", "aba");
    TEST(1, "a|b", " a");
    TEST(1, "a|b", " b");
    TEST(0, "a|b", " c");
    TEST(1, "a|b|c|d|e", " a ");
    TEST(1, "a|b|c|d|e", " c ");
    TEST(1, "a|b|c|d|e", " e ");
    TEST(0, "a|b|c|d|e", " x ");
    TEST(1, "{a}|{b}|{c}|{d}|{e}", " a ");
    TEST(1, "{a}|{b}|{c}|{d}|{e}", " c ");
    TEST(1, "{a}|{b}|{c}|{d}|{e}", " e ");
    TEST(0, "{a}|{b}|{c}|{d}|{e}", " x ");
    TEST(1, "^xx{alpha}|{beta}xx$", "xxalphaxx");
    TEST(1, "^xx{alpha}|{beta}xx$", "xxbetaxx");
    TEST(1, "[a-z]", "aaa");
    TEST(1, "^{Error}|{Warning}", "Warning search.cpp 35: Conversion may lose significant digits in function AskReplace()");
    TEST(1, "^{Error}|{Warning} (.+)", "Warning search.cpp 35: Conversion may lose significant digits in function AskReplace()");
    TEST(1, "^{Error}|{Warning} ([a-z.]#) ([0-9]#)", "Warning search.cpp 35: Conversion may lose significant digits in function AskReplace()");
    TEST(1, "^{Error}|{Warning} (.+) ([0-9]+): (.*)$", "Warning search.cpp 35: Conversion may lose significant digits in function AskReplace()");
    TEST(1, "^{Error}|{Warning} (.+) ([0-9]+): (.*)$", "Error search.cpp 35: Conversion may lose significant digits in function AskReplace()");
    TEST(1, "^([a-z]+ +)*\\(", "blabla bla bla bla (");
    TEST(1, "^([a-z]+\\s+)+\\(", "blabla bla bla bla (");
    TEST(1, "^([a-z]+\\s*)+\\(", "blabla bla bla bla(");
    TEST(1, "^([a-z]+\\s+)+\\(", "blabla bla   bla bla (");
    TEST(1, "^([a-z]+\\s*)+\\(", "blabla   bla bla bla(");
    TEST(1, "^([a-z]# #)*\\(", "blabla bla bla bla (");
    TEST(1, "^([a-z]+ @)@\\(", "blabla bla bla bla (");
    TEST(1, "^[\\x20-\\xFF]+$", "blabla");
    TEST(1, "{a{a{a{a|a}|{a|a}a}a}a|a}", "aaaaaaaaaaaaaaaaa");

    while (1) {
        printf("Regexp: "); fflush(stdout); gets(line);
        if (!*line) break;
        a = RxCompile(line); RxDump(0, a);
        printf("String: "); fflush(stdout); gets(line);
        printf("rc = %d\n", RxExec(a, line, strlen(line), line, &b));
        for (int i = 0; i < NSEXPS; i++) {
            if (b.Open[i] != -1) {
                printf("%d: %d %d\n", i, b.Open[i], b.Close[i]);
            }
        }
        RxFree(a);
    }
    return 0;
}

#endif
