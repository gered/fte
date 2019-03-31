/*    con_linux.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

// If you're searching for portability it is not here ;-)

#define USE_GPM     //uncomment here to use GPM
#define USE_SCRNMAP // use USER translation table instead of direct mapping
      // The translation table is assumed to be invertible (more or less).
      // How do we get other translation tables from kernel, the USER one
      // is null mapping by default (not very useful)?

// PROBLEM: we use raw mode, and this disables the console
// switching (consoles cannot be switched when the editor is busy).
// probably needs to be fixed in the kernel (IMO kernel should
// switch consoles when raw mode is used, unless console is under
// VT_PROCESS control). Why does kernel trust user processes to do it?

// FIX: caps lock is ignored (I haven't pressed it in years, except by mistake).
// FIX: access GPM clipboard. how?

// ... some more comments below


#include <signal.h>
#include <assert.h>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/vt.h>
#include <sys/kd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h>
#include <linux/tty.h>
#include <linux/major.h>
#include <linux/kdev_t.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#ifdef USE_GPM
extern "C" {
#include <gpm.h>
}
#endif

#include "console.h"
#include "gui.h"

#define MAX_PIPES 4
//#define PIPE_BUFLEN 4096

typedef struct {
    int used;
    int id;
    int fd;
    int pid;
    int stopped;
    EModel *notify;
} GPipe;

static GPipe Pipes[MAX_PIPES] = {
    { 0 }, { 0 }, { 0 }, { 0 }
};

#define die(s) { printf("%s\n", s); exit(1); } while(0);

unsigned int VideoCols = 80;
unsigned int VideoRows = 25;
unsigned int CursorX = 0;
unsigned int CursorY = 0;
int CursorVisible = 1;
int VtNum = -1;
static int VtFd = -1;
static int VcsFd = -1;
struct termios Save_termios;
struct kbdiacrs diacr_table;
#ifdef USE_GPM
int GpmFd = -1;
#endif
int LastMouseX = 0, LastMouseY = 0;
int WindowChanged = 0;
int drawPointer = 1;
int mouseDrawn = 0;
TCell MousePosCell;
#ifdef USE_SCRNMAP
static int noCharTrans = 0;

static unsigned char toScreen[256];
static unsigned char fromScreen[256];
#endif

#define MEM_PAGE_SIZE 4096
#define VIDEO_SIZE (VideoCols * VideoRows * 2)
#define VIDEO_MAP_SIZE ((VIDEO_SIZE | (MEM_PAGE_SIZE - 1)) + 1)

int GetKeyEvent(TEvent *Event);
int GetMouseEvent(TEvent *Event);

void mouseShow() {
#ifdef USE_GPM
    if (GpmFd != -1 && VcsFd != -1 && drawPointer && mouseDrawn == 0) {
        int pos = (LastMouseX + LastMouseY * VideoCols) * 2 + 4;
        lseek(VcsFd, pos, SEEK_SET);
        read(VcsFd, &MousePosCell, 2);
        TCell newCell = MousePosCell ^ 0x7700;  // correct ?
        lseek(VcsFd, pos, SEEK_SET);
        write(VcsFd, &newCell, 2);
        mouseDrawn = 1;
    }
#endif
}

void mouseHide() {
#ifdef USE_GPM
    if (GpmFd != -1 && VcsFd != -1 && drawPointer && mouseDrawn == 1) {
        int pos = (LastMouseX + LastMouseY * VideoCols) * 2 + 4;
        lseek(VcsFd, pos, SEEK_SET);
        write(VcsFd, &MousePosCell, 2);
        mouseDrawn = 0;
    }
#endif
}

void SigWindowChanged(int arg) {
    signal(SIGWINCH, SigWindowChanged);
    WindowChanged = 1;
}

static void Cleanup() {
    ConDone();
}

static void Die(int) {
    ConDone();
    exit(66);
}

int ConInit(int XSize, int YSize) {
    int tmp;
    int mode;
    struct termios newt;
    //char ttyname[20];
    char vcsname[20];
//    struct vt_mode vtm;
    //long free_tty;
//    struct winsize ws;
    struct stat stb;
    unsigned char vc_data[4];
#ifdef USE_GPM
    Gpm_Connect conn;
#endif

    VtFd = 2; /* try stderr as output */
    if (isatty(VtFd) == 0) {
        die("not a terminal.");
    }
    if (fstat(VtFd, &stb) != 0) {
        perror("stat");
        die("stat failed");
    }
    VtNum = MINOR(stb.st_rdev);
    if (MAJOR(stb.st_rdev) != TTY_MAJOR)
        die("Not running in a virtual console.");
    if (ioctl(VtFd, KDGKBMODE, &mode) != 0)
        die("failed to get kbdmode");
