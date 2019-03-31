#ifndef  __CON_IKCS_H
#define  __CON_IKCS_H

static struct remapKey keyboardStd[] =
{
    {0,}
};

static struct remapKey keyboardHalfCz[] =
{
    {XK_2, 0xff & '�'},
    {XK_3, 0xff & '�'},
    {XK_4, 0xff & '�'},
    {XK_5, 0xff & '�'},
    {XK_6, 0xff & '�'},
    {XK_7, 0xff & '�'},
    {XK_8, 0xff & '�'},
    {XK_9, 0xff & '�'},
    {XK_0, 0xff & '�'},
    {0,}
};

static struct remapKey keyboardFullCz[] =
{
    {XK_1, 0xff & '+'},
    {XK_exclam, XK_1},
    {XK_2, 0xff & '�'},
    {XK_at, XK_2},
    {XK_3, 0xff & '�'},
    {XK_numbersign, XK_3},
    {XK_4, 0xff & '�'},
    {XK_dollar, XK_4},
    {XK_5, 0xff & '�'},
    {XK_percent, XK_5},
    {XK_6, 0xff & '�'},
    {XK_asciicircum, XK_6},
    {XK_7, 0xff & '�'},
    {XK_ampersand, XK_7},
    {XK_8, 0xff & '�'},
    {XK_asterisk, XK_8},
    {XK_9, 0xff & '�'},
    {XK_parenleft, XK_9},
    {XK_0, 0xff & '�'},
    {XK_parenright, XK_0},

    {XK_minus, XK_equal},
    {XK_underscore, XK_percent},

    {XK_bracketleft, 0xff & '�'},
    {XK_braceleft, 0xff & '/'},
    {XK_bracketright, 0xff & ')'},
    {XK_braceright, 0xff & '('},

    {XK_semicolon, 0xff & '�'},
    {XK_apostrophe, 0xff & '�'},
    {XK_colon, 0xff & '"'},
    {XK_quotedbl, 0xff & '!'},

    {XK_comma, 0xff & ','},
    {XK_less, 0xff & '?'},
    {XK_period, 0xff & '.'},
    {XK_greater, 0xff & ':'},
    {XK_question, 0xff & '_'},
    {XK_slash, 0xff & '-'},

    {0,}
};

static struct remapKey keyboardAcute[] =
{
    {XK_w, 0xff & '�'},
    {XK_W, 0xff & '�'},
    {XK_e, 0xff & '�'},
    {XK_E, 0xff & '�'},
    {XK_r, 0xff & '�'},
    {XK_R, 0xff & '�'},
    {XK_t, 0xff & '�'},
    {XK_T, 0xff & '�'},
    {XK_y, 0xff & '�'},
    {XK_Y, 0xff & '�'},
    {XK_u, 0xff & '�'},
    {XK_U, 0xff & '�'},
    {XK_i, 0xff & '�'},
    {XK_I, 0xff & '�'},
    {XK_o, 0xff & '�'},
    {XK_O, 0xff & '�'},
    {XK_p, 0xff & '�'},
    {XK_P, 0xff & '�'},

    {XK_a, 0xff & '�'},
    {XK_A, 0xff & '�'},
    {XK_s, 0xff & '�'},
    {XK_S, 0xff & '�'},
    {XK_d, 0xff & '�'},
    {XK_D, 0xff & '�'},
    {XK_h, 0xff & '�'},
    {XK_H, 0xff & '�'},
    {XK_l, 0xff & '�'},
    {XK_L, 0xff & '�'},

    {XK_z, 0xff & '�'},
    {XK_Z, 0xff & '�'},
    {XK_x, 0xff & '�'},
    {XK_X, 0xff & '�'},
    {XK_c, 0xff & '�'},
    {XK_C, 0xff & '�'},
    {XK_b, 0xff & '�'},
    {XK_B, 0xff & '�'},
    {XK_n, 0xff & '�'},
    {XK_N, 0xff & '�'},

    {XK_equal, 0xff & '�'},

    {0,}
};

static struct remapKey keyboardCaron[] =
{
    {XK_e, 0xff & '�'},
    {XK_E, 0xff & '�'},
    {XK_r, 0xff & '�'},
    {XK_R, 0xff & '�'},
    {XK_t, 0xff & '�'},
    {XK_T, 0xff & '�'},
    {XK_u, 0xff & '�'},
    {XK_U, 0xff & '�'},
    {XK_i, 0xff & '�'},
    {XK_I, 0xff & '�'},
    {XK_o, 0xff & '�'},
    {XK_O, 0xff & '�'},

