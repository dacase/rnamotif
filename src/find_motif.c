#include <stdio.h>
#include <string.h>
#include "rnamot.h"
#include "y.tab.h"

int	find_motif( n_descr, descr, sites, locus, slen, sbuf )
int	n_descr;
STREL_T descr[];
SITE_T	*sites;
char	locus[];
int	slen;
char	sbuf[];
{
	char	prefix[ 100 ];
	int	tminlen, tmaxlen;
	
	fprintf( stderr, "locus = %s, slen = %d\n", locus, slen );
}
