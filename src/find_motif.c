#include <stdio.h>

#include "rnamot.h"

static	int	motiflen();

int	find_motif( n_descr, descr, sites, slen, sbuf )
int	n_descr;
STREL_T descr[];
SITE_T	*sites;
int	slen;
char	sbuf[];
{
	int	mlen;
	
	mlen = motiflen( n_descr, descr, slen );
}

static	int	motiflen( n_descr, descr, slen )
int	n_descr;
STREL_T	descr[];
int	slen;
{

}
