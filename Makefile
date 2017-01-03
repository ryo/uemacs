
CC?=		cc

CFLAGS=		-pipe
CFLAGS+=	-Wall \
		-Wstrict-prototypes \
		-Wmissing-prototypes \
		-Wpointer-arith \
		-Wno-uninitialized \
		-Wreturn-type \
		-Wpointer-arith \
		-Wswitch \
		-Wshadow \
#		-Wwrite-strings \
#		-Wcast-qual \
#		-Werror

#PROFILE=	-pg

#CFLAGS+=	-O2 -funsigned-char -fomit-frame-pointer -fstrength-reduce
#CFLAGS+=	$(PROFILE)
CFLAGS+=	-g
CFLAGS+=	-I/usr/local/include -DICONV
FLAGS+= 	-DICONV
LIBS+=		-L/usr/local/lib
#LIBS+=		-liconv
#LIBS+=		-lpcreposix
LIBS+=		-ltermcap


PROGRAM=	uemacs

OBJS=		basic.o bind.o buffer.o char.o crypt.o display.o eval.o \
		exec.o file.o fileio.o input.o isearch.o line.o main.o \
		random.o region.o search.o tcap.o unix.o window.o word.o \
		kanji.o keyword.o

DSTDIR= /usr/local/bin



.c.o:
	$(CC) -c $(CFLAGS) $(PROFILE) $<


$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) $(PROFILE) $(OBJS) -o $(PROGRAM) $(LIBS)

install:
	install -c -o root -g bin -m 755 uemacs $(DSTDIR)

clean:
	-rm -fr $(OBJS) $(PROGRAM)


unix.o: estruct.h etype.h edef.h elang.h
region.o: estruct.h etype.h edef.h elang.h
main.o: estruct.h etype.h efunc.h edef.h elang.h ebind.h keyword.h
keyword.o: estruct.h etype.h edef.h elang.h keyword.h
exec.o: estruct.h etype.h edef.h elang.h
tcap.o: estruct.h etype.h edef.h elang.h
line.o: estruct.h etype.h edef.h elang.h kanji.h
eval.o: estruct.h etype.h edef.h elang.h evar.h
kanji.o: kanji.h
input.o: estruct.h etype.h edef.h elang.h
crypt.o: estruct.h etype.h edef.h elang.h
search.o: estruct.h etype.h edef.h elang.h
display.o: estruct.h etype.h edef.h elang.h keyword.h
window.o: estruct.h etype.h edef.h elang.h
fileio.o: estruct.h etype.h edef.h elang.h
buffer.o: estruct.h etype.h edef.h elang.h keyword.h
word.o: estruct.h etype.h edef.h elang.h kanji.h
bind.o: estruct.h etype.h edef.h elang.h epath.h
char.o: estruct.h etype.h edef.h elang.h
isearch.o: estruct.h etype.h edef.h elang.h
file.o: estruct.h etype.h edef.h elang.h
random.o: estruct.h etype.h edef.h elang.h kanji.h
basic.o: estruct.h etype.h edef.h elang.h kanji.h
