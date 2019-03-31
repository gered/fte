/*    c_config.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

typedef struct _CurPos {
    int sz;
    const char *a;
    const char *c;
    const char *z;
    int line;
    const char *name; // filename
} CurPos;

#ifdef CONFIG_INDENT_C
extern int C_Indent;
extern int C_BraceOfs;
extern int C_CaseOfs;
extern int C_CaseDelta;
extern int C_ClassOfs;
extern int C_ClassDelta;
extern int C_ColonOfs;
extern int C_CommentOfs;
extern int C_CommentDelta;
extern int C_FirstLevelWidth;
extern int C_FirstLevelIndent;
extern int C_Continuation;
extern int C_ParenDelta;
#endif

#ifdef CONFIG_INDENT_REXX
extern int REXX_Base_Indent;
extern int REXX_Do_Offset;
#endif

extern int ShowVScroll;
extern int ShowHScroll;
extern int ShowMenuBar;

int SystemClipboard = 0;
int ScreenSizeX = -1, ScreenSizeY = -1;
int ScrollBarWidth = 1;
int CursorInsSize[2] = { 90, 100 };
int CursorOverSize[2] = { 0, 100 };
int OpenAfterClose = 1;
int SelectPathname = 0;
char DefaultModeName[32] = "";
RxNode *CompletionFilter = NULL;
#if defined(DOS) || defined(DOSP32)
char PrintDevice[MAXPATH] = "PRN";
#else
char PrintDevice[MAXPATH] = "\\DEV\\PRN";
#endif
char CompileCommand[256] = "make";
int KeepHistory = 0;
int LoadDesktopOnEntry = 0;
int SaveDesktopOnExit = 0;
char WindowFont[64] = "";
int KeepMessages = 0;
int ScrollBorderX = 0;
int ScrollBorderY = 0;
int ScrollJumpX = 8;
int ScrollJumpY = 1;
int GUIDialogs = 1;
int PMDisableAccel = 0;
int SevenBit = 0;
int WeirdScroll = 0;
int LoadDesktopMode = 0;
char HelpCommand[128] = "man -a";
char *ConfigSourcePath = 0;

#ifdef CONFIG_SYNTAX_HILIT
int AddKeyword(ColorKeywords *tab, char color, const char *keyword) {
    int len;

    len = strlen(keyword);
    if (len < 1 || len >= CK_MAXLEN) return 0;

    if (tab->key[len]) {
        int lx = strlen(tab->key[len]);
        char *key;

        key = (char *)realloc(tab->key[len], lx + len + 1 + 1);
        assert(key != NULL);

        tab->key[len] = key;
        assert(tab->key[len] != 0);
        strcpy(tab->key[len] + lx, keyword);
        tab->key[len][lx + len] = color;
        tab->key[len][lx + len + 1] = 0;
    } else {
        tab->key[len] = (char *)malloc(len + 2);
        assert(tab->key[len] != 0);
        strcpy(tab->key[len], keyword);
        tab->key[len][len] = color;
        tab->key[len][len + 1] = 0;
    }
    tab->count[len]++;
    tab->TotalCount++;
    return 1;
}
#endif

int SetModeNumber(EMode *mode, int what, int number) {
    int j = what;

    if (j == BFI_LeftMargin || j == BFI_RightMargin) number--;
    mode->Flags.num[j] = number;
    return 0;
}

int SetModeString(EMode *mode, int what, const char *string) {
    int j = what;

#ifdef CONFIG_SYNTAX_HILIT
    if (j == BFI_Colorizer) {
        mode->fColorize = FindColorizer(string);
    } else
#endif
        if (j == BFI_EventMap) {
            mode->fEventMap = FindEventMap(string);
        } else if (j == BFI_IndentMode) {
            mode->Flags.num[j] = GetIndentMode(string);
        } else if (j == BFS_WordChars) {
            SetWordChars(mode->Flags.WordChars, string);
        } else if (j == BFS_CapitalChars) {
            SetWordChars(mode->Flags.CapitalChars, string);
        } else if (j == BFS_FileNameRx) {
            if (mode->MatchName)
                free(mode->MatchName);
            if (mode->MatchNameRx)
                RxFree(mode->MatchNameRx);
            mode->MatchName = strdup(string);
            mode->MatchNameRx = RxCompile(string);
        } else if (j == BFS_FirstLineRx) {
            if (mode->MatchLine)
                free(mode->MatchLine);
            if (mode->MatchLineRx)
                RxFree(mode->MatchLineRx);
            mode->MatchLine = strdup(string);
            mode->MatchLineRx = RxCompile(string);
        } else {
            if (mode->Flags.str[j & 0xFF])
                free(mode->Flags.str[j & 0xFF]);
            mode->Flags.str[j & 0xFF] = strdup(string);
        }
    return 0;
}

int SetGlobalNumber(int what, int number) {
    switch (what) {
#ifdef CONFIG_INDENT_C
    case FLAG_C_Indent:          C_Indent = number; break;
    case FLAG_C_BraceOfs:        C_BraceOfs = number; break;
    case FLAG_C_CaseOfs:         C_CaseOfs = number; break;
    case FLAG_C_CaseDelta:       C_CaseDelta = number; break;
    case FLAG_C_ClassOfs:        C_ClassOfs = number; break;
    case FLAG_C_ClassDelta:      C_ClassDelta = number; break;
    case FLAG_C_ColonOfs:        C_ColonOfs = number; break;
    case FLAG_C_CommentOfs:      C_CommentOfs = number; break;
    case FLAG_C_CommentDelta:    C_CommentDelta = number; break;
    case FLAG_C_FirstLevelIndent: C_FirstLevelIndent = number; break;
    case FLAG_C_FirstLevelWidth: C_FirstLevelWidth = number; break;
    case FLAG_C_Continuation:    C_Continuation = number; break;
    case FLAG_C_ParenDelta:      C_ParenDelta = number; break;
#endif
#ifdef CONFIG_INDENT_REXX
    case FLAG_REXX_Indent:       REXX_Base_Indent = number; break;
    case FLAG_REXX_Do_Offset:    REXX_Do_Offset = number; break;
#endif
    case FLAG_ScreenSizeX:       ScreenSizeX = number; break;
    case FLAG_ScreenSizeY:       ScreenSizeY = number; break;
    case FLAG_CursorInsertStart: CursorInsSize[0] = number; break;
    case FLAG_CursorInsertEnd:   CursorInsSize[1] = number; break;
    case FLAG_CursorOverStart:   CursorOverSize[0] = number; break;
    case FLAG_CursorOverEnd:     CursorOverSize[1] = number; break;
    case FLAG_SysClipboard:      SystemClipboard = number; break;
    case FLAG_OpenAfterClose:    OpenAfterClose = number; break;
    case FLAG_ShowVScroll:       ShowVScroll = number; break;
    case FLAG_ShowHScroll:       ShowHScroll = number; break;
    case FLAG_ScrollBarWidth:    ScrollBarWidth = number; break;
    case FLAG_SelectPathname:    SelectPathname = number; break;
    case FLAG_ShowMenuBar:       ShowMenuBar = number; break;
    case FLAG_ShowToolBar:       ShowToolBar = number; break;
    case FLAG_KeepHistory:       KeepHistory = number; break;
    case FLAG_LoadDesktopOnEntry: LoadDesktopOnEntry = number; break;
    case FLAG_SaveDesktopOnExit: SaveDesktopOnExit = number; break;
    case FLAG_KeepMessages:      KeepMessages = number; break;
    case FLAG_ScrollBorderX:     ScrollBorderX = number; break;
    case FLAG_ScrollBorderY:     ScrollBorderY = number; break;
    case FLAG_ScrollJumpX:       ScrollJumpX = number; break;
    case FLAG_ScrollJumpY:       ScrollJumpY = number; break;
    case FLAG_GUIDialogs:        GUIDialogs = number; break;
    case FLAG_PMDisableAccel:    PMDisableAccel = number; break;
    case FLAG_SevenBit:          SevenBit = number; break;
    case FLAG_WeirdScroll:       WeirdScroll = number; break;
    case FLAG_LoadDesktopMode:   LoadDesktopMode = number; break;
    default:
        return -1;
    }
    return 0;
}

int SetGlobalString(long what, const char *string) {
    switch (what) {
    case FLAG_DefaultModeName: strcpy(DefaultModeName, string); break;
    case FLAG_CompletionFilter: if ((CompletionFilter = RxCompile(string)) == NULL) return -1; break;
    case FLAG_PrintDevice: strcpy(PrintDevice, string); break;
    case FLAG_CompileCommand: strcpy(CompileCommand, string); break;
    case FLAG_WindowFont: strcpy(WindowFont, string); break;
    case FLAG_HelpCommand: strcpy(HelpCommand, string); break;
    default:
        return -1;
    }
    return 0;
}

int SetEventString(EEventMap *Map, long what, const char *string) {
    switch (what) {
    case EM_MainMenu:
    case EM_LocalMenu:
        Map->SetMenu(what, string);
        break;
    default:
        return -1;
    }
    return 0;
}

#ifdef CONFIG_SYNTAX_HILIT
int SetColorizeString(EColorize *Colorize, long what, const char *string) {
    switch (what) {
    case COL_SyntaxParser:
        Colorize->SyntaxParser = GetHilitMode(string);
        break;
    default:
        return -1;
    }
    return 0;
}
#endif

unsigned char GetObj(CurPos &cp, unsigned short &len) {
    len = 0;
    if (cp.c + 3 <= cp.z) {
        unsigned char c;
        unsigned char l[2];
        c = *cp.c++;
        memcpy(l, cp.c, 2);
        len = (l[1] << 8) + l[0];
        cp.c += 2;
        return c;
    }
    return 0xFF;
}

const char *GetCharStr(CurPos &cp, unsigned short len) {
    const char *p = cp.c;
    if (cp.c + len > cp.z)
        return 0;
    cp.c += len;
    return p;
}

int GetNum(CurPos &cp, long &num) {
    unsigned char n[4];
    if (cp.c + 4 > cp.z) return 0;
    memcpy(n, cp.c, 4);
    num =
        (n[3] << 24) +
        (n[2] << 16) +
        (n[1] << 8) +
        n[0];

    if ((n[3] > 127) && sizeof(long) > 4)
        num = num | (~0xFFFFFFFFUL);
    cp.c += 4;
    return 1;
}

int ReadCommands(CurPos &cp, const char *Name) {
    unsigned char obj;
    unsigned short len;
    long Cmd = NewCommand(Name);
    long cmdno;

    if (GetObj(cp, len) != CF_INT) return-1;
    if (GetNum(cp, cmdno) == 0) return -1;
    if (cmdno != (Cmd | CMD_EXT)) {
        fprintf(stderr, "Bad Command map %s -> %ld != %ld\n", Name, Cmd, cmdno);
        return -1;
    }

    while ((obj = GetObj(cp, len)) != 0xFF) {
        switch (obj) {
        case CF_COMMAND:
            {
                //              char *s;
                long cnt;
                long ign;
                long cmd;

                //                if ((s = GetCharStr(cp, len)) == 0) return -1;
                if (GetNum(cp, cmd) == 0) return -1;
                if (GetObj(cp, len) != CF_INT) return -1;
                if (GetNum(cp, cnt) == 0) return -1;
                if (GetObj(cp, len) != CF_INT) return -1;
                if (GetNum(cp, ign) == 0) return -1;

                //                if (cmd != CmdNum(s)) {
                //                    fprintf(stderr, "Bad Command Id: %s -> %d\n", s, cmd);
                //                    return -1;
                //                }

                if (AddCommand(Cmd, cmd, cnt, ign) == 0) {
                    if (Name == 0 || strcmp(Name, "xx") != 0) {
                        fprintf(stderr, "Bad Command Id: %ld\n", cmd);
                        return -1;
                    }
                }
            }
            break;
        case CF_STRING:
            {
                const char *s = GetCharStr(cp, len);

                if (s == 0) return -1;
                if (AddString(Cmd, s) == 0) return -1;
            }
            break;
        case CF_INT:
            {
                long num;

                if (GetNum(cp, num) == 0) return -1;
                if (AddNumber(Cmd, num) == 0) return -1;
            }
            break;
        case CF_VARIABLE:
            {
                long num;

                if (GetNum(cp, num) == 0) return -1;
                if (AddVariable(Cmd, num) == 0) return -1;
            }
            break;
        case CF_CONCAT:
            if (AddConcat(Cmd) == 0) return -1;
            break;
        case CF_END:
            return Cmd;
        default:
            return -1;
        }
    }
    return -1;
}

int ReadMenu(CurPos &cp, const char *MenuName) {
    unsigned char obj;
    unsigned short len;

    int menu = -1, item = -1;

    menu = NewMenu(MenuName);

    while ((obj = GetObj(cp, len)) != 0xFF) {
        switch (obj) {
        case CF_ITEM:
            {
                if (len == 0) {
                    item = NewItem(menu, 0);
                } else {
                    const char *s = GetCharStr(cp, len);
                    int Cmd;
                    if (s == 0) return -1;
                    item = NewItem(menu, s);
                    if ((obj = GetObj(cp, len)) != CF_MENUSUB) return -1;
                    if ((Cmd = ReadCommands(cp, 0)) == -1) return -1;
                    Menus[menu].Items[item].Cmd = Cmd + 65536;
                }
            }
            break;
        case CF_SUBMENU:
            {
                const char *s = GetCharStr(cp, len);
                const char *w;

                if ((obj = GetObj(cp, len)) != CF_STRING) return -1;
                if ((w = GetCharStr(cp, len)) == 0) return -1;
                item = NewSubMenu(menu, s, GetMenuId(w), SUBMENU_NORMAL);
            }
            break;

        case CF_SUBMENUCOND:
            {
                const char *s = GetCharStr(cp, len);
                const char *w;

                if ((obj = GetObj(cp, len)) != CF_STRING) return -1;
                if ((w = GetCharStr(cp, len)) == 0) return -1;
                item = NewSubMenu(menu, s, GetMenuId(w), SUBMENU_CONDITIONAL);
            }
            break;

        case CF_END:
            return 0;
        default:
            return -1;
        }
    }
    return -1;
}

int ReadColors(CurPos &cp, const char *ObjName) {
    unsigned char obj;
    unsigned short len;

    while ((obj = GetObj(cp, len)) != 0xFF) {
        switch (obj) {
        case CF_STRING:
            {
                char cl[30];
                const char *sname = GetCharStr(cp, len);
                const char *svalue;
                if (sname == 0) return -1;
                if ((obj = GetObj(cp, len)) != CF_STRING) return -1;
                if ((svalue = GetCharStr(cp, len)) == 0) return -1;
                strcpy(cl, ObjName);
                strcat(cl, ".");
                strcat(cl, sname);
                if (SetColor(cl, svalue) == 0) return -1;
            }
            break;
        case CF_END:
            return 0;
        default:
            return -1;
        }
    }
    return -1;
}

#ifdef CONFIG_SYNTAX_HILIT
int ReadHilitColors(CurPos &cp, EColorize *Colorize, const char * /*ObjName*/) {
    unsigned char obj;
    unsigned short len;

    while ((obj = GetObj(cp, len)) != 0xFF) {
        switch (obj) {
        case CF_INT:
            {
                long cidx;
                const char *svalue;

                if (GetNum(cp, cidx) == 0) return -1;
                if ((obj = GetObj(cp, len)) != CF_STRING)
                    return -1;
                if ((svalue = GetCharStr(cp, len)) == 0)
                    return -1;
                if (Colorize->SetColor(cidx, svalue) == 0)
                    return -1;
            }
            break;
        case CF_END:
            return 0;
        default:
            return -1;
        }
    }
    return -1;
}

