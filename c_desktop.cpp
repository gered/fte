/*    c_desktop.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_DESKTOP

#define DESKTOP_VER "FTE Desktop 2\n"
#define DESKTOP_VER1 "FTE Desktop 1\n"

char DesktopFileName[256] = "";

int SaveDesktop(char *FileName) {
    FILE *fp;
    EModel *M;
    
    fp = fopen(FileName, "w");
    if (fp == 0)
        return 0;
    
    setvbuf(fp, FileBuffer, _IOFBF, sizeof(FileBuffer));
    
    fprintf(fp, DESKTOP_VER);
    
    M = ActiveModel;
    while (M) {
        switch(M->GetContext()) {
        case CONTEXT_FILE:
            {
                EBuffer *B = (EBuffer *)M;
                fprintf(fp, "F|%d|%s\n", B->ModelNo, B->FileName);
            }
            break;
#ifdef CONFIG_OBJ_DIRECTORY
        case CONTEXT_DIRECTORY:
            {
                EDirectory *D = (EDirectory *)M;
                fprintf(fp, "D|%d|%s\n", D->ModelNo, D->Path);
            }
            break;
#endif
        }
        M = M->Next;
        if (M == ActiveModel)
            break;
    }
#ifdef CONFIG_TAGS
    TagsSave(fp);
#endif
    markIndex.saveToDesktop(fp);
    fclose(fp);
    return 1;
}

int LoadDesktop(char *FileName) {
    FILE *fp;
    char line[512];
    char *p, *e;
    int FLCount = 0;

#ifdef CONFIG_TAGS
    TagClear();
#endif
    
    fp = fopen(FileName, "r");
    if (fp == 0)
        return 0;

    //setvbuf(fp, FileBuffer, _IOFBF, sizeof(FileBuffer));
    
    if (fgets(line, sizeof(line), fp) == 0 ||
        (strcmp(line, DESKTOP_VER) != 0 &&
        (strcmp(line, DESKTOP_VER1) != 0)))
    {
        fclose(fp);
        return 0;
    }
    while (fgets(line, sizeof(line), fp) != 0) {
        e = strchr(line, '\n');
        if (e == 0)
            break;
        *e = 0;
        if ((line[0] == 'D' || line[0] == 'F') && line[1] == '|') {
            int ModelNo = -1;
            p = line + 2;
            if (isdigit(*p)) {
                ModelNo = atoi(p);
                while (isdigit(*p)) p++;
                if (*p == '|')
                    p++;
            }

            if (line[0] == 'F') { // file
                if (FLCount > 0)
                    suspendLoads = 1;
                if (FileLoad(0, p, 0, ActiveView))
                    FLCount++;
                suspendLoads  = 0;
#ifdef CONFIG_OBJ_DIRECTORY
            } else if (line[0] == 'D') { // directory
                EModel *m = new EDirectory(0, &ActiveModel, p);
                assert(ActiveModel != 0 && m != 0);
#endif
            }

            if (ActiveModel) {
                if (ModelNo != -1) {
                    if (FindModelID(ActiveModel, ModelNo) == 0)
                        ActiveModel->ModelNo = ModelNo;
                }

                if (ActiveModel != ActiveModel->Next) {
                    suspendLoads = 1;
                    ActiveView->SelectModel(ActiveModel->Next);
                    suspendLoads  = 0;
                }
            }
        } else {
#ifdef CONFIG_TAGS
            if (line[0] == 'T' && line[1] == '|') { // tag file
                TagsAdd(line + 2);
#endif
            } else if (line[0] == 'M' && line[1] == '|') { // mark
                char *name;
                char *file;
                EPoint P;
                //long l;
                char *e;

                p = line + 2;
                P.Row = strtol(p, &e, 10);
                if (*e != '|')
                    break;
                p = e + 1;
                P.Col = strtol(p, &e, 10);
                if (*e != '|')
                    break;
                p = e + 1;
                name = p;
                while (*p && *p != '|')
                    p++;
                if (*p == '|')
                    *p++ = 0;
                else
                    break;
                file = p;
                
                markIndex.insert(name, file, P);
            }
        }
    }
    fclose(fp);
    return 1;
}

#endif
