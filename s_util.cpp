
/*    s_util.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#define BUF_SZ (sizeof(FileBuffer))

extern RxNode *CompletionFilter;

// should use DosCopy under OS/2...
int copyfile(char *f1, char *f2) { // from F1 to F2
    void *buffer;
    int fd1, fd2;
    int rd;

    buffer = FileBuffer;

    if ((fd1 = open(f1, O_RDONLY | O_BINARY)) == -1)
        return -1;
    if ((fd2 = open(f2, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0666)) == -1) {
        close(fd1);
        return -1;
    }
    while ((rd = read(fd1, buffer, BUF_SZ)) > 0) {
        if (write(fd2, buffer, rd) != rd) {
            close(fd1);
            close(fd2);
            unlink(f2);
            return -1;
        }
    }
    close(fd2);
    close(fd1);
    return 0;
}

char *MakeBackup(char *FileName, char *NewName) {
//    static char NewName[260];
    int l = strlen(FileName);
    if (l <= 0)
        return NULL;

    /* try 1 */
    strcpy(NewName, FileName);
    strcat(NewName, "~");
    if (!IsSameFile(FileName,NewName)) {
        if (access(NewName, 0) == 0)                 // Backup already exists?
            unlink(NewName);                         // Then delete the file..
        if (access(FileName, 0) != 0)                // Original found?
            return NewName;
        if (copyfile(FileName, NewName) == 0)
            return NewName;
#if 0
        if (errno == 2)
            return NewName; /* file not found */
#endif
    }
    
    /* try 2: 8.3 */
    strcpy(NewName, FileName);
    NewName[l-1] = '~';
    if (!IsSameFile(FileName,NewName)) {
        if (access(NewName, 0) == 0)                   // Backup exists?
            unlink(NewName);                           // Then delete;
        if (access(FileName, 0) != 0)                  // Original exists?
            return NewName;                            // If not-> return base..
        if (copyfile(FileName, NewName) == 0)
            return NewName;
#if 0
        if (errno == 2)
            return NewName;
#endif
    }

    return NULL;
}

int GetCharFromEvent(TEvent &E, char *Ch) {
    *Ch = 0;
    if (E.Key.Code & kfModifier)
        return 0;
    if (kbCode(E.Key.Code) == kbEsc) { *Ch = 27; return 1; }
    if (kbCode(E.Key.Code) == kbEnter) { *Ch = 13; return 1; }
    if (kbCode(E.Key.Code) == (kbEnter | kfCtrl)) { *Ch = 10; return 1; }
    if (kbCode(E.Key.Code) == kbBackSp) { *Ch = 8; return 1; }
    if (kbCode(E.Key.Code) == (kbBackSp | kfCtrl)) { *Ch = 127; return 1; }
    if (kbCode(E.Key.Code) == kbTab) { *Ch = 9; return 1; }
    if (kbCode(E.Key.Code) == kbDel) { *Ch = 127; return 1; }
    if (keyType(E.Key.Code) == kfCtrl) {
        *Ch = (char) (E.Key.Code & 0x1F);
        return 1;
    }
    if (isAscii(E.Key.Code)) {
        *Ch = (char)E.Key.Code;
        return 1;
    }
    return 0;
}

int CompletePath(const char *Base, char *Match, int Count) {
    char Name[MAXPATH];
    const char *dirp;
    char *namep;
    int len, count = 0;
    char cname[MAXPATH];
    int hascname = 0;
    RxMatchRes RM;
    FileFind *ff;
    FileInfo *fi;
    int rc;

    if (strcmp(Base, "") == 0) {
        if (ExpandPath(".", Name) != 0) return -1;
    } else {
        if (ExpandPath(Base, Name) != 0) return -1;
    }
//    SlashDir(Name);
    dirp = Name;
    namep = SepRChr(Name);
    if (namep == Name) {
        dirp = SSLASH;
        namep = Name + 1;
    } else if (namep == NULL) {
        namep = Name;
        dirp = SDOT;
    } else {
        *namep = 0;
        namep++;
    }
    
    len = strlen(namep);
    strcpy(Match, dirp);
    SlashDir(Match);
    cname[0] = 0;

    ff = new FileFind(dirp, "*",
                      ffDIRECTORY | ffHIDDEN
#if defined(USE_DIRENT)
                      | ffFAST // for SPEED
#endif
                     );
    if (ff == 0)
        return 0;
    rc = ff->FindFirst(&fi);
    while (rc == 0) {
        char *dname = fi->Name();

        // filter out unwanted files
        if ((strcmp(dname, ".") != 0) &&
            (strcmp(dname, "..") != 0) &&
            (!CompletionFilter || RxExec(CompletionFilter, dname, strlen(dname), dname, &RM) != 1))
        {
            if ((
#if defined(UNIX)
                strncmp
#else // os2, nt, ...
                strnicmp
#endif
                (namep, dname, len) == 0)
                && (dname[0] != '.' || namep[0] == '.'))
            {
                count++;
                if (Count == count) {
                    Slash(Match, 1);
                    strcat(Match, dname);
                    if (
#if defined(USE_DIRENT) // for SPEED
                        IsDirectory(Match)
#else
                        fi->Type() == fiDIRECTORY
#endif
                       )
                        Slash(Match, 1);
                } else if (Count == -1) {
                    
                    if (!hascname) {
                        strcpy(cname, dname);
                        hascname = 1;
                    } else {
                        int o = 0;
#ifdef UNIX
                        while (cname[o] && dname[o] && (cname[o] == dname[o])) o++;
#endif
#if defined(OS2) || defined(NT) || defined(DOS) || defined(DOSP32)
                        while (cname[o] && dname[o] && (toupper(cname[o]) == toupper(dname[o]))) o++;
#endif
                        cname[o] = 0;
                    }
                }
            }
        }
        delete fi;
        rc = ff->FindNext(&fi);
    }
    delete ff;
    if (Count == -1) {
        Slash(Match, 1);
        strcat(Match, cname);
        if (count == 1) SlashDir(Match);
    }
    return count;
}

int UnTabStr(char *dest, int maxlen, const char *source, int slen) {
    char *p = dest;
    int i;
    int pos = 0;

    maxlen--;
    for (i = 0; i < slen; i++) {
        if (maxlen > 0) {
            if (source[i] == '\t') {
                do {
                    if (maxlen > 0) {
                        *p++ = ' ';
                        maxlen--;
                    }
                    pos++;
                } while (pos & 0x7);
            } else {
                *p++ = source[i];
                pos++;
                maxlen--;
            }
        } else
            break;
    }

    //dest[pos] = 0;
    *p = '\0';
    return pos;
}
