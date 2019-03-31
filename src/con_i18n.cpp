/*
 *    con_i18n.cpp
 *
 *    Copyright (c) 1998, Zdenek Kabelac
 *
 *    I18N support by kabi@fi.muni.cz
 *
 *    written as plain 'C' module and might be used
 *    in other programs to implement I18N support
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "con_i18n.h"

#define KEYMASK 0xff
#define KEYBYTEMAX 0xf00

#ifdef USE_HARD_REMAP
/*
 * This part is used when your Xserver doesn't work well with XKB extension
 */


/* Keyboard definition file - currently only Czech national keyboard
 * write your own keyboard for your language
 * And remember - this is supposed to be used only if your Xserver
 * is not supporting keyboard extension
 */
#include "con_ikcz.h"

/*
 * Quite a complex function to convert normal keys
 * to dead-keys and remapped keys
 */
static int I18NKeyAnalyze(XKeyEvent * keyEvent, KeySym * key, /*fold00*/
			  char *keyName, int nbytes)
{
    static long prev_state = 0, local_switch = 0,
    keypos = 0, last_keypos = 0, remap = 0;
    long i;
    struct keyboardRec *kbdActual;

    /* Find the state of keyboard
     * Check for different ISO group or modifier 5
     * So to activate remaping, you need to press at least
     * ScrollLock which is usually mapped as modifier 5
     */
    i = ((keyEvent->state | local_switch) & 0xFFA0);

    if (i != prev_state) {
	/* reset table position */
	last_keypos = keypos = 0;
	prev_state = i;
    }

    if (keyEvent->type == KeyPress) {
	if (i && ((*key == XK_Pause) || (*key == XK_F21))) {
	    remap = !remap;
	    return 0;
	} else if (*key == XK_F23) {
	    local_switch ^= (1UL<< 12);
	    return 0;
	}
    }
    /*
     * Check for already remapped ISO8859-X keys
     * this should not normaly happen :-)
     * as we need to use this hack
     */

    if ((*key > KEYMASK) && (*key < KEYBYTEMAX) || (*key > 0xffffff00)) {
	*key &= KEYMASK;
	keyName[0] = (char) *key;
	return 1;
    }
    /* Select keyboard map */
    if (!i)
	kbdActual = nationalKey[0];
    else if (!remap)
	kbdActual = nationalKey[1];
    else
	kbdActual = nationalKey[2];

    if (keyEvent->type == KeyPress) {
	long i = last_keypos;

	/*
	 * Search for DeadKey -> do loop over all tables.
	 *
	 * Note: We can define ONE DeadKey and use
	 * it for several tables sequentially
	 */
	for (;;) {
	    i++;
	    if ((kbdActual[0].tab == NULL) || kbdActual[i].tab == NULL) {
		i = 0;
		if (kbdActual[i].tab == NULL) {
		    /* Looks like empty table -> IGNORE */
		    keypos = i;
		    return nbytes;
		}
	    }
	    if (i == last_keypos)
		break;

	    if (kbdActual[i].deadkey == *key) {
		keypos = kbdActual[i].next;
		/* Found DeadKey -> delete it
		 * and mark actual position for next search */
		last_keypos = i;
		keyName[0] = *key = 0;
		return 0;
	    }
	}
    } else if (keypos)
	return 0;		/* ignore key release */

    /* Now we know it is not a DeadKey and we
     * are in selected remaping keyboard table */

    /* printf("** key:%5d\tstatus:0x%x\n", *key, prev_state); */
    if (*key < KEYBYTEMAX) {
	/*
	 * this is selected constant and will change when
	 * this editor will be able to write in japan :-)
	 */
	int i = 0;

	/* remaping only real keys */
	while (kbdActual[keypos].tab[i].key_english != 0) {
	    if (*key == kbdActual[keypos].tab[i].key_english) {
		*key = kbdActual[keypos].tab[i].key_remap;
		break;
	    }
	    i++;
	}
	last_keypos = keypos = kbdActual[keypos].next;
	/* printf("** remap: %3d %3d\n", keypos, *key); */
	keyName[0] = *key && KEYMASK;
	return 1;
    }
    return 0;
}
 /*FOLD00*/
