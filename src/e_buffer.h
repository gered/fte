/*    e_buffer.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef _BUFFER_H_
#define _BUFFER_H_

#define bmLine    0
#define bmStream  1
#define bmColumn  2

#define E_OK          0   // all ok
#define E_CANCEL      1   // operation cancelled
#define E_ERROR       2   // error
#define E_NOMEM       3   // out of memory

#define umDelete      0
#define umInsert      1
#define umSplitLine   2
#define umJoinLine    3

#define tmNone        0
#define tmLeft        1
#define tmRight       2

typedef unsigned char TransTable[256];

#ifdef DOS /* 16 bit, sometime ;-) */
#define RWBUFSIZE     8192
#else
#define RWBUFSIZE     32768
#endif

extern char FileBuffer[RWBUFSIZE];

#define ChClass(x)  (WGETBIT(Flags.WordChars, (x)) ? 1 : 0)
#define ChClassK(x) (((x) == ' ' || (x) == 9) ? 2 : ChClass(x))
    
#define InRange(a,x,b) (((a) <= (x)) && ((x) < (b)))
#define Min(a,b) (((a) < (b))?(a):(b))
#define Max(a,b) (((a) > (b))?(a):(b))

#define NextTab(pos,ts) (((pos) / (ts) + 1) * (ts))

#define GapLine(x,g,c,a) (((x) < (g)) ? (x) : ((x) + (a) - (c)))

typedef class ELine* PELine;
typedef class EPoint* PEPoint;

#define CHAR_TRESHOLD  0x3U

class ELine {
public:
    int Count;
    char *Chars;
#ifdef CONFIG_SYNTAX_HILIT
    hlState StateE;
#endif
    
    ELine(int ACount, char *AChars);
    ELine(char *AChars, int ACount);
    ~ELine();
    int Allocate(unsigned int Bytes);

//    int Length(EBufferFlags *CurFlags);
};

class EPoint {
public:
    int Row;
    int Col;

//    EPoint(EPoint &M) { Row = M.Row; Col = M.Col; }
    EPoint(int aRow = 0, int aCol = 0) { Row = aRow; Col = aCol; }
    ~EPoint() {}
};

typedef struct _UndoStack {
    int NextCmd, Record, Undo;
    int UndoPtr;
    int Num;
    void **Data;  
    int *Top;
} UndoStack;

#ifdef CONFIG_OBJ_ROUTINE
class RoutineView;

typedef struct _RoutineList {
    int Count;
    int *Lines;
} RoutineList;
#endif

#ifdef CONFIG_BOOKMARKS
typedef struct {
    char *Name;
    EPoint BM;
} EBookmark;
#endif

typedef struct {
    int line;
    unsigned char level;
    unsigned char open;
    unsigned short flags;
} EFold;

class EEditPort: public EViewPort {
public:
    EBuffer *Buffer;
    EPoint TP, OldTP;
    EPoint CP;
    int Rows, Cols;

    EEditPort(EBuffer *B, EView *V);
    virtual ~EEditPort();
    
    virtual void HandleEvent(TEvent &Event);
    virtual void HandleMouse(TEvent &Event);
    virtual void UpdateView();
    virtual void RepaintView();
    virtual void UpdateStatus();
    virtual void RepaintStatus();
    
    virtual void Resize(int Width, int Height);
    int SetTop(int Col, int Row);
    virtual void GetPos();
    virtual void StorePos();
    void DrawLine(int L, TDrawBuffer B);
    void ScrollY(int Delta);
    void RedrawAll();
};

class EBuffer: public EModel {
public:
    //char *Name;
    char *FileName;
    int Modified;
    EPoint TP;
    EPoint CP;
    EPoint BB;
    EPoint BE;
    EPoint PrevPos;
    EPoint SavedPos;

    EBufferFlags Flags;
    EMode *Mode;
    int BlockMode;
    int ExtendGrab;
    int AutoExtend;
    int Loaded;

#ifdef CONFIG_UNDOREDO
    UndoStack US;
#endif

    struct stat FileStatus;
    int FileOk;
    int Loading;