#if 0
    if (mode != K_XLATE) { // X, etc...
        sprintf(ttyname, "/dev/console");
        if ((VtFd = open(ttyname, O_RDWR)) == -1)
            die("failed to open /dev/console");
        if (ioctl(VtFd, VT_OPENQRY, &free_tty) < 0 || free_tty == -1)
            die("could not find a free tty\n");
        close(VtFd);
        VtNum = free_tty;
        sprintf(ttyname, "/dev/tty%d", VtNum);
        if ((VtFd = open(ttyname, O_RDWR)) == -1)
            die("could not open tty");
    }
#endif
    sprintf(vcsname, "/dev/vcsa%d", VtNum);

     /*
      * This is the _only_ place that we use our extra privs if any,
      * If there is an error, we drop them prior to calling recovery
      * functions, if we succeed we go back as well.
      *
      * Ben Collins <bcollins@debian.org>
      */
     extern uid_t effuid;
     extern gid_t effgid;

     seteuid(effuid);
     setegid(effgid);
     VcsFd = open(vcsname, O_RDWR);
     setuid(getuid());
     setgid(getgid());

     if (VcsFd == -1) {
        perror("open");
        die("failed to open /dev/vcsa*");
    }
    if (read(VcsFd, &vc_data, 4) != 4) {
        perror("read");
        die("failed to read from /dev/vcsa*");
    }
    VideoRows = vc_data[0];
    VideoCols = vc_data[1];
    CursorX = vc_data[2];
    CursorY = vc_data[3];

#ifdef USE_SCRNMAP
    if (ioctl(VtFd, GIO_SCRNMAP, &toScreen) == -1)
        die("could not get screen character mapping!");

    {
        int c;
        for (c = 0; c < 256; c++)
            fromScreen[c] = 0;
        for (c = 0; c < 256; c++)
            fromScreen[toScreen[c]] = c;
    }
#endif

    tmp = tcgetattr(VtFd, &Save_termios);
    if (tmp) fprintf(stderr, "tcsetattr = %d\n", tmp);
    tmp = tcgetattr(VtFd, &newt);
    if (tmp) fprintf(stderr, "tcsetattr = %d\n", tmp);

    newt.c_lflag &= ~ (ICANON | ECHO | ISIG);
    newt.c_iflag = 0;
    newt.c_cc[VMIN] = 16;
    newt.c_cc[VTIME] = 1;
    tmp = tcsetattr(VtFd, TCSAFLUSH, &newt);
    if (tmp) fprintf(stderr, "tcsetattr = %d\n", tmp);

    /* set keyboard to return MEDIUMRAW mode */
    if (ioctl(VtFd, KDSKBMODE, K_MEDIUMRAW) != 0) {
        perror("Could not activate K_MEDIUMRAW mode");
        tmp = tcsetattr(VtFd, 0, &Save_termios);
        die("could not activate medium raw mode\n");
    }

    /* get keyboard diacritics table */
    if (ioctl(VtFd, KDGKBDIACR, &diacr_table) != 0) {
        perror("Could not get diacritics table");
        diacr_table.kb_cnt=0;
    }

    signal(SIGWINCH, SigWindowChanged);
    signal(SIGSEGV, Die);
    signal(SIGBUS, Die);
    signal(SIGIOT, Die);
    signal(SIGQUIT, Die);
    signal(SIGTERM, Die);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGHUP, Die);
    atexit(Cleanup);

#ifdef USE_GPM
    conn.eventMask = (unsigned short)~0U;
    conn.defaultMask = GPM_DRAG;
    conn.minMod = 0;
    conn.maxMod = (unsigned short)~0U;

    GpmFd = Gpm_Open(&conn, 0);
    mouseShow();
#endif
    return 0;
}

int ConDone() {
    if (VtFd != -1) {
        int tmp;

#ifdef USE_GPM
        if (GpmFd != -1) {
            mouseHide();
            Gpm_Close();
            GpmFd = -1;
        }
#endif
        ioctl(VtFd, KDSKBMODE, K_XLATE);
        tmp = tcsetattr(VtFd, 0, &Save_termios);
        if (tmp) fprintf(stderr, "tcsetattr = %d\n", tmp);
    }
    return 0;
}

int ConSuspend() {
    int tmp;
#ifdef USE_GPM
    mouseHide();
    Gpm_Close();
    GpmFd = -1;
#endif
    ioctl(VtFd, KDSKBMODE, K_XLATE);
    tmp = tcsetattr(VtFd, 0, &Save_termios);
    if (tmp) fprintf(stderr, "tcsetattr = %d\n", tmp);
    return 0;
}