int ReadKeywords(CurPos &cp, ColorKeywords *keywords, int color) {
    unsigned char obj;
    unsigned short len;

    while ((obj = GetObj(cp, len)) != 0xFF) {
        switch (obj) {
        case CF_STRING:
            {
                const char *kname = GetCharStr(cp, len);
                if (kname == 0) return -1;
                if (AddKeyword(keywords, (char) color, kname) != 1) return -1;
            }
            break;
        case CF_END:
            return 0;
        default:
            return -1;
        }
    }
    return -1;
}
#endif

int ReadEventMap(CurPos &cp, EEventMap *Map, const char * /*MapName*/) {
    unsigned char obj;
    unsigned short len;

    while ((obj = GetObj(cp, len)) != 0xFF) {
        switch (obj) {
        case CF_KEY:
            {
                EKey *Key;
                const char *s;
                int Cmd;

                if ((s = GetCharStr(cp, len)) == 0) return -1;
                if ((Key = SetKey(Map, s)) == 0) return -1;
                if ((obj = GetObj(cp, len)) != CF_KEYSUB) return -1;
                if ((Cmd = ReadCommands(cp, 0)) == -1) return -1;
                Key->Cmd = Cmd;
            }
            break;

#ifdef CONFIG_ABBREV
        case CF_ABBREV:
            {
                EAbbrev *Ab;
                const char *s;
                const char *x;
                int Cmd;

                if ((s = GetCharStr(cp, len)) == 0) return -1;
                obj = GetObj(cp, len);
                if (obj == CF_KEYSUB) {
                    if ((Cmd = ReadCommands(cp, 0)) == -1) return -1;
                    Ab = new EAbbrev(s, Cmd);
                } else if (obj == CF_STRING) {
                    x = GetCharStr(cp, len);
                    Ab = new EAbbrev(s, x);
                } else
                    return -1;
                if (Ab) {
                    Map->AddAbbrev(Ab);
                }
            }
            break;
#endif

        case CF_SETVAR:
            {
                long what;

                if (GetNum(cp, what) == 0) return -1;
                switch (GetObj(cp, len)) {
                case CF_STRING:
                    {
                        const char *val = GetCharStr(cp, len);
                        if (len == 0) return -1;
                        if (SetEventString(Map, what, val) != 0) return -1;
                    }
                    break;
                    /*                case CF_INT:
                     {
                     long num;

                     if (GetNum(cp, num) == 0) return -1;
                     if (SetModeNumber(Mode, what, num) != 0) return -1;
                     }
                     break;*/
                default:
                    return -1;
                }
            }
            break;
        case CF_END:
            return 0;
        default:
            return -1;
        }
    }
    return -1;
}

