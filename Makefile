CC  = wcc386
CPP = wpp386
LD  = wlink

INCDIR    =
LIBDIR    =

#OPTIMIZE  = /d2
OPTIMIZE  = /d1 /onatx /oe=40

CCFLAGS   = /zp4 /5r /fp3 /j $(OPTIMIZE) $(INCDIR) /dDOSP32 /d__32BIT__ /dWATCOM /d__DOS4G__
LDFLAGS   = debug all $(LIBDIR)

OBJS = indent.obj &
       e_mark.obj &
       o_mode~1.obj &
       c_desk~1.obj &
       c_bind.obj &
       c_color.obj &
       c_config.obj &
       c_hist~1.obj &
       c_hilit.obj &
       c_mode.obj &
       e_block.obj &
       e_buffer.obj &
       e_cmds.obj &
       e_redraw.obj &
       e_file.obj &
       e_fold.obj &
       e_trans.obj &
       e_line.obj &
       e_load~1.obj &
       e_regex.obj &
       e_print.obj &
       e_search.obj &
       e_undo.obj &
       e_tags.obj &
       g_draw.obj &
       g_menu.obj &
       h_ada.obj &
       h_c.obj &
       h_fte.obj &
       h_html.obj &
       h_ipf.obj &
       h_make.obj &
       h_merge.obj &
       h_pascal.obj &
       h_perl.obj &
       h_plain.obj &
       h_msg.obj &
       h_rexx.obj &
       h_sh.obj &
       h_tex.obj &
       h_diff.obj &
       h_catbs.obj &
       h_simple.obj &
       i_comp~1.obj &
       i_ascii.obj &
       i_choice.obj &
       i_input.obj &
       i_key.obj &
       i_search.obj &
       i_view.obj &
       i_mode~1.obj &
       i_oview.obj &
       o_bufl~1.obj &
       o_list.obj &
       o_mess~1.obj &
       o_model.obj &
       o_rout~1.obj &
       o_buffer.obj &
       o_dire~1.obj &
       s_files.obj &
       s_direct.obj &
       s_util.obj &
       view.obj &
       gui.obj &
       egui.obj &
       fte.obj &
       commands.obj &
       log.obj

DOSP32OBJS = memicmp.obj &
             port.obj &
             portdos.obj &
             g_text.obj &
             menu_t~1.obj &
             con_dosx.obj &
             clip_no.obj &
             g_nodlg.obj &
             e_djgpp2.obj

CFTE_OBJS = cfte.obj &
            s_files.obj &
            port.obj &
            memicmp.obj

FTE_OBJS = $OBJS $DOSP32OBJS

.NOCHECK
build: fte.exe

.NOCHECK
run : fte.exe
	fte.exe

.NOCHECK
debug : fte.exe
	wd /swap /trap=rsi fte.exe

.cpp.obj: .AUTODEPEND
	$(CPP) $[. /zq $(CCFLAGS)

.c.obj: .AUTODEPEND
  $(CC) $[. /zq $(CCFLAGS)

clean: .SYMBOLIC
	del *.obj
	del bin2c.exe
	del cfte.exe
	del fte.exe
	del defcfg.cnf
	del defcfg.h
	del cfte.lnk
	del fte.lnk
	del bin2c.lnk

bin2c.exe: bin2c.cpp bin2c.lnk
	$(LD) $(LDFLAGS) @bin2c.lnk

cfte.exe: $(CFTE_OBJS) bin2c.exe cfte.lnk
	$(LD) $(LDFLAGS) @cfte.lnk

defcfg.cnf: defcfg.fte cfte.exe
	cfte defcfg.fte defcfg.cnf

defcfg.h: defcfg.cnf bin2c.exe
	bin2c defcfg.cnf >defcfg.h

c_config.obj: defcfg.h

fte.exe: $(FTE_OBJS) bin2c.exe cfte.exe fte.lnk
	$(LD) $(LDFLAGS) @fte.lnk

bin2c.lnk: bin2c.obj
  %create $^@
  %append $^@ NAME bin2c.exe
  %append $^@ SYSTEM DOS4G
  %append $^@ OPTION QUIET
  %append $^@ OPTION STACK=16k
  %append $^@ FILE bin2c.obj

cfte.lnk: $(CFTE_OBJS)
  %create $^@
  %append $^@ NAME cfte.exe
  %append $^@ SYSTEM DOS4G
  %append $^@ OPTION QUIET
  %append $^@ OPTION STACK=16k
  @for %i in ($(CFTE_OBJS)) do %append $^@ FILE %i

fte.lnk: $(FTE_OBJS)
  %create $^@
  %append $^@ NAME fte.exe
  %append $^@ SYSTEM DOS4G
  %append $^@ OPTION QUIET
  %append $^@ OPTION STACK=16k
  @for %i in ($(FTE_OBJS)) do %append $^@ FILE %i



