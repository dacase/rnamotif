#include <stdio.h>
#include <string.h>

#include "rnamot.h"
#include "y.tab.h"

	/* These are the "contexts" that the parser operates under	*/
	/* required, as several items have the same shape, ie descr's	*/
	/* but are descr's in the descr section, but sites in the	*/
	/* site section.						*/

#define	CTX_START	0
#define	CTX_PARM	1
#define	CTX_DESCR	2
#define	CTX_SITE	3
#define	CTX_ERROR	4

static	int	rmc_context = CTX_START;

#define	VALSTKSIZE	20
static	VALUE_T	valstk[ VALSTKSIZE ];
static	int	n_valstk;

static	IDENT_T	idtab[] = {
	{ "wc", T_PAIR, C_VAR },
	{ "gu", T_PAIR, C_VAR },
	{ "tr", T_PAIR, C_VAR },
	{ "qu", T_PAIR, C_VAR },
	{ "overlap", T_INT, C_VAR },
	{ "db", T_STRING, C_VAR }
};
static	int	n_idtab = sizeof( idtab ) / sizeof( IDENT_T );

#define	DESCRSIZE 100
static	STREL_T	descr[ DESCRSIZE ];
static	int	n_descr;
static	STREL_T	*stp;

void	SE_dump();
void	SE_dump_descr();

void	set_context( sym )
int	sym;
{

	switch( sym ){
	case SYM_PARM :
		if( rmc_context < CTX_PARM )
			rmc_context = CTX_PARM;
		else if( rmc_context == CTX_PARM ){
			fprintf( stderr,
		"set_context: FATAL: At most 1 `parm' section permitted.\n" );
			exit( 1 );
		}else if( rmc_context > CTX_PARM ){
			fprintf( stderr,
	"set_context: FATAL: section order is pair, `parm', descr, site.\n" );
			exit( 1 );
		}
		break;
	case SYM_DESCR :
		n_descr = 0;
		if( rmc_context < CTX_DESCR )
			rmc_context = CTX_DESCR;
		else if( rmc_context == CTX_DESCR ){
			fprintf( stderr,
		"set_context: FATAL: At most 1 `descr' section permitted.\n" );
			exit( 1 );
		}else if( rmc_context > CTX_DESCR ){
			fprintf( stderr,
	"set_context: FATAL: section order is pair, parm, `descr', site.\n" );
			exit( 1 );
		}
		break;
	case SYM_SITE :
		if( rmc_context == CTX_DESCR )
			rmc_context = CTX_SITE;
		else if( rmc_context == CTX_SITE ){
			fprintf( stderr,
		"set_context: FATAL: At most 1 `site' section permitted.\n" );
			exit( 1 );
		}else if( rmc_context < CTX_DESCR ){
			fprintf( stderr,
	"set_context: FATAL: section order is pair, parm, descr, `site'.\n" );
			exit( 1 );
		}
		break;
	default :
		fprintf( stderr,
			"set_context: FATAL: unexpected symbol: %d.\n", sym );
		exit( 1 );
		break;
	}
}

void	SE_new( stype )
int	stype;
{

	n_valstk = 0;
	if( rmc_context == CTX_DESCR ){
		if( n_descr == DESCRSIZE ){
			fprintf( stderr,
			"SE_new: FATAL: descr array size(%d) exceeded.\n",
				DESCRSIZE );
			exit( 1 );
		}
		stp = &descr[ n_descr ];
		n_descr++;
		stp->s_type = stype;
		stp->s_index = n_descr - 1;
		stp->s_tag = NULL;
		stp->s_next = NULL;
		stp->s_pairs = NULL;
		stp->s_minlen = UNDEF;
		stp->s_maxlen = UNDEF;
		stp->s_seq = NULL;
		stp->s_mismatch = 0;
		stp->s_mispair = 0;
		stp->s_pairdata = NULL;
		stp->s_sites = NULL;
	}
}

void	SE_addval( expr )
NODE_T	*expr;
{

	dumpexpr( stderr, expr, 0 );
}

void	SE_dump( fp, d_pair, d_parm, d_descr, d_site )
FILE	*fp;
int	d_pair;
int	d_parm;
int	d_descr;
int	d_site;
{
	STREL_T	*stp;
	int	i;

	if( d_descr ){
		fprintf( stderr, "DESCR: %3d structure elements.\n", n_descr );
		for( stp = descr, i = 0; i < n_descr; i++, stp++ )
			SE_dump_descr( fp, stp );
	}
}
void	SE_dump_descr( fp, stp )
FILE	*fp;
STREL_T	*stp;
{

	fprintf( fp, "descr[%3d] = {\n", stp->s_index + 1 );
	fprintf( fp, "\ttype = " );
	switch( stp->s_type ){
	case SYM_SS :
		fprintf( fp, "ss" );
		break;
	case SYM_H5 :
		fprintf( fp, "h5" );
		break;
	case SYM_H3 :
		fprintf( fp, "h3" );
		break;
	case SYM_P5 :
		fprintf( fp, "p5" );
		break;
	case SYM_P3 :
		fprintf( fp, "p3" );
		break;
	case SYM_T1 :
		fprintf( fp, "t1" );
		break;
	case SYM_T2 :
		fprintf( fp, "t2" );
		break;
	case SYM_T3 :
		fprintf( fp, "t3" );
		break;
	case SYM_Q1 :
		fprintf( fp, "q1" );
		break;
	case SYM_Q2 :
		fprintf( fp, "q2" );
		break;
	case SYM_Q3 :
		fprintf( fp, "q3" );
		break;
	case SYM_Q4 :
		fprintf( fp, "q4" );
		break;
	default :
		fprintf( fp, "unknown (%d)", stp->s_type );
		break;
	}
	fprintf( fp, "\n" );

	fprintf( fp, "\tlen  = " );
	if( stp->s_minlen == LASTVAL )
		fprintf( fp, "LASTVAL" );
	else
		fprintf( fp, "%d", stp->s_minlen );
	fprintf( fp, ":" );
	if( stp->s_maxlen == LASTVAL )
		fprintf( fp, "LASTVAL" );
	else
		fprintf( fp, "%d", stp->s_maxlen );
	fprintf( fp, "\n" );

	fprintf( fp, "\ttag  = %s\n",
		stp->s_tag ? stp->s_tag : "(No tag)" );

	fprintf( fp, "\tseq  = %s\n",
		stp->s_seq ? stp->s_seq : "(No seq)" );
	fprintf( fp, "}\n" );
}