    {XK_a, 0xff & '�'},
    {XK_A, 0xff & '�'},
    {XK_s, 0xff & '�'},
    {XK_S, 0xff & '�'},
    {XK_d, 0xff & '�'},
    {XK_D, 0xff & '�'},
    {XK_l, 0xff & '�'},
    {XK_L, 0xff & '�'},

    {XK_z, 0xff & '�'},
    {XK_Z, 0xff & '�'},
    {XK_x, 0xff & '�'},
    {XK_X, 0xff & '�'},
    {XK_c, 0xff & '�'},
    {XK_C, 0xff & '�'},
    {XK_n, 0xff & '�'},
    {XK_N, 0xff & '�'},

    {XK_plus, 0xff & '�'},
    {0,}
};

static struct remapKey keyboardFirst[] =
{
    {XK_w, 0xff & '�'},
    {XK_W, 0xff & '�'},
    {XK_e, 0xff & '�'},
    {XK_E, 0xff & '�'},
    {XK_r, 0xff & '�'},
    {XK_R, 0xff & '�'},
    {XK_t, 0xff & '�'},
    {XK_T, 0xff & '�'},
    {XK_y, 0xff & '�'},
    {XK_Y, 0xff & '�'},
    {XK_u, 0xff & '�'},
    {XK_U, 0xff & '�'},
    {XK_i, 0xff & '�'},
    {XK_I, 0xff & '�'},
    {XK_o, 0xff & '�'},
    {XK_O, 0xff & '�'},
    {XK_p, 0xff & '�'},
    {XK_P, 0xff & '�'},

    {XK_a, 0xff & '�'},
    {XK_A, 0xff & '�'},
    {XK_s, 0xff & '�'},
    {XK_S, 0xff & '�'},
    {XK_d, 0xff & '�'},
    {XK_D, 0xff & '�'},
    {XK_h, 0xff & '�'},
    {XK_H, 0xff & '�'},
    {XK_l, 0xff & '�'},
    {XK_L, 0xff & '�'},

    {XK_z, 0xff & '�'},
    {XK_Z, 0xff & '�'},
    {XK_x, 0xff & '�'},
    {XK_X, 0xff & '�'},
    {XK_c, 0xff & '�'},
    {XK_C, 0xff & '�'},
    {XK_b, 0xff & '�'},
    {XK_B, 0xff & '�'},
    {XK_n, 0xff & '�'},
    {XK_N, 0xff & '�'},

    {0,}
};

static struct remapKey keyboardSecond[] =
{
    {XK_equal, 0xff & '�'},
    {XK_plus, 0xff & '�'},

    {XK_e, 0xff & '�'},
    {XK_E, 0xff & '�'},
    {XK_r, 0xff & '�'},
    {XK_R, 0xff & '�'},
    {XK_t, 0xff & '�'},
    {XK_T, 0xff & '�'},
    {XK_u, 0xff & '�'},
    {XK_U, 0xff & '�'},
    {XK_i, 0xff & '�'},
    {XK_I, 0xff & '�'},
    {XK_o, 0xff & '�'},
    {XK_O, 0xff & '�'},

    {XK_a, 0xff & '�'},
    {XK_A, 0xff & '�'},
    {XK_s, 0xff & '�'},
    {XK_S, 0xff & '�'},
    {XK_d, 0xff & '�'},
    {XK_D, 0xff & '�'},
    {XK_l, 0xff & '�'},
    {XK_L, 0xff & '�'},

    {XK_z, 0xff & '�'},
    {XK_Z, 0xff & '�'},
    {XK_x, 0xff & '�'},
    {XK_X, 0xff & '�'},
    {XK_c, 0xff & '�'},
    {XK_C, 0xff & '�'},
    {XK_n, 0xff & '�'},
    {XK_N, 0xff & '�'},

    {0,}
};



static struct remapKey keyboardThird[] =
{
    {XK_a, 0xff & '�'},
    {XK_A, 0xff & '�'},
    {XK_e, 0xff & '�'},
    {XK_E, 0xff & '�'},
    {XK_u, 0xff & '�'},
    {XK_U, 0xff & '�'},
    {XK_o, 0xff & '�'},
    {XK_O, 0xff & '�'},