    int RAllocated;   // text line allocation
    int RGap;
    int RCount;
    PELine *LL;
    
    int VAllocated;   // visible lines
    int VGap;
    int VCount;
    int *VV;

#ifdef CONFIG_FOLDS
    int FCount;
    EFold *FF;
#endif

    EPoint Match;
    int MatchLen;
    int MatchCount;
    RxMatchRes MatchRes;

#ifdef CONFIG_BOOKMARKS
    int BMCount;
    EBookmark *BMarks;
#endif
    
#ifdef CONFIG_OBJ_ROUTINE
    RoutineList rlst;
    RoutineView *Routines;
#endif

    int MinRedraw, MaxRedraw;
    int RedrawToEos;
    
#ifdef CONFIG_WORD_HILIT
    char **WordList;
    int WordCount;
#endif
#ifdef CONFIG_SYNTAX_HILIT
    SyntaxProc HilitProc;
    int StartHilit, EndHilit;
#endif
    
    // constructors
    EBuffer(int createFlags, EModel **ARoot, const char *AName);
    ~EBuffer();
    virtual void DeleteRelated();
    
    virtual EViewPort *CreateViewPort(EView *V);
    EEditPort *GetViewVPort(EView *V);
    EEditPort *GetVPort();
    virtual int CanQuit();
    virtual int ConfQuit(GxView *V, int multiFile = 0);
    
    virtual int GetContext();
    virtual EEventMap *GetEventMap();
    virtual int BeginMacro();
    virtual int ExecCommand(int Command, ExState &State);
    virtual void HandleEvent(TEvent &Event);
    
    virtual void GetName(char *AName, int MaxLen);
    virtual void GetPath(char *APath, int MaxLen);
    virtual void GetInfo(char *AInfo, int MaxLen);
    virtual void GetTitle(char *ATitle, int MaxLen, char *ASTitle, int SMaxLen);

    PELine RLine(int No) {
#ifdef DEBUG_EDITOR
        int N = GapLine(No, RGap, RCount, RAllocated);
        if (!((No < RCount) && (No >= 0) && (LL[N]))) {
            printf("Get No = %d/%d Gap=%d RAlloc = %d, VCount = %d\n", No, RCount, RGap, RAllocated, VCount);
            assert((No < RCount) && (No >= 0) && (LL[N]));
        }
#endif
        return LL[GapLine(No, RGap, RCount, RAllocated)];
    }
    void RLine(int No, PELine L) {
#ifdef DEBUG_EDITOR
        if (!((No >= 0))) printf("Set No = %d\n", No);
        assert((No >= 0));
#endif
        LL[GapLine(No, RGap, RCount, RAllocated)] = L;
    }
    int Vis(int No) {
#ifdef DEBUG_EDITOR
        if (No < 0 || No >= VCount) {
            printf("Vis get no %d of %d\n", No, VCount);
            assert (No >= 0 && No < VCount);
        }
#endif
        return VV[GapLine(No, VGap, VCount, VAllocated)];
    }
    void Vis(int No, int V) {
#ifdef DEBUG_EDITOR
        if (No < 0 || No >= VCount) {
            printf("Vis set no %d of %d to %d\n", No, VCount, V);
            assert (No >= 0 && No < VCount);
        }
#endif
        VV[GapLine(No, VGap, VCount, VAllocated)] = V;
    }
    PELine VLine(int No) {
#ifdef DEBUG_EDITOR
        if (!((No < VCount) && (No >= 0))) {
            printf("VGet No = %d\n", No);
            assert((No < VCount) && (No >= 0));
        }
        if (Vis(No) < 0)
            assert(1 == 0);
#endif
        return RLine(No + Vis(No));
    }
    void VLine(int No, PELine L) {
#ifdef DEBUG_EDITOR
        if (!((No >= 0))) {
            printf("VSet No = %d\n", No);
            assert((No >= 0));
        }
        if (VV[No] < 0)
            assert(1 == 0);
#endif
        RLine(No + Vis(No), L);
    }

