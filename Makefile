# Uncomment & use these 3 lines to use Genbank in addition to FASTN:
GBHOME=		/home/macke/gensearch
GBLIB=		$(GBHOME)/libgb.a
#CFLAGS=		-O2 -DUSE_GENBANK $(LOCAL_CFLAGS)
CFLAGS=		-g -DUSE_GENBANK $(LOCAL_CFLAGS)

# Uncomment & use this line for FASTN only
#CFLAGS=	-O2 $(LOCAL_CFLAGS)

YACC=	yacc
LEX=	lex

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
		mk_dumpnode.h y.tab.h > dumpnode.h

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
