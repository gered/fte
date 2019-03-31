/*    s_files.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "sysdep.h"
#include "s_files.h"

#if defined(OS2)
#define INCL_DOS
#include <os2.h>
#endif

#if defined(NT)
#   define  WIN32_LEAN_AND_MEAN 1
#   include <windows.h>
#endif

#if defined(DOSP32)
#   include "port.h"
#endif


#if defined(DJGPP)
static inline int is_end(int c)
{
    return c == 0 || c == '.' || c == '/';
}

static inline int is_filename_char(int c)
{
    return (strchr("+<>",c) == NULL);
}


static void my_fixpath(const char *in, char *out) {
    // this does most of the cleanup
    _fixpath(in,out);
    if (_USE_LFN)
        return;

    // handle 8+3 restrictions
    char tmp[MAXPATH];
    char *t = tmp;
    char *o = out;
    if (o[0] && o[1] == ':') {
        *t++ = *o++;
        *t++ = *o++;
    }
    while (*o) {
        int i;
        // copy over slash
        if (*o == '/')
            *t++ = *o++;
        // copy over filename (up to 8 chars)
        for (i = 0; i < 8 && !is_end(*o); o++)
            if (is_filename_char(*o))
                *t++ = *o, i++;
                // copy over extension (up to 3 chars)
                if (*o == '.') {
                    // don't copy a trailing '.' unless following a ':'
                    if (o[1] == 0 && o > out && o[-1] != ':')
                        break;
                    *t++ = (i > 0 ? '.' : '_');
                    o++;
                    for (i = 0; i < 3 && !is_end(*o); o++)
                        if (is_filename_char(*o))
                            *t++ = *o, i++;
                }
                // find next slash
                while (*o && *o != '/')
                    o++;
    }
    *t++ = 0;
#if 0
    if (strcmp(out,tmp) != 0)
        fprintf(stderr,"fix: '%s'->'%s'\n",out,tmp);
#endif
    strcpy(out,tmp);
}
#endif


char *Slash(char *Path, int Add) {
    int len = strlen(Path);

    if (Add) {
        if ((len == 0) || !ISSLASH(Path[len - 1])) {
            Path[len] = SLASH;
            Path[len+1] = 0;
        }
    } else {
        if ((len > 1)
#if PATHTYPE == PT_DOSISH
            && ((len > 3) || (Path[1] != ':'))
#endif
           ) {
            if (ISSLASH(Path[len - 1])) {
                Path[len - 1] = 0;
            }
        }
    }
    return Path;
}

char *SlashDir(char *Path) {
    int len = strlen(Path);
    if (len > 1) {
#if PATHTYPE == PT_DOSISH
        if ((len == 2) && Path[1] == ':') {
#ifdef DJGPP
            char tmp[MAXPATH];
            strcpy(tmp,Path);
            my_fixpath(tmp,Path);
            strcat(Path,SSLASH);
#else
            Path[2] = SLASH;
            Path[3] = 0;
#endif
        }
        else
#endif
            if (!ISSLASH(Path[len - 1])) {
                struct stat statbuf;
                if (stat(Path, &statbuf) == 0) {
                    if (S_ISDIR(statbuf.st_mode)) {
                        Path[len] = SLASH;
                        Path[len+1] = 0;
                    }
                }
            }
    }
    return Path;
}

int IsDirectory(const char *Path) {
    int len = strlen(Path);
    if (len > 0) {
#if PATHTYPE == PT_DOSISH
        if ((len == 2) && Path[1] == ':')
            return 1;
        else
#endif
            if (!ISSLASH(Path[len - 1])) {
                struct stat statbuf;
                if (stat(Path, &statbuf) == 0) {
                    if (S_ISDIR(statbuf.st_mode)) {
                        return 1;
                    }
                }
                return 0;
            } else
                return 1;
    }
    return 0;
}

#if PATHTYPE == PT_DOSISH
static int GetDiskCurDir(int drive, char *dir) {
    if (drive < 1 || drive > 26)
        return -1;

#if defined(__EMX__)
    return (_getcwd1(dir, (char)(drive + 'A' - 1)) == 0) ? 0 : -1;
#elif defined(OS2)
    {
        ULONG len = MAXPATH - 4; // 'c:\0'

        return (DosQueryCurrentDir(drive, dir, &len) == 0) ? 0 : -1;
    }
#elif defined(NT)
    {
        char orig[MAXPATH], newdrive[MAXPATH];

        // set to new drive, get directory and restore original directory
        if (!GetCurrentDirectory(sizeof(orig), orig))
            return -1;
        newdrive[0] = drive + 'A' - 1;
        newdrive[1] = ':';
        newdrive[2] = 0;
        if (!SetCurrentDirectory(newdrive))
            return -1;
        if (!GetCurrentDirectory(sizeof(newdrive), newdrive))
            return -1;
        strcpy(dir, newdrive + 3);

        SetCurrentDirectory(orig); // ? check
        return 0;
    }
#elif defined(DOS) || defined(DOSP32)
    return (plGetcurdir(drive, dir) != 0);
#endif
}

static int SetDrive(int drive) { // 1 = A, 2 = B, 3 = C, ...
    if (drive < 1 || drive > 26)
        return -1;

    // mess
    // _chdrive seems to be most portable between dosish systems,
    // but seem to take different arguments ???

#if defined(__EMX__)
    return _chdrive(drive + 'A' - 1);
#elif defined(OS2)
    return (DosSetDefaultDisk(drive) == 0) ? 0 : -1;
#elif defined(NT)
    char buf[3];

    buf[0] = (char)(drive + 'A' - 1);
    buf[1] = ':';
    buf[2] = '\0';
    return SetCurrentDirectory(buf) ? 0 : -1;
#elif defined(DOS) || defined(DOSP32)
    unsigned int ndr;

    _dos_setdrive(drive, &ndr);
    return 0; //(ndr == drive) ? 0 : -1; // ?
#endif
}
#endif

#if PATHTYPE == PT_UNIXISH
int RemoveDots(char *Source, char *Dest) {
    char *p;
    char *d;

    p = Source;
    d = Dest;
    while (*p) {
        // if ((strncmp(p, SSLASH SSLASH, 2) == 0)) {
        if (ISSLASH(p[0]) && ISSLASH(p[1])) {
            p++;
            // } else if ((strncmp(p, SSLASH ".." SSLASH, 4) == 0) || (strcmp(p, SSLASH "..") == 0)) {
        } else if (ISSLASH(p[0]) && p[1] == '.' && p[2] == '.' &&
                   (ISSLASH(p[3]) || p[3] == 0)) {
            p += 3;
            while ((d > Dest) && !ISSEP(*d)) d--;
            *d = 0;
            continue;
            // } else if ((strncmp(p, SSLASH "." SSLASH, 3) == 0) || (strcmp(p, SSLASH ".") == 0)) {
        } else if (ISSLASH(p[0]) && p[1] == '.' &&
                   (ISSLASH(p[2]) || p[2] == 0)) {
            p += 2;
            continue;
        }
        *d++ = *p++;
        *d = 0;
    }
    *d = 0;
    return 0;
}
#endif

int ExpandPath(const char *Path, char *Expand) {
    char Name[MAXPATH];

    if (Path[0] == 0) {
        Expand[0] = 0;
        return 0;
    }
#if PATHTYPE == PT_DOSISH
    int slashed = 0;

    strcpy(Name, Path);
    if (Name[0] != 0)
        slashed = ISSLASH(Name[strlen(Name)-1]);
    Slash(Name, 0);
#if defined(DJGPP)
    my_fixpath(Name, Expand);
#else
    //puts(Name);
    if (Name[0] && Name[1] == ':' && Name[2] == 0) { // '?:'
        int drive = Name[0];

        strcpy(Expand, Name);
        Expand[2] = '\\';
        Expand[3] = 0;

        drive = (int)(toupper(Name[0]) - 'A' + 1);

        if (GetDiskCurDir(drive, Expand + 3) == -1)
            return -1;
    } else {
#if defined(__EMX__)
        if (_fullpath(Expand, Name, MAXPATH) == -1) return -1;
#else
        if (_fullpath(Expand, Name, MAXPATH) == NULL) return -1;
#endif
    }
#endif
#if defined(__EMX__)
    {
        char *p = Expand;

        if (p && *p) do {
            if (ISSLASH(*p))
                *p = SLASH;
        } while (*p++);
    }
#endif
    if (slashed)
        SlashDir(Expand);
    //puts(Expand);
    return 0;
#endif
#if PATHTYPE == PT_UNIXISH
    char Name2[MAXPATH];
    char *path, *p;

    strcpy(Name, Path);
    switch (Name[0]) {
    case SLASH:
        break;
    case '~':
        if (Name[1] == SLASH || Name[1] == 0) {
            path = Name + 1;
	    strncpy(Name2, getenv("HOME"), sizeof(Name2) - 1);
            Name2[sizeof(Name2) - 1] = 0;
        } else {
            struct passwd *pwd;

            p = Name;
            p++;
            while (*p && (*p != SLASH)) p++;
            if (*p == SLASH) {
                path = p + 1;
                *p = 0;
            } else {
                path = p;
            }
            pwd = getpwnam(Name + 1);
            if (pwd == NULL)
                return -1;
            strcpy(Name2, pwd->pw_dir);
        }
        if (path[0] != SLASH)
            Slash(Name2, 1);
        strcat(Name2, path);
        strcpy(Name, Name2);
        break;
    default:
        if (getcwd(Name, MAXPATH) == NULL) return -1;
        Slash(Name, 1);
        strcat(Name, Path);
        break;
    }
    return RemoveDots(Name, Expand);
#endif
}

int IsSameFile(const char *Path1, const char *Path2) {
    char p1[MAXPATH], p2[MAXPATH];

    if (ExpandPath(Path1, p1) == -1) return -1;
    if (ExpandPath(Path2, p2) == -1) return -1;
    if (filecmp(p1, p2) == 0) return 1;
    return 0;
}

int JustDirectory(const char *Path, char *Dir) {
    char *p;

    if (ExpandPath(Path, Dir) == -1)
        strcpy(Dir, Path);
    p = SepRChr(Dir);
    if (p) { p[1] = 0; }
    else Dir[0] = 0;
    return 0;
}

int JustLastDirectory(const char *Path, char *Dir) {
    int lastSlash = strlen(Path);
    while (lastSlash > 0 && !ISSEP(Path[lastSlash])) lastSlash--;

    int secondLastSlash = lastSlash;
    while (secondLastSlash > 0 && !ISSEP(Path[secondLastSlash - 1])) secondLastSlash--;

    strncpy(Dir, Path + secondLastSlash, lastSlash - secondLastSlash);
    Dir[lastSlash - secondLastSlash] = 0;

    return 0;
}

int JustFileName(const char *Path, char *Name) {
    int len = strlen(Path);

    while (len > 0 && !ISSEP(Path[len - 1])) len--;
    strcpy(Name, Path + len);
    return 0;
}

int JustRoot(const char *Path, char *Root) {
#if PATHTYPE == PT_UNIXISH
    strcpy(Root, SSLASH);
#else
    strncpy(Root, Path, 3);
    Root[3] = 0;
#endif
    return 0;
}

int FileExists(const char *Path) {
    return (access(Path, 0) == 0) ? 1 : 0;
}

int IsFullPath(const char *Path) {
    if (ISSLASH(Path[0])
#if PATHTYPE == PT_DOSISH
        || (Path[0] != 0 && Path[1] == ':')
#endif
       )
        return 1;
    else
        return 0;
}

const char *ShortFName(const char *Path, int len) {
    static char P[MAXPATH];
    char p1[MAXPATH];
    int l1;

    if (len < 10) len = 10;
    if (ExpandPath(Path, p1) == -1) return Path;
    l1 = strlen(p1);
    if (l1 < len) {
        strcpy(P, p1);
    } else {
        strncpy(P, p1, 3);
        strcpy(P + 3, "...");
        strncpy(P + 6, p1 + l1 - len, len - 6);
    }
    return P;
}


int ChangeDir(char *Dir) {
    Slash(Dir, 0); // remove last \ except in case of ?:
#if PATHTYPE == PT_DOSISH
    if (Dir[0] && Dir[1] == ':')
        if (SetDrive(toupper(Dir[0]) - 'A' + 1) == -1)
            return -1;
#endif
    if (chdir(Dir) == -1)
        return -1;
    return 0;
}

int JoinDirFile(char *Dest, const char *Dir, const char *Name) {
    strcpy(Dest, Dir);
    Slash(Dest, 1);
    strcat(Dest, Name);
    return 0;
}

char *SepRChr(char *Dir)
{
    char *p;
    if (Dir && Dir[0]) {
        for (p = Dir + strlen(Dir); p > Dir; p--)
            if (ISSEP(p[-1]))
                return p-1;
    }
    return NULL;
}