int ConContinue() {
#ifdef USE_GPM
    Gpm_Connect conn;
#endif
    struct termios newt;
    int tmp;

    newt.c_lflag &= ~ (ICANON | ECHO | ISIG);
    newt.c_iflag = 0;
    newt.c_cc[VMIN] = 16;
    newt.c_cc[VTIME] = 1;
    tmp = tcsetattr(VtFd, TCSAFLUSH, &newt);
    if (tmp) fprintf(stderr, "tcsetattr = %d\n", tmp);

    /* set keyboard to return MEDIUMRAW mode */
    if (ioctl(VtFd, KDSKBMODE, K_MEDIUMRAW) != 0) {
        perror("ioctl KDSKBMODE");
        die("could not activate medium raw mode\n");
    }
#ifdef USE_GPM
    conn.eventMask = (unsigned short)~0U;
    conn.defaultMask = GPM_DRAG;
    conn.minMod = 0;
    conn.maxMod = (unsigned short)~0U;

    GpmFd = Gpm_Open(&conn, 0);
    mouseShow();
#endif
    return 0;
}

#ifdef USE_SCRNMAP
int conread(int fd, void *p, int len) {   // len should be a multiple of 2
    char buf[512];
    char *c = (char *)p;
    char *s = buf;

    if (noCharTrans || (len > 512)) {
        return read(fd, p, len);
    } else {
        int rlen = read(fd, buf, len);
        for (int n = 0; n < rlen; n += 2) {
            *c++ = fromScreen[(unsigned char)*s++];
            *c++ = *s++;
        }
        return rlen;
    }
}

int conwrite(int fd, void *p, int len) {  // len should be a multiple of 2
    char buf[512];
    char *s = (char *)p;
    char *c = buf;

    if (noCharTrans || (len > 512)) {
        return write(fd, p, len);
    } else {
        for (int n = 0; n < len; n += 2) {
            *c++ = toScreen[(unsigned char)*s++];
            *c++ = *s++;
        }
        return write(fd, buf, len);
    }
}
#else
#define conread(fd,b,len) read(fd,b,len)
#define conwrite(fd,b,len) write(fd,b,len)
#endif

int ConClear() {
    int X, Y;
    TCell Cell = ' ' | (0x07 << 8);
    ConQuerySize(&X, &Y);
    ConSetBox(0, 0, X, Y, Cell);
    ConSetCursorPos(0, 0);
    return 0;
}

int ConSetTitle(char *Title, char *STitle) {
    return 0;
}
int ConGetTitle(char *Title, int MaxLen, char *STitle, int SMaxLen) {
    *Title = 0;
    return 0;
}

int ConPutBox(int X, int Y, int W, int H, PCell Cell) {
    int i, hidden = 0;

    for (i = 0; i < H; i++) {
        if (LastMouseY == Y + i) { mouseHide(); hidden = 1; }
        lseek(VcsFd, 4 + ((Y + i) * VideoCols + X) * 2, SEEK_SET);
        conwrite(VcsFd, Cell, 2 * W);
        Cell += W;
        if (hidden) mouseShow();
    }
    return 0;
}

int ConGetBox(int X, int Y, int W, int H, PCell Cell) {
    int i, hidden = 0;

    for (i = 0; i < H; i++) {
        if (LastMouseY == Y + i) { mouseHide(); hidden = 1; }
        lseek(VcsFd, 4 + ((Y + i) * VideoCols + X) * 2, SEEK_SET);
        conread(VcsFd, Cell, 2 * W);
        Cell += W;
        if (hidden) mouseShow();
    }
    return 0;
}

int ConPutLine(int X, int Y, int W, int H, PCell Cell) {
    int i, hidden = 0;

    for (i = 0; i < H; i++) {
        if (LastMouseY == Y + i) { mouseHide(); hidden = 1; }
        lseek(VcsFd, 4 + ((Y + i) * VideoCols + X) * 2, SEEK_SET);
        conwrite(VcsFd, Cell, 2 * W);
        if (hidden) mouseShow();
    }
    return 0;
}

int ConSetBox(int X, int Y, int W, int H, TCell Cell) {
    TDrawBuffer B;
    MoveCh(B, Cell & 0xFF, Cell >> 8, W);
    ConPutLine(X, Y, W, H, B);
    return 0;
}

int ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count) {
    PCell C;
    TDrawBuffer B;

    if (Count == 0 || Count > H) return 0;
    C = (PCell)malloc(2 * W * H);
    if (C == 0)
        return -1;
#ifdef USE_SCRNMAP
    noCharTrans = 1;
#endif
    MoveCh(B, ' ', Fill, W);
    ConGetBox(X, Y, W, H, C);
    if (Way == csUp) {
        ConPutBox(X, Y, W, H - Count, C + W * Count);
        ConPutLine(X, Y + H - Count, W, Count, B);
    } else {
        ConPutBox(X, Y + Count, W, H - Count, C);
        ConPutLine(X, Y, W, Count, B);
    }
#ifdef USE_SCRNMAP
    noCharTrans = 0;
#endif
    free((void *)C);
    return 0;
}

int ConSetSize(int X, int Y) {
    return -1;
}

