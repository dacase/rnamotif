include ../config.h

RNAMOTOBJS=	\
	rnamot.o	\
	compile.o	\
	dbutil.o	\
	dump.o		\
	efn.o		\
	efn2.o		\
	errormsg.o	\
	find_motif.o	\
	getargs.o	\
	mm_regexp.o	\
	node.o		\
	preprocessor.o	\
	regexp.o	\
	score.o		\
	split.o		\
	y.tab.o	

all:	rnamotif	\
	rmprune		\
	rm2ct		\
	rmfmt		\
	efn_drv		\
	efn2_drv

rnamotif:	$(RNAMOTOBJS)
	$(CC)	$(CFLAGS) -o rnamotif $(RNAMOTOBJS) $(GBLIB) -lm

rnamot.o:	rmdefs.h rnamot.h dbutil.h y.tab.h

compile.o:	rmdefs.h rnamot.h mm_regexp.h y.tab.h

dbutil.o:	rmdefs.h dbutil.h

dump.o:		rmdefs.h rnamot.h y.tab.h

dumpnode.h:	y.tab.h mk_dumpnode.h
		./mk_dumpnode.h y.tab.h > dumpnode.h

efn.o:		rmdefs.h rnamot.h

efn2.o:		rmdefs.h rnamot.h

errormsg.o:	rmdefs.h

find_motif.o:	rmdefs.h rnamot.h y.tab.h

getargs.o:	rmdefs.h rnamot.h

mm_regexp.o:	rmdefs.h rnamot.h mm_regexp.h

node.o:		dumpnode.h rnamot.h y.tab.h

preprocessor.o:	rnamot.h

score.o:	rmdefs.h rnamot.h

y.tab.o:	rnamot.h y.tab.c

y.tab.c:	y.tab.h

y.tab.h:	rmgrm.y lex.yy.c
		$(YACC) -d -v -t rmgrm.y

lex.yy.c:	rmlex.l
		$(LEX) rmlex.l

rmprune:	rmprune.o split.o
	$(CC)	$(CFLAGS) -o rmprune rmprune.o split.o

rmprune.o:	rmdefs.h

rmfmt:		rmfmt.o split.o
	$(CC)	$(CFLAGS) -o rmfmt rmfmt.o split.o

rm2ct:		rm2ct.o split.o
	$(CC)	$(CFLAGS) -o rm2ct rm2ct.o split.o

efn_drv:	efn_drv.o efn.o errormsg.o split.o
	$(CC)	$(CFLAGS) -o efn_drv efn_drv.o efn.o errormsg.o split.o -lm

efn_drv.o:	rmdefs.h rnamot.h

efn2_drv:	efn2_drv.o efn2.o errormsg.o split.o
	$(CC)	$(CFLAGS) -o efn2_drv efn2_drv.o efn2.o errormsg.o split.o -lm

efn2_drv.o:	rmdefs.h rnamot.h

clean:
	-rm -f 	 *.o		\
		dumpnode.h	\
		y.tab.c		\
		y.tab.h		\
		y.output	\
		lex.yy.c	\
		rnamotif	\
		rmprune		\
		rmfmt		\
		rm2ct		\
		efn_drv		\
		efn2_drv	\
		core		\
		junk*