    int VToR(int No) {
#ifdef DEBUG_EDITOR
        if (!(No < VCount)) {
            printf("Get No = %d\n", No);
            assert((No < VCount));
        }
#endif
        return No + Vis(No);
    }
    
    int RToV(int No);
    int RToVN(int No);
    
    // allocation  
    int Allocate(int ACount);
    int MoveRGap(int RPos);
    int AllocVis(int ACount);
    int MoveVGap(int VPos);
    
    int Modify();
    int Clear();

#ifdef CONFIG_UNDOREDO
    int FreeUndo();
#endif
    // internal primitives  
    int ValidPos(EPoint W);
    int RValidPos(EPoint W);
    int LoadRegion(EPoint *A, int FH, int StripChar, int LineChar);
    int SaveRegion(EPoint *A, EPoint *Z, int FH, int AddCR, int AddLF, int Mode);
    
    int AssertLine(int Line);
    int InsertLine(EPoint Pos, int ACount, char *AChars);
    
    int UpdateMarker(int Type, int Line, int Col, int Lines, int Cols);
    int UpdateMark(EPoint &M, int Type, int Line, int Col, int Lines, int Cols);
    void UpdateVis(EPoint &M, int Row, int Delta);
    void UpdateVisible(int Row, int Delta);
    int LoadFrom(char *AFileName);
    int SaveTo(char *AFileName);
    
    int IsBlockStart();
    int IsBlockEnd();
    int BlockType(int Mode);
    int BeginExtend();
    int EndExtend();
    int CheckBlock();
    int BlockRedraw();
    int SetBB(EPoint M);
    int SetBE(EPoint M);
    
    int Load();
    int Save();
    int Reload();
    int FilePrint();
    int SetFileName(const char *AFileName, const char *AMode);
    
    int SetPos(int Col, int Row, int tabMode = tmNone);
    int SetPosR(int Col, int Row, int tabMode = tmNone);
    int CenterPos(int Col, int Row, int tabMode = tmNone);
    int CenterPosR(int Col, int Row, int tabMode = tmNone);
    int SetNearPos(int Col, int Row, int tabMode = tmNone);
    int SetNearPosR(int Col, int Row, int tabMode = tmNone);
    int CenterNearPos(int Col, int Row, int tabMode = tmNone);
    int CenterNearPosR(int Col, int Row, int tabMode = tmNone);
    int LineLen(int Row);
    int LineChars(int Row);

/////////////////////////////////////////////////////////////////////////////
// Undo/Redo Routines
/////////////////////////////////////////////////////////////////////////////
    
    int NextCommand();
#ifdef CONFIG_UNDOREDO
    int PushUData(void *data, int len);
    int PushULong(unsigned long l);
    int PushUChar(unsigned char ch);
    int PopUData(void *data, int len);
    int GetUData(int No, int pos, void **data, int len);
    int Undo(int undo);
    int Undo();
    int Redo();
    int BeginUndo();
    int EndUndo();
    int PushBlockData();
#endif

/////////////////////////////////////////////////////////////////////////////
// Primitive Editing
/////////////////////////////////////////////////////////////////////////////

    //int ExpReg(int Row, int Ofs, int ACount, int &B, int &E);
    int ScreenPos(ELine *L, int Offset);
    int CharOffset(ELine *L, int ScreenPos);
    int DelLine(int Row, int DoMark = 1);
    int UnTabPoint(int Row, int Col);
    int InsLine(int Row, int DoAppend, int DoMark = 1);
    int DelChars(int Row, int Ofs, int ACount);
    int InsChars(int Row, int Ofs, int ACount, char *Buffer);
    int ChgChars(int Row, int Ofs, int ACount, char *Buffer);
    int DelText(int Row, int Col, int ACount, int DoMark = 1);
    int InsText(int Row, int Col, int ACount, char *Buffer, int DoMark = 1);
    int InsLineText(int Row, int Col, int ACount, int Pos, PELine Line);
    int SplitLine(int Row, int Col);
    int JoinLine(int Row, int Col);
    int CanUnfold(int Row);
    int PadLine(int Row, int Length);
    
