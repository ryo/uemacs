#CC=gcc
CC=cc
CFLAGS=	-O 
#CFLAGS=	-O 

OFILES=		amaranth.o \
		basic.o bind.o buffer.o char.o crypt.o display.o eval.o \
		exec.o file.o fileio.o input.o isearch.o line.o lock.o main.o \
		mouse.o random.o region.o search.o  newunix.o window.o word.o
#		mouse.o random.o region.o search.o tcap.o newunix.o window.o word.o

CFILES=		amaranth.c \
		basic.c bind.c buffer.c char.c crypt.c display.c eval.c \
		exec.c file.c fileio.c input.c isearch.c line.c lock.c main.c \
		mouse.c random.c region.c search.c  newunix.c window.c word.c
#		mouse.c random.c region.c search.c tcap.c newunix.c window.c word.c

HFILES=		estruct.h edef.h efunc.h epath.h ebind.h evar.h elang.h

emacs:		$(OFILES)
		$(CC) $(CFLAGS) $(OFILES) -ltermcap -lc -o emacsnew
		strip emacsnew

$(OFILES):	$(HFILES)