int ConQuerySize(int *X, int *Y) {
    if (X) *X = VideoCols;
    if (Y) *Y = VideoRows;
    return 0;
}

int ConSetCursorPos(int X, int Y) {
    char pos[2];

    if (X >= 0 && X < int(VideoCols)) CursorX = X;
    if (Y >= 0 && Y < int(VideoRows)) CursorY = Y;
    pos[0] = CursorX;
    pos[1] = CursorY;
    lseek(VcsFd, 2, SEEK_SET);
    write(VcsFd, pos, 2);
    return 0;
}

int ConQueryCursorPos(int *X, int *Y) {
    if (X) *X = CursorX;
    if (Y) *Y = CursorY;
    return 0;
}

int ConShowCursor(void) {
    return 0;
}

int ConHideCursor(void) {
    return 0;
}

int ConCursorVisible(void) {
    return 1;
}

int ConSetMousePos(int X, int Y) {
    return -1;
}

int ConQueryMousePos(int *X, int *Y) {
    if (X) *X = LastMouseX;
    if (Y) *Y = LastMouseY;
    return -1;
}

int ConShowMouse(void) {
    return -1;
}

int ConHideMouse(void) {
    return -1;
}

int ConMouseVisible(void) {
    return 0;
}

int ConQueryMouseButtons(int *ButtonCount) {
    if (ButtonCount) *ButtonCount = 0;
    return 0;
}

int GetEvent(TEvent *Event);
static TEvent Prev = { evNone };

int ConGetEvent(TEventMask EventMask, TEvent *Event, int WaitTime, int Delete) {
    fd_set readfds;
    struct timeval timeout;

    if (Prev.What != evNone) {
        *Event = Prev;
        if (Delete) Prev.What = evNone;
        return 1;
    }
    Event->What = evNone;

    FD_ZERO(&readfds);
    FD_SET(VtFd, &readfds);
#ifdef USE_GPM
    if (GpmFd != -1)
        FD_SET(GpmFd, &readfds);
#endif
    for (int p = 0; p < MAX_PIPES; p++)
        if (Pipes[p].used)
            if (Pipes[p].fd != -1)
                FD_SET(Pipes[p].fd, &readfds);
    if (WaitTime == -1) {
        if (select(sizeof(fd_set) * 8, &readfds, NULL, NULL, NULL) < 0) return -1;
    } else {
        timeout.tv_sec = WaitTime / 1000;
        timeout.tv_usec = (WaitTime % 1000) * 1000;
        if (select(sizeof(fd_set) * 8, &readfds, NULL, NULL, &timeout) < 0) return -1;
    }
    if (FD_ISSET(VtFd, &readfds)) {
        GetKeyEvent(Event);
        if (!Delete)
            Prev = *Event;
        return 1;
#ifdef USE_GPM
    } else if (GpmFd != -1 && FD_ISSET(GpmFd, &readfds)) {
        GetMouseEvent(Event);
        if (!Delete)
            Prev = *Event;
        return 1;
#endif
    } else {
        for (int pp = 0; pp < MAX_PIPES; pp++) {
            if (Pipes[pp].used)
                if (Pipes[pp].fd != -1)
                    if (FD_ISSET(Pipes[pp].fd, &readfds)) {
                        if (Pipes[pp].notify) {
                            Event->What = evNotify;
                            Event->Msg.View = 0;
                            Event->Msg.Model = Pipes[pp].notify;
                            Event->Msg.Command = cmPipeRead;
                            Event->Msg.Param1 = pp;
                            Pipes[pp].stopped = 0;
                        }
                        //fprintf(stderr, "Pipe %d\n", Pipes[pp].fd);
                        return 0;
                    }
        }
        return -1;
    }
    return 0;
}

int ConPutEvent(TEvent Event) {
    Prev = Event;
    return 0;
}

int ConFlush(void) {return 0;  }
int ConGrabEvents(TEventMask EventMask) { return 0; }

int shift_state = 0;
int lock_state = 0;
int slock_state = 0;
char dead_key = 0;

