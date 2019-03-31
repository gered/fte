/*    clip_x11.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

int GetXSelection(int *len, char **data);
int SetXSelection(int len, char *data);

int GetPMClip() {
    char *data;
    int len;
    int i,j, l, dx;
    EPoint P;

    if (GetXSelection(&len, &data) == 0) {
        SSBuffer->Clear();
        j = 0;
        l = 0;

        for (i = 0; i < len; i++) {
            if (data[i] == '\n') {
                SSBuffer->AssertLine(l);
                P.Col = 0; P.Row = l++;
                dx = 0;
                if ((i > 0) && (data[i-1] == '\r')) dx++;
                SSBuffer->InsertLine(P, i - j - dx, data + j);
                j = i + 1;
            }
        }
        if (j < len) { // remainder
            i = len;
            SSBuffer->AssertLine(l);
            P.Col = 0; P.Row = l++;
            dx = 0;
            if ((i > 0) && (data[i-1] == '\r')) dx++;
            SSBuffer->InsText(P.Row, P.Col, i - j - dx, data + j);
            j = i + 1;
        }
        free(data);
        return 1;
    }
    return 0;
}

int PutPMClip() {
    PELine L;
    char *p = NULL;
    int rc = 0;
    int l = 0;

    for (int i = 0; i < SSBuffer->RCount; i++) {
        L = SSBuffer->RLine(i);
        char *n = (char *)realloc(p, l + L->Count + 1);
        if (n != NULL) {
            for(int j = 0; j < L->Count; j++) {
                if ((j < (L->Count - 1)) && (L->Chars[j + 1] == '\b'))
                    j++;
                else
                    n[l++] = L->Chars[j];
            }
            if (i < SSBuffer->RCount - 1)
                n[l++] = '\n';
            else
                n[l] = 0;
        } else
            break;
        p = n;   // if p already contains some address it will be freed
    }

    if (p != NULL) {
        // remove some 'UNWANTED' characters - sequence XX 0x08 YY -> YY
        // this makes usable cut&paste from manpages
        rc = (SetXSelection(l, p) == 0);
        free(p);
    }
    return (rc)?1:0;
}
