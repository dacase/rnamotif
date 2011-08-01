#
#		make doc		! make an ascii version
#		make x.doc		! single section .doc file
#		make ps			! all documentation in postscript form., with 
#                         table of contents and index
#       make x.ps		! single section .ps file
#

SHELL = /bin/sh
.SUFFIXES: .me .doc .ps .html .pdf
#
#  set site-specific ROFF stuff
#
NROFF  = troff -Tascii
TROFF  = troff -U
REFER  = refer
PIC    = pic
TBL    = tbl
EQN    = eqn
NEQN   = eqn -Tascii
GROTTY = grotty -u -b -f -o
GROPS  = grops

SRC = 	intro.me tutorial.me algorithms.me summary.me

DOC = 	intro.doc tutorial.doc algorithms.doc summary.doc

pdf:	rnamotif.pdf 
		/bin/rm -f rnamotif.me

doc:		$(DOC)

ps:		rnamotif.ps 
		/bin/rm -f rnamotif.me

rnamotif.me:	$(SRC)
		cat $(SRC) > rnamotif.me
#		./mk_index rnamotif.me
#		cat rnamotif.me.index >> rnamotif.me
		cat toc.me >> rnamotif.me
#		/bin/rm rnamotif.me.index

rnamotif.html:    $(SRC)
		cat $(SRC) > rnamotif.me
		troff2html -split 2 -toc bottom -eqn ps -tbl ps rnamotif.me
		perl -pi -e "s/rnamotif Users' Manual/\<H1\>rnamotif Users' Manual\<\/H1\>/;" rnamotif.html
#		insertsearch
		perl -pi -e 's/^Table of Contents/rnamotif Documentation/;s?^</ul>?</ul>\n<p>?' $@
		-mkdir www
		/bin/mv rnamotif*html www
		/bin/mv rnamotif*gif www

.me.html:    $<
		troff2html -split 2 -toc bottom -eqn ps -tbl nroff $<
		perl -pi -e 's/<body>/<body bgcolor="#A0F0F0" link="#EE0000" alink="#0000CC" vlink="#0000CC">/;s/_\[/</g;s/_\]/>/g;'  *.html
		perl -pi -e 's/^Table of Contents/rnamotif Documentation/;s?^</ul>?</ul>\n<p>?' $@

.me.doc:	$<
		cat nroff.dot rnamotif.mac $< | $(REFER) | $(PIC) | $(TBL) | $(NEQN) | \
			$(NROFF) -me 2> $@.errs | $(GROTTY) > $@ 
		/bin/rm $@.errs

.me.ps:		$<
		cat rnamotif.mac $< | $(REFER) | $(PIC) | $(TBL) | $(EQN) -Tps | \
			$(TROFF) -me -Tps 2> $@.errs | $(GROPS) | fix_bookmarks > $@ 
		/bin/rm $@.errs

.me.pdf:		$<
		cat rnamotif.mac $< | $(REFER) | $(PIC) | $(TBL) | $(EQN) -Tps | \
			$(TROFF) -me -Tps 2> $@.errs | $(GROPS) | fix_bookmarks \
			| ps2pdf - - > $@ 
		/bin/rm $@.errs

clean::
	-/bin/rm www/rnamotif-* www/rnamotif.html