static const struct {
    unsigned long KeySym;
    unsigned long KeyCode;
} KeyTrans[] = {
{ K(KT_FN, K_F1),               kbF1 },
{ K(KT_FN, K_F2),               kbF2 },
{ K(KT_FN, K_F3),               kbF3 },
{ K(KT_FN, K_F4),               kbF4 },
{ K(KT_FN, K_F5),               kbF5 },
{ K(KT_FN, K_F6),               kbF6 },
{ K(KT_FN, K_F7),               kbF7 },
{ K(KT_FN, K_F8),               kbF8 },
{ K(KT_FN, K_F9),               kbF9 },
{ K(KT_FN, K_F10),              kbF10 },
{ K(KT_FN, K_F11),              kbF11 },
{ K(KT_FN, K_F12),              kbF12 },
{ K(KT_FN, K_INSERT),           kbIns | kfGray },
{ K(KT_FN, K_REMOVE),           kbDel | kfGray },
{ K(KT_FN, K_FIND),             kbHome | kfGray },
{ K(KT_FN, K_SELECT),           kbEnd | kfGray },
{ K(KT_FN, K_PGUP),             kbPgUp | kfGray },
{ K(KT_FN, K_PGDN),             kbPgDn | kfGray },
{ K(KT_CUR, K_UP),              kbUp | kfGray },
{ K(KT_CUR, K_DOWN),            kbDown | kfGray },
{ K(KT_CUR, K_LEFT),            kbLeft | kfGray },
{ K(KT_CUR, K_RIGHT),           kbRight | kfGray },
{ K(KT_SPEC, K_ENTER),          kbEnter },
{ K(KT_PAD, K_PENTER),          kbEnter | kfGray },
{ K(KT_PAD, K_PPLUS),           '+' | kfGray },
{ K(KT_PAD, K_PMINUS),          '-' | kfGray },
{ K(KT_PAD, K_PSTAR),           '*' | kfGray },
{ K(KT_PAD, K_PSLASH),          '/' | kfGray },
{ K(KT_PAD, K_P0),              kbIns },
{ K(KT_PAD, K_PDOT),            kbDel },
{ K(KT_PAD, K_P1),              kbEnd },
{ K(KT_PAD, K_P2),              kbDown },
{ K(KT_PAD, K_P3),              kbPgDn },
{ K(KT_PAD, K_P4),              kbLeft },
{ K(KT_PAD, K_P5),              kbCenter },
{ K(KT_PAD, K_P6),              kbRight },
{ K(KT_PAD, K_P7),              kbHome },
{ K(KT_PAD, K_P8),              kbUp },
{ K(KT_PAD, K_P9),              kbPgUp },
{ K(KT_FN, K_PAUSE),            kbPause },
{ K(KT_LATIN, 27),              kbEsc },
{ K(KT_LATIN, 13),              kbEnter },
{ K(KT_LATIN, 8),               kbBackSp },
{ K(KT_LATIN, 127),             kbDel },
{ K(KT_LATIN, 9),               kbTab },
{ K(KT_SHIFT, KG_SHIFT),        kbShift | kfSpecial | kfModifier },
{ K(KT_SHIFT, KG_SHIFTL),       kbShift | kfSpecial | kfModifier },
{ K(KT_SHIFT, KG_SHIFTR),       kbShift | kfSpecial | kfModifier | kfGray },
{ K(KT_SHIFT, KG_CTRL),         kbCtrl | kfSpecial | kfModifier },
{ K(KT_SHIFT, KG_CTRLL),        kbCtrl | kfSpecial | kfModifier },
{ K(KT_SHIFT, KG_CTRLR),        kbCtrl | kfSpecial | kfModifier | kfGray },
{ K(KT_SHIFT, KG_ALT),          kbAlt | kfSpecial | kfModifier },
{ K(KT_SHIFT, KG_ALTGR),        kbAlt | kfSpecial | kfModifier | kfGray },
{ 0, 0 }
};

struct {
    unsigned long KeySym;
    char Diacr;
} DeadTrans[] = {
{ K_DGRAVE, '`' },
{ K_DACUTE, '\'' },
{ K_DCIRCM, '^' },
{ K_DTILDE, '~' },
{ K_DDIERE, '"' },
{ K_DCEDIL, ',' }
};