    int ShowRow(int Row);
    int HideRow(int Row);
    int ExposeRow(int Row); // make row visible (open all folds containing)

/////////////////////////////////////////////////////////////////////////////    
// Redraw/Windowing Routines
/////////////////////////////////////////////////////////////////////////////    

    void Draw(int Line0, int LineE);
    void DrawLine(TDrawBuffer B, int L, int C, int W, int &HilitX);
    void Hilit(int FromRow);
    void Rehilit(int ToRow);
    void Redraw();
    void FullRedraw();
    int  GetHilitWord(int len, char *str, ChColor &clr, int IgnCase = 0);
    
/////////////////////////////////////////////////////////////////////////////   
// Utility Routines
/////////////////////////////////////////////////////////////////////////////
    
    int LineIndented(int Row);
    int IndentLine(int Row, int Indent);
#ifdef CONFIG_SYNTAX_HILIT
    int GetMap(int Row, int *StateLen, hsState **StateMap);
#endif
    int FindStr(char *Data, int Len, int Options);
    int FindRx(RxNode *Rx, int Options);
    int Find(SearchReplaceOptions &opt);
    int IsLineBlank(int Row);
    int TrimLine(int Row);
    
#ifdef CONFIG_OBJ_ROUTINE
    int ScanForRoutines();
#endif

/////////////////////////////////////////////////////////////////////////////
// Bookmark Routines
/////////////////////////////////////////////////////////////////////////////
    
#ifdef CONFIG_BOOKMARKS
    int PlaceBookmark(char *Name, EPoint P);
    int RemoveBookmark(char *Name);
    int GetBookmark(char *Name, EPoint &P);
    int GotoBookmark(char *Name);
#endif
    
/////////////////////////////////////////////////////////////////////////////
// Editing Routines
/////////////////////////////////////////////////////////////////////////////
    
    int     MoveLeft();
    int     MoveRight();
    int     MoveUp();
    int     MoveDown();
    int     MovePrev();
    int     MoveNext();
    int     MoveWordLeftX(int start);
    int     MoveWordRightX(int start);
    int     MoveWordLeft();
    int     MoveWordRight();
    int     MoveWordPrev();
    int     MoveWordNext();
    int     MoveWordEndLeft();
    int     MoveWordEndRight();
    int     MoveWordEndPrev();
    int     MoveWordEndNext();
    int     MoveWordOrCapLeft();
    int     MoveWordOrCapRight();
    int     MoveWordOrCapPrev();
    int     MoveWordOrCapNext();
    int     MoveWordOrCapEndLeft();
    int     MoveWordOrCapEndRight();
    int     MoveWordOrCapEndPrev();
    int     MoveWordOrCapEndNext();
//    int     MoveWordStart();
//    int     MoveWordEnd();
    int     MoveLineStart();
    int     MoveLineEnd();
    int     MovePageUp();
    int     MovePageDown();
    int     MovePageLeft();
    int     MovePageRight();
    int     MovePageStart();
    int     MovePageEnd();
    int     MoveFileStart();
    int     MoveFileEnd();
    int     MoveBlockStart();
    int     MoveBlockEnd();
    int     ScrollLeft(int Cols);
    int     ScrollRight(int Cols);
    int     ScrollDown(int Lines);
    int     ScrollUp(int Lines);
    int     MoveToLine();
    int     MoveToColumn();
    int     MoveFirstNonWhite();
    int     MoveLastNonWhite();
    int     MovePrevEqualIndent();
    int     MoveNextEqualIndent();
    int     MovePrevTab();
    int     MoveNextTab();
    int     MoveLineTop();
    int     MoveLineCenter();
    int     MoveLineBottom();
    int     MovePrevPos();
    int     MoveSavedPosCol();
    int     MoveSavedPosRow();
    int     MoveSavedPos();
    int     SavePos();
    int     MoveTabStart();
    int     MoveTabEnd();
    int     MoveFoldTop();
    int     MoveFoldPrev();
    int     MoveFoldNext();
    int     MoveBeginOrNonWhite();
    int     MoveBeginLinePageFile();
    int     MoveEndLinePageFile();
    