    {XK_s, 0xff & '�'},
    {XK_S, 0xff & '�'},
    {XK_l, 0xff & '�'},
    {XK_L, 0xff & '�'},

    {XK_z, 0xff & '�'},
    {XK_Z, 0xff & '�'},
    {XK_c, 0xff & '�'},
    {XK_C, 0xff & '�'},

    {0,}
};

static struct remapKey keyboardFourth[] =
{
    {XK_a, 0xff & '�'},
    {XK_A, 0xff & '�'},
    {XK_e, 0xff & '�'},
    {XK_E, 0xff & '�'},
    {XK_u, 0xff & '�'},
    {XK_U, 0xff & '�'},
    {XK_o, 0xff & '�'},
    {XK_O, 0xff & '�'},

    {XK_l, 0xff & '�'},
    {XK_L, 0xff & '�'},

    {0,}
};

static struct remapKey keyboardFifth[] =
{
    {XK_a, 0xff & '�'},
    {XK_A, 0xff & '�'},
    {XK_o, 0xff & '�'},
    {XK_O, 0xff & '�'},

    {XK_l, 0xff & '|'},
    {XK_L, 0xff & '|'},

    {0,}
};

#define KEYMAPS_MACRO \
    {keyboardAcute, 0, 0},      /*  1 */ \
    {keyboardCaron, 0, 0},      /*  2 */ \
    {keyboardFirst, 0, 0},      /*  3 */ \
    {keyboardSecond, 0, 0},     /*  4 */ \
    {keyboardThird, 0, 0},      /*  5 */ \
    {keyboardFourth, 0, 0},     /*  6 */ \
    {keyboardFifth, 0, 0},      /*  7 */

#ifdef XK_dead_acute
#define XKB_DEAD_KEYS \
    {keyboardStd, XK_dead_acute, 1}, \
    {keyboardStd, XK_dead_caron, 2}, \
    {keyboardStd, XK_dead_iota, 3}, \
    {keyboardStd, XK_dead_iota, 4}, \
    {keyboardStd, XK_dead_iota, 5}, \
    {keyboardStd, XK_dead_iota, 6}, \
    {keyboardStd, XK_dead_iota, 7}, \
    {keyboardStd, XK_dead_iota, 0},

#else
#define XKB_DEAD_KEYS
#endif

#ifdef XK_F22
#define F22_DEAD_KEYS \
    {keyboardStd, XK_F22, 3}, \
    {keyboardStd, XK_F22, 4}, \
    {keyboardStd, XK_F22, 5}, \
    {keyboardStd, XK_F22, 6}, \
    {keyboardStd, XK_F22, 7}, \
    {keyboardStd, XK_F22, 0},
#else
#define F22_DEAD_KEYS
#endif

#define KBD_MACRO \
    {keyboardStd, XK_Print, 3}, \
    {keyboardStd, XK_Print, 4}, \
    {keyboardStd, XK_Print, 5}, \
    {keyboardStd, XK_Print, 6}, \
    {keyboardStd, XK_Print, 7}, \
    {keyboardStd, XK_Print, 0}, \
    XKB_DEAD_KEYS \
    F22_DEAD_KEYS


static struct keyboardRec kbdStdRec[] =
{
    {keyboardStd, 0, 0},        /*  0 */

    KEYMAPS_MACRO

    KBD_MACRO

    {NULL,}
};

static struct keyboardRec kbdHalfCzRec[] =
{
    {keyboardHalfCz, 0, 0},     /*  0 */

    KEYMAPS_MACRO

    KBD_MACRO

    {NULL,}
};

static struct keyboardRec kbdFullCzRec[] =
{
    {keyboardFullCz, 0, 0},     /*  0 */

    KEYMAPS_MACRO

    {keyboardStd, XK_equal, 1},
    {keyboardStd, XK_plus, 2},

    KBD_MACRO

    {NULL,}
};

/*
 * one standart keyboard and two national keyboards
 * (for programmers and for writers)
 */
static struct keyboardRec *nationalKey[] =
{
    kbdStdRec,
    kbdHalfCzRec,
    kbdFullCzRec,
    0
};

#endif