int GetKeyEvent(TEvent *Event) {
    char keycode;

    Event->What = evNone;

    if (read(VtFd, &keycode, 1) != 1)
        return -1;

    int key = keycode & 0x7F;
    int press = (keycode & 0x80) ? 0 : 1;
    unsigned int keysym;
    int rc;
    struct kbentry kbe;
    int shift_final;
    int ShiftFlags;
    TKeyCode KeyCode;

    kbe.kb_index = key;
    kbe.kb_table = 0;
    rc = ioctl(VtFd, KDGKBENT, &kbe);
    keysym = kbe.kb_value;

    //printf("rc = %d, value = %04X, ktype=%d, kval=%d\n",
    //       rc, keysym, KTYP(keysym), KVAL(keysym));

    int ksv = KVAL(keysym);

    switch(KTYP(keysym)) {
    case KT_SHIFT:
        // :-(((, have to differentiate between shifts.
        if (ksv == KG_SHIFT) {
            if (key == 54)
                ksv = KG_SHIFTR;
            else
                ksv = KG_SHIFTL;
        } else if (ksv == KG_CTRL) {
            if (key == 97)
                ksv = KG_CTRLR;
            else
                ksv = KG_CTRLL;
        }
        if (press)
            shift_state |= (1 << ksv);
        else
            shift_state &= ~(1 << ksv);
        break;
    case KT_LOCK:
//        if (press)
//            lock_state |= (1 << ksv);
//        else
//            lock_state &= ~(1 << ksv);
        if (press) lock_state ^= (1 << ksv);
        break;
    case KT_SLOCK:
        if (press)
            slock_state |= (1 << ksv);
        else
            slock_state &= ~(1 << ksv);
        break;
    }

    shift_final = shift_state ^ lock_state ^ slock_state;

    if (shift_final & ((1 << KG_SHIFT) |
                       (1 << KG_SHIFTL) |
                       (1 << KG_SHIFTR)))
    {
        shift_final |= (1 << KG_SHIFT);
        shift_final &= ~( (1 << KG_SHIFTL) | (1 << KG_SHIFTR) );
    }
    if (shift_final & ((1 << KG_CTRL) |
                       (1 << KG_CTRLL) |
                       (1 << KG_CTRLR)))
    {
        shift_final |= (1 << KG_CTRL);
        shift_final &= ~( (1 << KG_CTRLL) | (1 << KG_CTRLR) );
    }

    ShiftFlags = 0;
    if ((shift_final & (1 << KG_SHIFT)) ||
        (shift_final & (1 << KG_SHIFTL)) ||
        (shift_final & (1 << KG_SHIFTR)))
        ShiftFlags |= kfShift;
    if ((shift_final & (1 << KG_CTRL)) ||
        (shift_final & (1 << KG_CTRLL)) ||
        (shift_final & (1 << KG_CTRLR)))
        ShiftFlags |= kfCtrl;
//    if ((shift_final & (1 << KG_ALT)) ||
//        (shift_final & (1 << KG_ALTGR)))
    if (shift_final & (1 << KG_ALT))
        ShiftFlags |= kfAlt;

    // printf("shift: %X, lock: %X, slock: %X, final: %X, ShiftFlags: %X\n",
    //        shift_state, lock_state, slock_state, shift_final, ShiftFlags);

    if (KTYP(keysym) != KT_SLOCK)
        slock_state = 0;

    KeyCode = 0;
    if (key == 14 && keysym == K(KT_LATIN,127)) {
        // we are running on a system with broken backspace key
        KeyCode = kbBackSp;
    } else for(unsigned int i = 0; i < sizeof(KeyTrans) / sizeof(KeyTrans[0]); i++) {
        if (KeyTrans[i].KeySym == keysym) {
            KeyCode = KeyTrans[i].KeyCode;
            break;
        }
    }
    if (KeyCode == 0) {
        switch (KTYP(keysym)) {
        case KT_CONS:
        case KT_FN:
        //case KT_SPEC:
        case KT_LOCK:
        case KT_SLOCK:
        //case KT_META:
        case KT_CUR:

        default:
            //if (!(shift_final & KG_ALTGR))
            //    break;
            dead_key = 0;
            break;

        case KT_SHIFT:
            break;
        case KT_LETTER:
            // take caps into account
            if (0) {
                ShiftFlags ^= kfShift;
                shift_final ^= (1 << KG_SHIFT);
            }
            // no break
        case KT_LATIN:
//            if (shift_final & (1 << KG_ALTGR))
//                ShiftFlags &= ~kfAlt;

            if (ShiftFlags & kfAlt)
                shift_final &=
                    ~(1 << KG_SHIFT);

            kbe.kb_index = key;
            kbe.kb_table = shift_final;
            rc = ioctl(VtFd, KDGKBENT, &kbe);
            if (rc != 0)
                break;

            keysym = kbe.kb_value;

            //if (KTYP(keysym) != KT_LATIN &&
            //    KTYP(keysym) != KT_LETTER &&
            //    KTYP(keysym) != KT_ASCII)
            //    break;

            KeyCode = KVAL(keysym);
            if (ShiftFlags & kfAlt)
                KeyCode = toupper(KeyCode);

            if (KTYP(keysym) == KT_DEAD) {
                for (int i = 0;(unsigned) i < sizeof(DeadTrans) / sizeof(DeadTrans[0]); i++) {
                    if (DeadTrans[i].KeySym == keysym) {
                        dead_key = DeadTrans[i].Diacr;
                        return -1;
                    }
                }
                dead_key = 0;
                return -1;
            }
            if (! (ShiftFlags & (kfAlt | kfCtrl)) && dead_key) {
                for (int i = 0; (unsigned) i < diacr_table.kb_cnt; i++) {
                    if (diacr_table.kbdiacr[i].base == KeyCode &&
                        diacr_table.kbdiacr[i].diacr == dead_key) {
                        KeyCode=diacr_table.kbdiacr[i].result;
                        break;
                    }
                }
            }
            dead_key = 0;

            break;
        }
    }

    if (KeyCode == 0)
        return -1;

    if (ShiftFlags & kfCtrl)
        if (KeyCode < 32)
            KeyCode += 64;

    KeyCode |= ShiftFlags;

    if (!press)
        KeyCode |= kfKeyUp;

    if (KeyCode != 0) {

        if (KTYP(keysym) != KT_SHIFT)
            dead_key = 0;

        if (KeyCode & kfKeyUp)
            Event->What = evKeyUp;
        else
            Event->What = evKeyDown;
        Event->Key.Code = KeyCode & ~kfKeyUp;


        // switching consoles should be done by the kernel, but
        // it is not (why, since I am not using VT_PROCESS??).
        // this has a delay if the app is working.
        // there is also a problem with shift-states (should update it
        // after getting back, but how to know when that happens, without
        // using VT_PROCESS?).
        {
            int vc = -1;

//            switch (kbCode(Event->Key.Code) | kfCtrl) {
            switch (kbCode(Event->Key.Code)) {
            case kfAlt | kfCtrl | kbF1: vc = 1; break;
            case kfAlt | kfCtrl | kbF2: vc = 2; break;
            case kfAlt | kfCtrl | kbF3: vc = 3; break;
            case kfAlt | kfCtrl | kbF4: vc = 4; break;
            case kfAlt | kfCtrl | kbF5: vc = 5; break;
            case kfAlt | kfCtrl | kbF6: vc = 6; break;
            case kfAlt | kfCtrl | kbF7: vc = 7; break;
            case kfAlt | kfCtrl | kbF8: vc = 8; break;
            case kfAlt | kfCtrl | kbF9: vc = 9; break;
            case kfAlt | kfCtrl | kbF10: vc = 10; break;
            case kfAlt | kfCtrl | kbF11: vc = 11; break;
            case kfAlt | kfCtrl | kbF12: vc = 12; break;
            }
            if (vc != -1) {
                /* perform the console switch */
                ioctl(VtFd, VT_ACTIVATE, vc);
                Event->What = evNone;
//                shift_state = lock_state = slock_state = 0; // bad
                shift_state = slock_state = 0; // bad
                return -1;
            }
        }

        return 0;
    }
    return -1;
}