    int     KillLine();
    int     KillChar();
    int     KillCharPrev();
    int     KillWord();
    int     KillWordPrev();
    int     KillWordOrCap();
    int     KillWordOrCapPrev();
    int     KillToLineStart();
    int     KillToLineEnd();
    int     KillBlock();
    int     BackSpace();
    int     Delete();
    int     CompleteWord();
    int     KillBlockOrChar();
    int     KillBlockOrCharPrev();

#define ccUp       0
#define ccDown     1
#define ccToggle   2
    
    int     CharTrans(TransTable tab);
    int     CharCaseUp();
    int     CharCaseDown();
    int     CharCaseToggle();

    int     LineTrans(TransTable tab);
    int     LineCaseUp();
    int     LineCaseDown();
    int     LineCaseToggle();

    int     BlockTrans(TransTable tab);
    int     BlockCaseUp();
    int     BlockCaseDown();
    int     BlockCaseToggle();

    int     CharTrans(ExState &State);
    int     LineTrans(ExState &State);
    int     BlockTrans(ExState &State);
    int     GetTrans(ExState &State, TransTable tab);
    
    int     LineInsert();
    int     LineAdd();
    int     LineSplit();
    int     LineJoin();
    int     LineNew();
    int     LineIndent();
    int     LineTrim();
    int     LineCenter();
    int     FileTrim();
    int     BlockTrim();

#ifdef CONFIG_UNDOREDO
    int     CanUndo();
    int     CanRedo();
#endif
    
    int     LineLen();
    int     LineCount();
    int     CLine();
    int     CColumn();

    int     InsertChar(char aCh);
    int     TypeChar(char aCh);
    int     InsertString(char *aStr, int aCount);
    int     InsertSpacesToTab(int TSize);
    int     InsertTab();
    int     InsertSpace();
    int     SelfInsert();
#ifdef CONFIG_WORDWRAP
    int     DoWrap(int WrapAll);
    int     WrapPara();
#endif
    int     InsPrevLineChar();
    int     InsPrevLineToEol();
    int     LineDuplicate();
    
    int     GetMatchBrace(EPoint &M, int MinLine, int MaxLine, int show);
    int     MatchBracket();
    int     HilitMatchBracket();
    
    int     BlockBegin();
    int     BlockEnd();
    int     BlockUnmark();
    int     BlockCut(int Append);
    int     BlockCopy(int Append);
    int     BlockPaste();
    int     BlockKill();
    int     BlockIndent();
    int     BlockUnindent();
    int     BlockClear();
    int     BlockMarkStream();
    int     BlockMarkLine();
    int     BlockMarkColumn();
    int     BlockReadFrom(char *aFileName, int blockMode);
    int     BlockWriteTo(char *aFileName, int Append = 0);
    int     BlockExtendBegin();
    int     BlockExtendEnd();
    int     BlockReIndent();
    int     BlockIsMarked();
    int     BlockPasteStream();
    int     BlockPasteLine();
    int     BlockPasteColumn();
    int     BlockSelectWord();
    int     BlockSelectLine();
    int     BlockSelectPara();
    int     BlockPrint();
    int     BlockSort(int Reverse);
    int     ClipClear();
    int     BlockUnTab();
    int     BlockEnTab();

    int     ToggleAutoIndent();
    int     ToggleInsert();
    int     ToggleExpandTabs();
    int     ToggleShowTabs();
    int     ToggleUndo();
    int     ToggleReadOnly();
    int     ToggleKeepBackups();
    int     ToggleMatchCase();
    int     ToggleBackSpKillTab();
    int     ToggleDeleteKillTab();
    int     ToggleSpaceTabs();
    int     ToggleIndentWithTabs();
    int     ToggleBackSpUnindents();
    int     ToggleWordWrap();
    int     ToggleTrim();
    int     ToggleShowMarkers();
    
    int     SetLeftMargin();
    int     SetRightMargin();
    
    int     ShowPosition();