#ifdef CONFIG_SYNTAX_HILIT
int ReadColorize(CurPos &cp, EColorize *Colorize, const char *ModeName) {
    unsigned char obj;
    unsigned short len;

    long LastState = -1;

    while ((obj = GetObj(cp, len)) != 0xFF) {
        switch (obj) {
        case CF_COLOR:
            if (ReadHilitColors(cp, Colorize, ModeName) == -1) return -1;
            break;

        case CF_KEYWORD:
            {
                const char *colorstr;

                if ((colorstr = GetCharStr(cp, len)) == 0) return -1;

                unsigned int Col;
                unsigned int ColBg, ColFg;

                if (sscanf(colorstr, "%1X %1X", &ColFg, &ColBg) != 2)
                    return 0;

                Col = ColFg | (ColBg << 4);

                int color = ChColor(Col);
                if (ReadKeywords(cp, &Colorize->Keywords, color) == -1) return -1;
            }
            break;

        case CF_HSTATE:
            {
                long stateno;
                long color;

                if (Colorize->hm == 0)
                    Colorize->hm = new HMachine();

                assert(Colorize->hm != 0);

                if (GetNum(cp, stateno) == 0)
                    return -1;

                assert(stateno == LastState + 1);

                obj = GetObj(cp, len);
                assert(obj == CF_INT);

                if (GetNum(cp, color) == 0)
                    return -1;

                HState newState;

                newState.InitState();

                newState.color = color;

                Colorize->hm->AddState(newState);
                LastState = stateno;
            }
            break;

        case CF_HTRANS:
            {
                HTrans newTrans;
                long nextState;
                long matchFlags;
                const char *match;
                long color;

                if (GetNum(cp, nextState) == 0)
                    return -1;
                obj = GetObj(cp, len);
                assert(obj == CF_INT);
                if (GetNum(cp, matchFlags) == 0)
                    return -1;
                obj = GetObj(cp, len);
                assert(obj == CF_INT);
                if (GetNum(cp, color) == 0)
                    return -1;
                obj = GetObj(cp, len);
                assert(obj == CF_STRING);
                if ((match = GetCharStr(cp, len)) == 0)
                    return -1;

                newTrans.InitTrans();

                newTrans.matchFlags = matchFlags;
                newTrans.nextState = nextState;
                newTrans.color = color;

                if ((newTrans.matchFlags & MATCH_SET) ||
                    (newTrans.matchFlags & MATCH_NOTSET))
                {
                    newTrans.matchLen = 1;
                    newTrans.match = (char *)malloc(256/8);
                    assert(newTrans.match != NULL);
                    SetWordChars(newTrans.match, match);
                } else {
                    newTrans.match = strdup(match);
                    newTrans.matchLen = strlen(match);
                }

                Colorize->hm->AddTrans(newTrans);
            }
            break;

        case CF_HWTYPE:
            {
                long nextKwdMatchedState;
                long nextKwdNotMatchedState;
                long nextKwdNoCharState;
                long options;
                const char *wordChars;

                obj = GetObj(cp, len);
                assert(obj == CF_INT);
                if (GetNum(cp, nextKwdMatchedState) == 0)
                    return -1;

                obj = GetObj(cp, len);
                assert(obj == CF_INT);
                if (GetNum(cp, nextKwdNotMatchedState) == 0)
                    return -1;

                obj = GetObj(cp, len);
                assert(obj == CF_INT);
                if (GetNum(cp, nextKwdNoCharState) == 0)
                    return -1;

                obj = GetObj(cp, len);
                assert(obj == CF_INT);
                if (GetNum(cp, options) == 0)
                    return -1;

                obj = GetObj(cp, len);
                assert(obj == CF_STRING);
                if ((wordChars = GetCharStr(cp, len)) == 0)
                    return -1;

                Colorize->hm->LastState()->options = options;
                Colorize->hm->LastState()->nextKwdMatchedState = nextKwdMatchedState;
                Colorize->hm->LastState()->nextKwdNotMatchedState = nextKwdNotMatchedState;
                Colorize->hm->LastState()->nextKwdNoCharState = nextKwdNoCharState;

                if (wordChars && *wordChars) {
                    Colorize->hm->LastState()->wordChars = (char *)malloc(256/8);
                    assert(Colorize->hm->LastState()->wordChars != NULL);
                    SetWordChars(Colorize->hm->LastState()->wordChars, wordChars);
                }
            }
            break;

        case CF_HWORDS:
            {
                const char *colorstr;
                int color;

                if ((colorstr = GetCharStr(cp, len)) == 0) return -1;

                color = hcPlain_Keyword;

                if (strcmp(colorstr, "-") != 0) {
                    const char *Value = colorstr;
                    int Col;

                    if (*Value == '-') {
                        Value++;
                        if (sscanf(Value, "%1X", &Col) != 1) return -1;
                        Col |= (hcPlain_Background & 0xF0);
                    } else if (Value[1] == '-') {
                        if (sscanf(Value, "%1X", &Col) != 1) return -1;
                        Col <<= 4;
                        Col |= (hcPlain_Background & 0x0F);
                    } else {
                        if (sscanf(Value, "%2X", &Col) != 1) return -1;
                    }
                    color = Col;
                }
                if (ReadKeywords(cp, &Colorize->hm->LastState()->keywords, color) == -1) return -1;
            }
            break;

        case CF_SETVAR:
            {
                long what;

                if (GetNum(cp, what) == 0) return -1;
                switch (GetObj(cp, len)) {
                case CF_STRING:
                    {
                        const char *val = GetCharStr(cp, len);
                        if (len == 0) return -1;
                        if (SetColorizeString(Colorize, what, val) != 0) return -1;
                    }
                    break;
                    /*                case CF_INT:
                     {
                     long num;

                     if (GetNum(cp, num) == 0) return -1;
                     if (SetModeNumber(Mode, what, num) != 0) return -1;
                     }
                     break;*/
                default:
                    return -1;
                }
            }
            break;
        case CF_END:
            return 0;
        default:
            return -1;
        }
    }
    return -1;
}
#endif

