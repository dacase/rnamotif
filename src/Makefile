include ../config.h

RNAMOTOBJS=	\
	rnamot.o	\
	compile.o	\
	dump.o		\
	errormsg.o	\
	fastn.o		\
	find_motif.o	\
	mm_regexp.o	\
	node.o		\
	regexp.o	\
	score.o		\
	y.tab.o	

all:	rnamot	\
	rmfmt

rnamot:	$(RNAMOTOBJS)
	$(CC)	$(CFLAGS) -o rnamot $(RNAMOTOBJS) $(GBLIB)

rnamot.o:	rnamot.h dbase.h y.tab.h

compile.o:	rnamot.h y.tab.h

dump.o:		rnamot.h y.tab.h

dumpnode.h:	y.tab.h mk_dumpnode.h
		./mk_dumpnode.h y.tab.h > dumpnode.h

find_motif.o:	rnamot.h y.tab.h

node.o:		dumpnode.h rnamot.h y.tab.h

y.tab.o:	rnamot.h y.tab.c

y.tab.c:	y.tab.h

y.tab.h:	rmgrm.y lex.yy.c
		$(YACC) -d -v -t rmgrm.y

lex.yy.c:	rmlex.l
		$(LEX) rmlex.l

rmfmt:		rmfmt.o split.o
	$(CC)	$(CFLAGS) -o rmfmt rmfmt.o split.o

clean:
	-rm -f 	 *.o		\
		dumpnode.h	\
		y.tab.c		\
		y.tab.h		\
		y.output	\
		lex.yy.c	\
		rnamot		\
		rmfmt		\
		core		\
		junk*