    int     Search(ExState &State, char *aString, int Options, int CanResume = 0);
    int     SearchAgain(ExState &State, unsigned int Options);
    int     SearchReplace(ExState &State, char *aString, char *aReplaceString, int Options);
    int     Search(ExState &State);
    int     SearchB(ExState &State);
    int     SearchRx(ExState &State);
    int     SearchAgain(ExState &State);
    int     SearchAgainB(ExState &State);
    int     SearchReplace(ExState &State);
    int     SearchReplaceB(ExState &State);
    int     SearchReplaceRx(ExState &State);
    
#ifdef CONFIG_WORD_HILIT
    int     HilitAddWord(const char *Word);
    int     HilitFindWord(const char *Word);
    int     HilitRemoveWord(const char *Word);
    int     HilitWord();
#endif
    int     SearchWord(int Flags);

#ifdef CONFIG_FOLDS
    int     FindFold(int Line);
    int     FindNearFold(int Line);
    int     FoldCreate(int Line);
    int     FoldCreateByRegexp(char *Regexp);
    int     FoldDestroy(int Line);
    int     FoldDestroyAll();
    int     FoldPromote(int Line);
    int     FoldDemote(int Line);
    int     FoldOpen(int Line);
    int     FoldOpenAll();
    int     FoldOpenNested();
    int     FoldClose(int Line);
    int     FoldCloseAll();
    int     FoldToggleOpenClose();
#endif
    
    int     ChangeMode(char *Mode);
    int     ChangeKeys(char *Mode);
    int     ChangeFlags(char *Mode);

    int ScrollLeft(ExState &State);
    int ScrollRight(ExState &State);
    int ScrollDown(ExState &State);
    int ScrollUp(ExState &State);
    
    /* editor functions with user interface */
    
    int MoveToColumn(ExState &State);
    int MoveToLine(ExState &State);
    int FoldCreateByRegexp(ExState &State);
#ifdef CONFIG_BOOKMARKS
    int PlaceBookmark(ExState &State);
    int RemoveBookmark(ExState &State);
    int GotoBookmark(ExState &State);
#endif
    int InsertString(ExState &State);
    int SelfInsert(ExState &State);
    int FileReload(ExState &State);
    int FileSaveAs(char *FileName);
    int FileSaveAs(ExState &State);
    int FileWriteTo(char *FileName);
    int FileWriteTo(ExState &State);
    int BlockReadX(ExState &State, int BlockMode);
    int BlockRead(ExState &State);
    int BlockReadStream(ExState &State);
    int BlockReadLine(ExState &State);
    int BlockReadColumn(ExState &State);
    int BlockWrite(ExState &State);
    int Find(ExState &State);
    int FindReplace(ExState &State);
    int FindRepeat(ExState &State);
    int FindRepeatOnce(ExState &State);
    int FindRepeatReverse(ExState &State);
    int InsertChar(ExState &State);
    int TypeChar(ExState &State);
    int ChangeMode(ExState &State);
    int ChangeKeys(ExState &State);
    int ChangeFlags(ExState &State);
    int ChangeTabSize(ExState &State);
    int ChangeRightMargin(ExState &State);
    int ChangeLeftMargin(ExState &State);
    
#ifdef CONFIG_I_ASCII
    int ASCIITable(ExState &State);
#endif

#ifdef CONFIG_TAGS
    int FindTag(ExState &State);
    int FindTagWord(ExState &State);
#endif

    int SetCIndentStyle(ExState &State);

    int FindFunction(int delta, int way);
    int BlockMarkFunction();
    int IndentFunction();
    int MoveFunctionPrev();
    int MoveFunctionNext();
    int InsertDate(ExState& state);
    int InsertUid();

    int ShowHelpWord(ExState &State);

    int PlaceGlobalBookmark(ExState &State);
    int PushGlobalBookmark();

    virtual int GetStrVar(int var, char *str, int buflen);
    virtual int GetIntVar(int var, int *value);

    int SetIndentWithTabs(ExState &State);
    int FoldCreateAtRoutines();
};

extern EBuffer *SSBuffer;
extern SearchReplaceOptions LSearch;

extern int suspendLoads;

int DoneEditor();

EBuffer *FindFile(char *FileName);

#endif