int ReadMode(CurPos &cp, EMode *Mode, const char * /*ModeName*/) {
    unsigned char obj;
    unsigned short len;

    while ((obj = GetObj(cp, len)) != 0xFF) {
        switch (obj) {
        case CF_SETVAR:
            {
                long what;

                if (GetNum(cp, what) == 0) return -1;
                switch (GetObj(cp, len)) {
                case CF_STRING:
                    {
                        const char *val = GetCharStr(cp, len);
                        if (len == 0) return -1;
                        if (SetModeString(Mode, what, val) != 0) return -1;
                    }
                    break;
                case CF_INT:
                    {
                        long num;

                        if (GetNum(cp, num) == 0) return -1;
                        if (SetModeNumber(Mode, what, num) != 0) return -1;
                    }
                    break;
                default:
                    return -1;
                }
            }
            break;
        case CF_END:
            return 0;
        default:
            return -1;
        }
    }
    return -1;
}

int ReadObject(CurPos &cp, const char *ObjName) {
    unsigned char obj;
    unsigned short len;

    while ((obj = GetObj(cp, len)) != 0xFF) {
        switch (obj) {
        case CF_COLOR:
            if (ReadColors(cp, ObjName) == -1) return -1;
            break;
#ifdef CONFIG_OBJ_MESSAGES
        case CF_COMPRX:
            {
                long file, line, msg;
                const char *regexp;

                if (GetObj(cp, len) != CF_INT) return -1;
                if (GetNum(cp, file) == 0) return -1;
                if (GetObj(cp, len) != CF_INT) return -1;
                if (GetNum(cp, line) == 0) return -1;
                if (GetObj(cp, len) != CF_INT) return -1;
                if (GetNum(cp, msg) == 0) return -1;
                if (GetObj(cp, len) != CF_REGEXP) return -1;
                if ((regexp = GetCharStr(cp, len)) == 0) return -1;

                if (AddCRegexp(file, line, msg, regexp) == 0) return -1;
            }
            break;
#endif
        case CF_SETVAR:
            {
                long what;
                if (GetNum(cp, what) == 0) return -1;

                switch (GetObj(cp, len)) {
                case CF_STRING:
                    {
                        const char *val = GetCharStr(cp, len);
                        if (len == 0) return -1;
                        if (SetGlobalString(what, val) != 0) return -1;
                    }
                    break;
                case CF_INT:
                    {
                        long num;

                        if (GetNum(cp, num) == 0) return -1;
                        if (SetGlobalNumber(what, num) != 0) return -1;
                    }
                    break;
                default:
                    return -1;
                }
            }
            break;
        case CF_END:
            return 0;
        default:
            return -1;
        }
    }
    return -1;
}

