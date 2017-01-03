# $Header: f:/SALT/emacs/RCS/Makefile,v 1.8 1992/02/15 13:16:38 SALT Exp SALT $
#----------------------------------------
#	Makefile: MicroEMACS 1.30 J1.43
#----------------------------------------

#----------------------------------------
#	設定
#----------------------------------------

.SUFFIXES: .hed .htp .inp .awk .ht2

AS	= as
ASFLAGS	= -w -u
LD	= lk
LDFLAGS =
CC	= gcc
OPT1	= -fomit-frame-pointer -fstrength-reduce
OPT2	= -fforce-mem -fcombine-regs
#DEFS	= -DNOEDBIND
CFLAGS	= -O -Wall -funsigned-char -D__NO_INLINE $(OPT1) $(OPT2) $(DEFS)
GPERF	= gperf
GPFLAGS	= -tpogaT
AWK	= awk
MALLOC	= nalloc

#----------------------------------------
#	生成規則
#----------------------------------------

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

.s.o:
	$(AS) $(ASFLAGS) $<

#----------------------------------------
#	ファイル指定
#----------------------------------------

EXEC	= em.x

#H1	= estruct.h edef.h etype.h iocs.h elang.h
#HD	= edir.h
#HE	= evar.h
#HH	= hprint.h
#HM	= ebind.h efunc.h
#HU	= menu.h hprint.h

LIBS	= symlinklib.a gnulib.a clib.a iocslib.a doslib.a kanjilib.a

HOBJS	= chash.o dirhash.o funchash.o

OBJS	= autoload.o basic.o bind.o buffer.o comhash.o complete.o compare.o	\
	  dired.o display.o displine.o dos.o envhash.o eval.o exec.o fepctrl.o	\
	  file.o fileio.o hentrap.o history.o hpr.o human68.o init.o input.o	\
	  intercept.o isearch.o langc.o latex.o line.o lineedit.o main.o	\
	  $(MALLOC).o menu.o mouse.o nthctype.o random.o region.o search.o	\
	  setargv.o system.o txrascopy.o window.o word.o			\
	  $(HOBJS) __main.o

#----------------------------------------
#	実行ファイル制作
#----------------------------------------

all:	$(EXEC) makex.x exe.x

indirect: Makefile
	@fecho $(LDFLAGS) -o $(EXEC) > indirect.new
	@fecho autoload.o basic.o bind.o buffer.o comhash.o	>> indirect.new
	@fecho complete.o compare.o dired.o display.o		>> indirect.new
	@fecho displine.o dos.o envhash.o eval.o exec.o		>> indirect.new
	@fecho fepctrl.o file.o fileio.o hentrap.o history.o	>> indirect.new
	@fecho hpr.o human68.o init.o input.o intercept.o	>> indirect.new
	@fecho isearch.o langc.o latex.o line.o lineedit.o	>> indirect.new
	@fecho main.o $(MALLOC).o menu.o mouse.o nthctype.o	>> indirect.new
	@fecho random.o region.o search.o setargv.o system.o	>> indirect.new
	@fecho txrascopy.o window.o word.o			>> indirect.new
	@fecho chash.o dirhash.o funchash.o			>> indirect.new
	@fecho __main.o -l $(LIBS)				>> indirect.new
	@mv indirect.new indirect

em.x:	$(OBJS) indirect
	$(LD) -i indirect

#----------------------------------------
#	オリジナルキーバインド作成
#----------------------------------------

bn2fn1.sed: bn2fn1.tbl convert.awk
	cat bn2fn1.tbl | awk -f convert.awk > bn2fn1.new
	mv bn2fn1.new bn2fn1.sed

bn2fn2.sed: bn2fn2.tbl convert.awk
	cat bn2fn2.tbl | awk -f convert.awk > bn2fn2.new
	mv bn2fn2.new bn2fn2.sed

ebind.h: ebind.def bn2fn1.sed bn2fn2.sed
	cat ebind.def | sed -f bn2fn1.sed | sed -f bn2fn2.sed > ebind.new
	mv ebind.new ebind.h

#----------------------------------------
#	Perfect-Hash関数作成
#----------------------------------------

$(HOBJS:%.o=%.c):%.c: %.hed %.htp %.awk
	$(AWK) -f $*.awk $*.htp > $*.ht2
	cat $*.hed $*.ht2 > $@

funchash.htp: funchash.inp
	$(GPERF) $(GPFLAGS) -k1,2,3,$$ -N func_in_word_set -K f_name $< > $@

dirhash.htp: dirhash.inp
	$(GPERF) $(GPFLAGS) -k1,2,$$ -N dirs_in_word_set -K d_name $< > $@

chash.htp: chash.inp
	$(GPERF) $(GPFLAGS) -k1,3,5,$$ -N c_in_word_set -K c_name $< > $@

#----------------------------------------
#	オブジェクト生成
#----------------------------------------

__main.o: __main.s
	$(CC) -c $<

autoload.o:	$(H1)
basic.o:	$(H1)
bind.o:		$(H1)
buffer.o:	$(H1)
comhash.o:	$(H1)
compare.o:	$(H1)
complete.o:	$(H1) $(HE)
dired.o:	$(H1)
dirhash.o:	$(H1)
display.o:	$(H1) $(HH)
displine.o:	$(H1) $(HH)
dos.o:		$(H1)
envhash.o:	$(H1)
eval.o:		$(H1) $(HE)
exec.o:		$(H1) $(HD)
fepctrl.o:	fepctrl.h
file.o:		$(H1)
fileio.o:	$(H1)
funchash.o:	$(H1)
hentarp.o:
history.o:	$(H1)
hpr.o:
human68.o:	$(H1)
init.o:		$(H1) ebind.h
input.o:	$(H1)
intercept.o:
isearch.o:	$(H1)
langc.o:	$(H1)
latex.o:	$(H1)
line.o:		$(H1)
lineedit.o:	$(H1)
main.o:		$(H1) $(HM) $(HE)
$(MALLOC).o:	$(H1)
menu.o:		$(H1) $(HU)
mouse.o:	$(H1)
nthctype.o:
random.o:	$(H1)
region.o:	$(H1)
search.o:	$(H1)
setargv.o:	$(H1)
system.o:
txrascpy.o:
window.o:	$(H1)
word.o:		$(H1)

#----------------------------------------
#	makex.x
#----------------------------------------

makex.x: makex.o
	$(CC) $<

makex.o: makex.c
	$(CC) -O -Wall -funsigned-char $(OPT1) $(OPT2) -c $<

#----------------------------------------
#	exe.x
#----------------------------------------

exe.x: exe.o
	$(CC) $<

exe.o: exe.c
	$(CC) -O -Wall -funsigned-char $(OPT1) $(OPT2) -c $<

#----------------------------------------
#	お掃除
#----------------------------------------

clean:
	-rm -f #* *.bak

#----------------------------------------
#	やり直しのためのお掃除
#----------------------------------------

moreclean:
	-rm -f #* *.bak *.htp
	-rm -f *.o *.ht2 *.sed ebind.h
	-rm -f funchash.c dirhash.c chash.c
	-rm -f indirect

