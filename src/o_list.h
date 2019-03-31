/*    o_list.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef __OLIST_H__
#define __OLIST_H__

class EList;

class EListPort: public EViewPort {
public:
    EList *List;
    int Row, TopRow, LeftCol;
    int OldRow, OldTopRow, OldLeftCol, OldCount;
    EListPort(EList *L, EView *V);
    virtual ~EListPort();
    
    void StorePos();
    void GetPos();
    
    virtual void HandleEvent(TEvent &Event);
    virtual void HandleMouse(TEvent &Event);

    void PaintView(int PaintAll);
    
    virtual void UpdateView();
    virtual void RepaintView();
    virtual void UpdateStatus();
    virtual void RepaintStatus();
};

class EList: public EModel {
public:
    char *Title;
    int Row, LeftCol, TopRow, Count;
    int MouseCaptured;
    int MouseMoved;
    int NeedsUpdate, NeedsRedraw;

    EList(int createFlags, EModel **ARoot, const char *aTitle);
    virtual ~EList();
    
    virtual EViewPort *CreateViewPort(EView *V);
    EListPort *GetViewVPort(EView *V);
    EListPort *GetVPort();
    
    void SetTitle(char *ATitle);
    
    virtual int ExecCommand(int Command, ExState &State);
    virtual EEventMap *GetEventMap();
    virtual int GetContext();
    virtual int BeginMacro();
    void HandleEvent(TEvent &Event);

    
    virtual void DrawLine(PCell B, int Line, int Col, ChColor color, int Width);
    virtual char *FormatLine(int Line);

    int SetPos(int ARow, int ACol);
    void FixPos();

    virtual void RescanList();
    virtual void UpdateList();
    virtual void FreeList();
    virtual int CanActivate(int Line);
    virtual int Activate(int No);

    int MoveLeft();
    int MoveRight();
    int MoveUp();
    int MoveDown();
    int MoveLineStart();
    int MoveLineEnd();
    int MovePageUp();
    int MovePageDown();
    int ScrollLeft(int Cols);
    int ScrollRight(int Cols);
    int ScrollUp(int Rows);
    int ScrollDown(int Rows);
    int MovePageStart();
    int MovePageEnd();
    int MoveFileStart();
    int MoveFileEnd();
    int Activate();

    int UpdateRows(int minim, int maxim);
};

#endif