int ReadConfigFile(CurPos &cp) {
    unsigned char obj;
    unsigned short len;

    {
        const char *p;

        obj = GetObj(cp, len);
        assert(obj == CF_STRING);
        if ((p = GetCharStr(cp, len)) == 0)
            return -1;

        if (ConfigSourcePath)
            free(ConfigSourcePath);

        ConfigSourcePath = strdup(p);
    }

    while ((obj = GetObj(cp, len)) != 0xFF) {
        switch (obj) {
        case CF_SUB:
            {
                const char *CmdName = GetCharStr(cp, len);

                if (ReadCommands(cp, CmdName) == -1) return -1;
            }
            break;
        case CF_MENU:
            {
                const char *MenuName = GetCharStr(cp, len);

                if (ReadMenu(cp, MenuName) == -1) return -1;
            }
            break;
        case CF_EVENTMAP:
            {
                EEventMap *EventMap = 0;
                const char *MapName = GetCharStr(cp, len);
                const char *UpMap = 0;

                if ((obj = GetObj(cp, len)) != CF_PARENT) return -1;
                if (len > 0)
                    if ((UpMap = GetCharStr(cp, len)) == 0) return -1;

                // add new mode
                if ((EventMap = FindEventMap(MapName)) == 0) {
                    EEventMap *OrgMap = 0;

                    if (strcmp(UpMap, "") != 0)
                        OrgMap = FindEventMap(UpMap);
                    EventMap = new EEventMap(MapName, OrgMap);
                } else {
                    if (EventMap->Parent == 0)
                        EventMap->Parent = FindEventMap(UpMap);
                }
                if (ReadEventMap(cp, EventMap, MapName) == -1) return -1;
            }
            break;

        case CF_COLORIZE:
            {
                EColorize *Mode = 0;
                const char *ModeName = GetCharStr(cp, len);
                const char *UpMode = 0;

                if ((obj = GetObj(cp, len)) != CF_PARENT) return -1;
                if (len > 0)
                    if ((UpMode = GetCharStr(cp, len)) == 0) return -1;

                // add new mode
                if ((Mode = FindColorizer(ModeName)) == 0)
                    Mode = new EColorize(ModeName, UpMode);
                else {
                    if (Mode->Parent == 0)
                        Mode->Parent = FindColorizer(UpMode);
                }
                if (ReadColorize(cp, Mode, ModeName) == -1)
                    return -1;
            }
            break;

        case CF_MODE:
            {
                EMode *Mode = 0;
                const char *ModeName = GetCharStr(cp, len);
                const char *UpMode = 0;

                if ((obj = GetObj(cp, len)) != CF_PARENT) return -1;
                if (len > 0)
                    if ((UpMode = GetCharStr(cp, len)) == 0) return -1;

                // add new mode
                if ((Mode = FindMode(ModeName)) == 0) {
                    EMode *OrgMode = 0;
                    EEventMap *Map;

                    if (strcmp(UpMode, "") != 0)
                        OrgMode = FindMode(UpMode);
                    Map = FindEventMap(ModeName);
                    if (Map == 0) {
                        EEventMap *OrgMap = 0;

                        if (strcmp(UpMode, "") != 0)
                            OrgMap = FindEventMap(UpMode);
                        Map = new EEventMap(ModeName, OrgMap);
                    }
                    Mode = new EMode(OrgMode, Map, ModeName);
                    Mode->fNext = Modes;
                    Modes = Mode;
                } else {
                    if (Mode->fParent == 0)
                        Mode->fParent = FindMode(UpMode);
                }
                if (ReadMode(cp, Mode, ModeName) == -1)
                    return -1;
            }
            break;
        case CF_OBJECT:
            {
                const char *ObjName;

                if ((ObjName = GetCharStr(cp, len)) == 0)
                    return -1;
                if (ReadObject(cp, ObjName) == -1)
                    return -1;
            }
            break;
        case CF_EOF:
            return 0;
        default:
            return -1;
        }
    }
    return -1;
}