int GetMouseEvent(TEvent *Event) {
#ifdef USE_GPM
    Gpm_Event e;

    Event->What = evNone;
    int rc = Gpm_GetEvent(&e);
    if (rc == 0) { 
        Gpm_Close();
        GpmFd = -1;
    } else if (rc == -1) {
    } else {
        //Gpm_FitEvent(&e);
        if ((e.type & GPM_MOVE) || (e.type & GPM_DRAG))
            Event->What = evMouseMove;
        else if (e.type & GPM_DOWN)
            Event->What = evMouseDown;
        else if (e.type & GPM_UP)
            Event->What = evMouseUp;
        else return 0;

        Event->Mouse.Count =   (e.type & GPM_SINGLE) ? 1
                             : (e.type & GPM_DOUBLE) ? 2
                             : (e.type & GPM_TRIPLE) ? 3 : 1;
        Event->Mouse.Buttons =
            ((e.buttons & GPM_B_LEFT) ? 1 : 0) |
            ((e.buttons & GPM_B_RIGHT) ? 2 : 0) |
            ((e.buttons & GPM_B_MIDDLE) ? 4 : 0);

        e.x--;
        e.y--;

        if (e.x < 0) e.x = 0;
        if (e.y < 0) e.y = 0;
        if (e.x >= int(VideoCols)) e.x = VideoCols - 1;
        if (e.y >= int(VideoRows)) e.y = VideoRows - 1;

        Event->Mouse.X = e.x;
        Event->Mouse.Y = e.y;

        Event->Mouse.KeyMask =
            ((e.modifiers & 1) ? kfShift : 0) |
            ((e.modifiers & (2 | 8)) ? kfAlt : 0) |
            ((e.modifiers & 4) ? kfCtrl : 0);

        if (LastMouseX != e.x ||
            LastMouseY != e.y)
        {
            mouseHide();
            LastMouseX = e.x;
            LastMouseY = e.y;
            mouseShow();
        }
    }

#else
    Event->What = evNone;
#endif
    return 0;
}

int ConSetCursorSize(int Start, int End) {
    return 0;
}

static PCell
SavedScreen = 0;
static int SavedX, SavedY, SaveCursorPosX, SaveCursorPosY;

int SaveScreen() {
    if (SavedScreen)
        free(SavedScreen);

    ConQuerySize(&SavedX, &SavedY);

    SavedScreen = (PCell) malloc(SavedX * SavedY * sizeof(PCell));

    if (SavedScreen)
        ConGetBox(0, 0, SavedX, SavedY, SavedScreen);
    ConQueryCursorPos(&SaveCursorPosX, &SaveCursorPosY);
    return 0;
}

int RestoreScreen() {
    if (SavedScreen) {
        ConPutBox(0, 0, SavedX, SavedY, SavedScreen);
        ConSetCursorPos(SaveCursorPosX, SaveCursorPosY);
    }
    return 1;
}