#else

/*********************************************
 *                                           *
 *   Standart methods for reading Keyboard   *
 *                                           *
 *********************************************/

/* ISO-8859-2 key-change

 * All these functions are for keyboard reading.
 * Correct displaing of this text is another thing,
 * but as I only need ISO-8859 encoding support,
 * I don't care about this (for now).
 */
static int I18NKeyAnalyze(XKeyEvent * keyEvent, KeySym * key, /*fold00*/
			  char *keyName, int nbytes)
{
    KeySym t = (unsigned char) keyName[0];

    /*
     * ISO-8859-2 is using some characters from 8859-1 and
     * rest of them is located between 0x100 - 0x200 in 'X' so
     * with ISO-8859-2 font we'll remap them down bellow < 0x100
     * This is mostly true for all Latin-X alphas, just
     * special font to display them correctly is needed.
     * This jobs does Xserver - and keysymbol is returned
     * in the 1st. byte of keyName string.
     */
    if ((nbytes == 1) && *key < KEYBYTEMAX)
	*key = t;
#ifdef USE_HACK_FOR_BAD_XSERVER
    /*
     * this is really hack - but some Xservers are somewhat
     * strange, so we remap character manually
     */
    else if (!nbytes && (*key > KEYMASK) && (*key < KEYBYTEMAX)) {
	*key &= KEYMASK;
	keyName[0] = *key & KEYMASK;
	nbytes = 1;
    }
#endif
    return nbytes;
}
 /*FOLD00*/
#endif

/*
 * Initialize I18N functions - took me hours to
 * figure out how this works even though it was
 * cut & pasted from 'xterm' sources, but as 'xterm'
 * is using Xt Toolkit some things have to be made
 * different
 */
XIC I18NInit(Display * display, Window win, unsigned long *mask) /*fold00*/
{
    XIC xic = (XIC) NULL;
#if XlibSpecificationRelease >= 6
    XIM xim = (XIM) NULL;
    XIMStyles *xim_styles;
    XIMStyle input_style = 0;
    char *s, tmp[256];
    int found = False;

    *mask = 0;

    /* Locale setting taken from XtSetLanguageProc */
    if (!(s = setlocale(LC_ALL, "")))
	fprintf(stderr, "I18N warning: Locale not supported by C library, "
		"locale unchanged!\n");

    if (!XSupportsLocale()) {
	fprintf(stderr, "I18N warning: Locale not supported by Xlib, "
		"locale set to C!\n");
	s = setlocale(LC_ALL, "C");
    }
    if (!XSetLocaleModifiers(""))
	fprintf(stderr, "I18N warning: X locale modifiers not supported, "
		"using default\n");

    xim = XOpenIM(display, NULL, NULL, NULL);
    if (xim == NULL) {
        // there are languages without Input Methods ????
	fprintf(stderr, "I18N warning: Input method not specified\n");
	return NULL;
    }

    if (XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL)
	|| xim_styles == NULL) {
	fprintf(stderr, "I18N error: Input method doesn't support any style\n");
	XCloseIM(xim);
	return NULL;
    }

    /*
     * This is some kind of debugging message to inform user
     * that something is wrong with his system
     */
    if (s != NULL && (strstr(s, XLocaleOfIM(xim)) == NULL))
	fprintf(stderr, "I18N warning: System locale: \"%s\" differs from "
		"IM locale: \"%s\"...\n", s, XLocaleOfIM(xim));

    /*
     * This part is cut&paste from other sources
     * There is no reason to do it this way, because
     * the only input style currently supported is Root
     * but for the future extension I'll leave it here
     */

    strcpy(tmp, XIM_INPUT_STYLE);
    for (s = tmp; s && !found;) {
	char *ns, *end;
	int i;

	while ((*s != 0) && isspace(*s))
	    s++;

	if (*s == 0)
	    break;

	if ((ns = end = strchr(s, ',')) != 0)
	    ns++;
	else
	    end = s + strlen(s);

	while (isspace(*end))
	    end--;
	*end = '\0';

	if (!strcmp(s, "OverTheSpot"))
	    input_style = (XIMPreeditPosition | XIMStatusArea);
	else if (!strcmp(s, "OffTheSpot"))
	    input_style = (XIMPreeditArea | XIMStatusArea);
	else if (!strcmp(s, "Root"))
	    input_style = (XIMPreeditNothing | XIMStatusNothing);

	for (i = 0; (unsigned short) i < xim_styles->count_styles; i++)
	    if (input_style == xim_styles->supported_styles[i]) {
		found = True;
		break;
	    }
	s = ns;
    }
    XFree(xim_styles);

    if (!found) {
	fprintf(stderr, "I18N error: Input method doesn't support my "
		"preedit type\n");
	XCloseIM(xim);
	return NULL;
    }
    /* This program only understand the Root preedit_style yet */
    if (input_style != (XIMPreeditNothing | XIMStatusNothing)) {
	fprintf(stderr, "I18N error: This program only supports the "
		"'Root' preedit type\n");
	XCloseIM(xim);
	return NULL;
    }
    xic = XCreateIC(xim, XNInputStyle, input_style,
		    XNClientWindow, win,
		    XNFocusWindow, win,
		    NULL);
    if (xic == NULL) {
	fprintf(stderr, "I18N error: Failed to create input context\n");
	XCloseIM(xim);
    } else if (XGetICValues(xic, XNFilterEvents, mask, NULL))
	fprintf(stderr, "I18N error: Can't get Event Mask\n");
