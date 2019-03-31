/*    fte.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef __FTE_H
#define __FTE_H

#include "ftever.h"
#include "feature.h"
#include "sysdep.h"

#ifdef NEED_LOG_H
#include "log.h"
#endif // NEED_LOG_H
#include "console.h"
#include "gui.h"
#include "c_desk~1.h"
#include "e_regex.h"
#include "c_mode.h"
#include "c_hilit.h"
#include "c_bind.h"
#include "e_undo.h"
#include "c_color.h"
#include "s_files.h"
#include "s_direct.h"
#include "c_config.h"
#include "c_fcon~1.h"
#include "i_oview.h"
#include "i_input.h"
#include "i_key.h"
#include "i_choice.h"
#include "i_ascii.h"
#include "i_view.h"
#include "i_mode~1.h"
#include "egui.h"
#include "o_model.h"
#include "e_buffer.h"
#include "e_mark.h"
#include "o_list.h"
#include "e_tags.h"
#include "s_util.h"
#include "i_search.h"
#include "i_comp~1.h"
#include "o_dire~1.h"
#include "o_mess~1.h"
#include "o_bufl~1.h"
#include "o_rout~1.h"
#include "o_mode~1.h"
#include "c_comm~1.h"
#include "c_hist~1.h"

#if defined(_DEBUG) && defined(MSVC) && defined(MSVCDEBUG)
#include <crtdbg.h>

#define new new( _CLIENT_BLOCK, __FILE__, __LINE__)

#endif //_DEBUG && MSVC && MSVCDEBUG

#endif