int LoadConfig(int /*argc*/, char ** /*argv*/, char *CfgFileName) {
    int fd, rc;
    char *buffer = 0;
    struct stat statbuf;
    CurPos cp;

    if ((fd = open(CfgFileName, O_RDONLY | O_BINARY)) == -1)
        return -1;
    if (fstat(fd, &statbuf) != 0) {
        close(fd);
        return -1;
    }
    buffer = (char *) malloc(statbuf.st_size);
    if (buffer == 0) {
        close(fd);
        return -1;
    }
    if (read(fd, buffer, statbuf.st_size) != statbuf.st_size) {
        close(fd);
        free(buffer);
        return -1;
    }
    close(fd);

    unsigned char l[4];
    unsigned long ln;

    memcpy(l, buffer, 4);
    ln = (l[3] << 24) + (l[2] << 16) + (l[1] << 8) + l[0];

    if (ln != CONFIG_ID) {
        free(buffer);
        DieError(0, "Bad .CNF signature");
        return -1;
    }

    memcpy(l, buffer + 4, 4);
    ln = (l[3] << 24) + (l[2] << 16) + (l[1] << 8) + l[0];

    if (ln != VERNUM) {
        free(buffer);
        DieError(0, "Bad .CNF version.");
        return -1;
    }

    cp.name = CfgFileName;
    cp.sz = statbuf.st_size;
    cp.a = buffer;
    cp.c = cp.a + 2 * 4;
    cp.z = cp.a + cp.sz;
    cp.line = 1;

    rc = ReadConfigFile(cp);

    free(buffer);

    if (rc == -1) {
        DieError(1, "Error %s offset %d\n", CfgFileName, cp.c - cp.a);
    }
    return rc;
}

#include "defcfg.h"

int UseDefaultConfig() {
    CurPos cp;
    int rc;

    cp.name = "Internal Configuration";
    cp.sz = sizeof(DefaultConfig);
    cp.a = (char *)DefaultConfig;
    cp.c = (char *)DefaultConfig + 2 * 4;
    cp.z = cp.a + cp.sz;
    cp.line = 1;

    rc = ReadConfigFile(cp);

    if (rc == -1)
        DieError(1, "Error %s offset %d\n", cp.name, cp.c - cp.a);
    return rc;
}