#else
    *mask = 0;
#endif
    return xic;
}
 /*FOLD00*/

void I18NFocusIn(XIC xic) /*fold00*/
{
#if XlibSpecificationRelease >= 6
    if (xic != NULL)
	XSetICFocus(xic);
#endif
}
 /*FOLD00*/

void I18NFocusOut(XIC xic) /*fold00*/
{
#if XlibSpecificationRelease >= 6
    if (xic != NULL)
	XUnsetICFocus(xic);
#endif
}
 /*FOLD00*/

/*
 * Lookup correct keysymbol from keymap event
 */
int I18NLookupString(XKeyEvent * keyEvent, char *keyName, int keySize, /*FOLD00*/
		     KeySym * key, XIC xic)
{
    static int showKeys = 0;
    int nbytes = 0;

#if XlibSpecificationRelease >= 6
    if (xic != NULL) {
	if (keyEvent->type == KeyPress) {
            Status status_return;

            /* No KeyRelease events here ! */
#if 1
	    nbytes = XmbLookupString(xic, keyEvent, keyName, keySize,
                                     key, &status_return);
#else
            wchar_t wk;
            nbytes = XwcLookupString(xic, keyEvent, &wk, 1, key, &status_return);
            printf("code=%0X\n", wk);
            keySize = 1;
            keyName[0] = wk & 0xFF;
#endif
	}
    } else
#endif
    do {
	static XComposeStatus compose_status = { NULL, 0 };
	nbytes = XLookupString(keyEvent, keyName, keySize,
			       key, &compose_status);
    } while (0);


    if (showKeys) {
	fprintf(stderr, "Key: 0x%04lx  '%s'\tKeyEventState:0x%x\t",
		*key, XKeysymToString(*key), keyEvent->state);
	if (nbytes && isprint(keyName[0])) {
	    keyName[nbytes] = 0;
	    fprintf(stderr, "String:'%s' Size:%2d  ", keyName, nbytes);
	}
	fputs("\n", stderr);
    }
    if (((*key == XK_F1) || (*key == XK_F11))
	&& ((keyEvent->state & (ShiftMask | ControlMask))
	    == (ShiftMask | ControlMask))
	&& (keyEvent->type == KeyPress))
	showKeys = !showKeys;

    return I18NKeyAnalyze(keyEvent, key, keyName, nbytes);
}
 /*FOLD00*/