GUI::GUI(int &argc, char **argv, int XSize, int YSize) {
    fArgc = argc;
    fArgv = argv;
    ::ConInit(-1, -1);
    SaveScreen();
    ::ConSetSize(XSize, YSize);
    gui = this;
}

GUI::~GUI() {
    RestoreScreen();
    ::ConDone();

    if(SavedScreen)
    {
        free(SavedScreen);
        SavedScreen = 0;
    }

    gui = 0;
}

int GUI::ConSuspend(void) {
    RestoreScreen();
    return ::ConSuspend();
}

int GUI::ConContinue(void) {
    SaveScreen();
    return ::ConContinue();
}

int GUI::ShowEntryScreen() {
    TEvent E;

    ConHideMouse();
    RestoreScreen();
    do { gui->ConGetEvent(evKeyDown, &E, -1, 1, 0); } while (E.What != evKeyDown);
    ConShowMouse();
    if (frames)
        frames->Repaint();
    return 1;
}

int GUI::OpenPipe(char *Command, EModel *notify) {
    int i;

    for (i = 0; i < MAX_PIPES; i++) {
        if (Pipes[i].used == 0) {
            int pfd[2];

            Pipes[i].id = i;
            Pipes[i].notify = notify;
            Pipes[i].stopped = 1;

            if (pipe((int *)pfd) == -1)
                return -1;

            switch (Pipes[i].pid = fork()) {
            case -1: /* fail */
                return -1;
            case 0: /* child */
                signal(SIGPIPE, SIG_DFL);
                VtFd = -1; // for atexit handler
                close(VtFd);
                close(pfd[0]);
                close(0);
                dup2(pfd[1], 1);
                dup2(pfd[1], 2);
                close(pfd[1]);
                exit(system(Command));
            default:
                close(pfd[1]);
                fcntl(pfd[0], F_SETFL, O_NONBLOCK);
                Pipes[i].fd = pfd[0];
            }
            Pipes[i].used = 1;
            //fprintf(stderr, "Pipe Open: %d\n", i);
            return i;
        }
    }
    return -1;
}

int GUI::SetPipeView(int id, EModel *notify) {
    if (id < 0 || id > MAX_PIPES)
        return -1;
    if (Pipes[id].used == 0)
        return -1;
    //fprintf(stderr, "Pipe View: %d %08X\n", id, notify);
    Pipes[id].notify = notify;
    return 0;
}

int GUI::ReadPipe(int id, void *buffer, int len) {
    int rc;

    if (id < 0 || id > MAX_PIPES)
        return -1;
    if (Pipes[id].used == 0)
        return -1;
    //fprintf(stderr, "Pipe Read: Get %d %d\n", id, len);

    rc = read(Pipes[id].fd, buffer, len);
    //fprintf(stderr, "Pipe Read: Got %d %d\n", id, len);
    if (rc == 0) {
        close(Pipes[id].fd);
        Pipes[id].fd = -1;
        return -1;
    }
    if (rc == -1) {
        Pipes[id].stopped = 1;
        return 0;
    }
    return rc;
}

int GUI::ClosePipe(int id) {
    int status = -1;

    if (id < 0 || id > MAX_PIPES)
        return -1;
    if (Pipes[id].used == 0)
        return -1;
    if (Pipes[id].fd != -1)
        close(Pipes[id].fd);
    kill(Pipes[id].pid, SIGHUP);
    alarm(1);
    waitpid(Pipes[id].pid, &status, 0);
    alarm(0);
    //fprintf(stderr, "Pipe Close: %d\n", id);
    Pipes[id].used = 0;
    return WEXITSTATUS(status);
}

int GUI::RunProgram(int mode, char *Command) {
    int rc, W, H, W1, H1;

    ConQuerySize(&W, &H);
    ConHideMouse();
    ConSuspend();

    if (*Command == 0)  // empty string = shell
        Command = getenv("SHELL");

    rc = system(Command);

    ConContinue();
    ConShowMouse();
    ConQuerySize(&W1, &H1);

    if (W != W1 || H != H1) {
        frames->Resize(W1, H1);
    }
    frames->Repaint();
    return rc;
}

char ConGetDrawChar(int index) {
    static const char tab[] = "ڿ��ĳ�ô��\x1A����\x1B\x1A";
    static const char tab_iso[] = "++++-|+++++>.*-^v :[>";
    //static char tab[] = "\x0D\x0C\x0E\x0B\x12\x19____+>\x1F\x01\x12 ";
    static int use = 0;

    assert(index >= 0 && index < int(strlen(tab)));

    if (use == 0) {
	char * c = getenv("ISOCONSOLE");
	use = (c == NULL) ? 1 : 2;
    }

    if (use == 2)
	return tab_iso[index];

#ifdef USE_SCRNMAP
    return fromScreen[tab[index]];
#else
    return tab[index];
#endif
}
