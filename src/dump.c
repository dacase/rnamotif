#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rnamot.h"
#include "y.tab.h"

extern	IDENT_T	rm_global_ids[];
extern	int	rm_n_global_ids;

extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;

extern	SITE_T	*rm_sites;

void	RM_dump_id();
void	RM_dump_pairset();
void	RM_dump_pair();
void	RM_dump_descr();
void	RM_dump_sites();

static	void	print_hierarchy();
static	void	print_1_element();
static	void	mk_prefix();
static	void	mk_strel_name();

void	RM_dump( fp, d_parms, d_descr, d_sites, d_hierarchy )
FILE	*fp;
int	d_parms;
int	d_descr;
int	d_sites;
int	d_hierarchy;
{
	int	i;
	IDENT_T	*ip;
	STREL_T	*stp;
	char	prefix[ 100 ];

	if( d_parms ){
		fprintf( fp, "PARMS: %3d global symbols.\n", rm_n_global_ids );
		for( ip = rm_global_ids, i = 0; i < rm_n_global_ids; i++, ip++ )
			RM_dump_id( fp, ip );
	}

	if( d_descr ){
		fprintf( fp, "DESCR: %3d structure elements.\n", rm_n_descr );
		for( stp = rm_descr, i = 0; i < rm_n_descr; i++, stp++ )
			RM_dump_descr( fp, stp );
	}

	if( d_sites ){
		RM_dump_sites( fp );
	}

	if( d_hierarchy ){
		fprintf( fp,
	"descr minl  maxl  mngl  mxgl  mnil  mxil start  stop  descr\n" );
		strcpy( prefix, "+" );
		print_hierarchy( fp, 0, prefix, 0, rm_descr );
	}
}

void	RM_dump_id( fp, ip )
FILE	*fp;
IDENT_T	*ip;
{
	PAIR_T	*pp;
	PAIRSET_T	*ps;
	int	i, b;

	fprintf( fp, "%s = {\n", ip->i_name );

	fprintf( fp, "\ttype  = " );
	switch( ip->i_type ){
	case T_UNDEF :
		fprintf( fp, "UNDEF\n" );
		break;
	case T_INT :
		fprintf( fp, "INT\n" );
		break;
	case T_FLOAT :
		fprintf( fp, "FLOAT\n" );
		break;
	case T_STRING :
		fprintf( fp, "STRING\n" );
		break;
	case T_PAIR :
		fprintf( fp, "PAIR\n" );
		break;
	case T_IDENT :
		fprintf( fp, "IDENT\n" );
		break;
	default :
		fprintf( fp, "-- BAD type %d\n", ip->i_type );
		break;
	}

	fprintf( fp, "\tclass = " );
	switch( ip->i_class ){
	case C_UNDEF :
		fprintf( fp, "UNDEF\n" );
		break;
	case C_LIT :
		fprintf( fp, "LIT\n" );
		break;
	case C_VAR :
		fprintf( fp, "VAR\n" );
		break;
	case C_EXPR :
		fprintf( fp, "EXPR\n" );
		break;
	default :
		fprintf( fp, "-- BAD class %d\n", ip->i_class );
		break;
	}
		
	fprintf( fp, "\tscope = " );
	switch( ip->i_scope ){
	case S_UNDEF :
		fprintf( fp, "UNDEF\n" );
		break;
	case S_GLOBAL :
		fprintf( fp, "GLOBAL\n" );
		break;
	case S_STREL :
		fprintf( fp, "STREL\n" );
		break;
	case S_SITE :
		fprintf( fp, "SITE\n" );
		break;
	default :
		fprintf( fp, "-- BAD scope %d\n", ip->i_scope );
		break;
	}

	fprintf( fp, "\tvalue = " );
	switch( ip->i_val.v_type ){
	case T_UNDEF :
		fprintf( fp, "UNDEF\n" );
		break;
	case T_INT :
		fprintf( fp, "%d\n", ip->i_val.v_value.v_ival );
		break;
	case T_FLOAT :
		fprintf( fp, "%f\n", ip->i_val.v_value.v_fval );
		break;
	case T_STRING :
		fprintf( fp, "%s\n", ip->i_val.v_value.v_pval ?
					ip->i_val.v_value.v_pval : "NULL" );
		break;
	case T_PAIR :
		ps = ip->i_val.v_value.v_pval;
		RM_dump_pairset( fp, ps );
		fprintf( fp, "\n" );
		break;
	case T_IDENT :
		fprintf( fp, "IDENT?\n" );
		break;
	default :
		fprintf( fp, "-- BAD type %d\n", ip->i_val.v_type );
		break;
	}
		
	fprintf( fp, "}\n" );
}

void	RM_dump_pairset( fp, ps )
FILE	*fp;
PAIRSET_T	*ps;
{
	PAIR_T	*pp;
	int	i, b;

	fprintf( fp, "{ " );
	if( ps != NULL ){
		for( pp = ps->ps_pairs, i = 0; i < ps->ps_n_pairs; i++, pp++ ){
			RM_dump_pair( fp, pp );
			if( i < ps->ps_n_pairs - 1 )
				fprintf( fp, ", " );
		}
	}
	fprintf( fp, " }" );
}

void	RM_dump_pair( fp, pp )
FILE	*fp;
PAIR_T	*pp;
{
	int	b;

	fprintf( fp, "\"" );
	for( b = 0; b < pp->p_n_bases; b++ ){
		fprintf( stderr, "%c", pp->p_bases[ b ] );
		if( b < pp->p_n_bases - 1 )
			fprintf( stderr, ":" );
	}
	fprintf( fp, "\"" );
}

void	RM_dump_descr( fp, stp )
FILE	*fp;
STREL_T	*stp;
{
	int	i;
	STREL_T	*stp1;

	fprintf( fp, "descr[%3d] = {\n", stp->s_index );
	fprintf( fp, "\ttype     = " );
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

	fprintf( fp, "\tproper   = %s", stp->s_proper ? "yes" : "no" );
	fprintf( fp, "\n" );

	fprintf( fp, "\tlineno   = %d\n", stp->s_lineno );

	fprintf( fp, "\ttag      = '%s'\n",
		stp->s_tag ? stp->s_tag : "(No tag)" );

	fprintf( fp, "\tnext     = " );
	if( stp->s_next != NULL ) 
		fprintf( fp, "%d\n", stp->s_next->s_index );
	else
		fprintf( fp, "(None)\n" );

	fprintf( fp, "\tprev     = " );
	if( stp->s_prev != NULL ) 
		fprintf( fp, "%d\n", stp->s_prev->s_index );
	else
		fprintf( fp, "(None)\n" );

	fprintf( fp, "\tinner    = " );
	if( stp->s_inner ){
		stp1 = stp->s_inner;
		fprintf( fp, "%d", stp1->s_index );
	}else
		fprintf( fp, "(None)" );
	fprintf( fp, "\n" );

	fprintf( fp, "\touter    = " );
	if( stp->s_outer ){
		stp1 = stp->s_outer;
		fprintf( fp, "%d", stp1->s_index );
	}else
		fprintf( fp, "(None)" );
	fprintf( fp, "\n" );

	fprintf( fp, "\tmates    = [ " );
	for( i = 0; i < stp->s_n_mates; i++ ){
		stp1 = stp->s_mates[ i ];
		fprintf( fp, "%d", stp1->s_index );
		if( i < stp->s_n_mates - 1 )
			fprintf( fp, ", " );
	}
	fprintf( fp, " ]\n" );
	fprintf( fp, "\tscopes   = [ " );
	for( i = 0; i < stp->s_n_scopes; i++ ){
		stp1 = stp->s_scopes[ i ];
		fprintf( fp, "%d", stp1->s_index );
		if( i < stp->s_n_scopes - 1 )
			fprintf( fp, ", " );
	}
	fprintf( fp, " ]\n" );
	fprintf( fp, "\tscope    = %d\n", stp->s_scope );

	fprintf( fp, "\tlen      = " );
	if( stp->s_minlen == UNDEF )
		fprintf( fp, "UNDEF" );
	else if( stp->s_minlen == UNBOUNDED )
		fprintf( fp, "UNBOUNDED" );
	else
		fprintf( fp, "%d", stp->s_minlen );
	fprintf( fp, ":" );
	if( stp->s_maxlen == UNDEF )
		fprintf( fp, "UNDEF" );
	else if( stp->s_maxlen == UNBOUNDED )
		fprintf( fp, "UNBOUNDED" );
	else
		fprintf( fp, "%d", stp->s_maxlen );
	fprintf( fp, "\n" );

	fprintf( fp, "\tglen     = " );
	if( stp->s_minglen == UNDEF )
		fprintf( fp, "UNDEF" );
	else if( stp->s_minglen == UNBOUNDED )
		fprintf( fp, "UNBOUNDED" );
	else
		fprintf( fp, "%d", stp->s_minglen );
	fprintf( fp, ":" );
	if( stp->s_maxglen == UNDEF )
		fprintf( fp, "UNDEF" );
	else if( stp->s_maxglen == UNBOUNDED )
		fprintf( fp, "UNBOUNDED" );
	else
		fprintf( fp, "%d", stp->s_maxglen );
	fprintf( fp, "\n" );

	fprintf( fp, "\tilen     = " );
	if( stp->s_minilen == UNDEF )
		fprintf( fp, "UNDEF" );
	else if( stp->s_minilen == UNBOUNDED )
		fprintf( fp, "UNBOUNDED" );
	else
		fprintf( fp, "%d", stp->s_minilen );
	fprintf( fp, ":" );
	if( stp->s_maxilen == UNDEF )
		fprintf( fp, "UNDEF" );
	else if( stp->s_maxilen == UNBOUNDED )
		fprintf( fp, "UNBOUNDED" );
	else
		fprintf( fp, "%d", stp->s_maxilen );
	fprintf( fp, "\n" );

	fprintf( fp, "\tstart    = " );
	if( stp->s_start.a_offset == UNDEF )
		fprintf( fp, "UNDEF" );
	else if( stp->s_start.a_l2r )
		fprintf( fp, "%d", stp->s_start.a_offset );
	else
		fprintf( fp, "$-%d", stp->s_start.a_offset );
	fprintf( fp, "\n" );

	fprintf( fp, "\tstop     = " );
	if( stp->s_stop.a_offset == UNDEF )
		fprintf( fp, "UNDEF" );
	else if( stp->s_stop.a_l2r )
		fprintf( fp, "%d", stp->s_stop.a_offset );
	else
		fprintf( fp, "$-%d", stp->s_stop.a_offset );
	fprintf( fp, "\n" );

	fprintf( fp, "\tseq      = '%s'\n",
		stp->s_seq ? stp->s_seq : "(No seq)" );

	fprintf( fp, "\tmismatch = " );
	if( stp->s_mismatch == UNDEF )
		fprintf( fp, "UNDEF\n" );
	else
		fprintf( fp, "%d\n", stp->s_mismatch );

	fprintf( fp, "\tmispair  = " );
	if( stp->s_mispair == UNDEF )
		fprintf( fp, "UNDEF\n" );
	else
		fprintf( fp, "%d\n", stp->s_mispair );

	fprintf( fp, "\tpair     = " );
	if( stp->s_pairset != NULL )
		RM_dump_pairset( fp, stp->s_pairset );
	else
		fprintf( fp, "(None)" );
	fprintf( fp, "\n" );

	fprintf( fp, "}\n" );
}

RM_dump_pos( fp, p, posp )
FILE	*fp;
int	p;
POS_T	*posp;
{

	fprintf( fp, "\tpos[%2d] = {\n", p + 1 );
	fprintf( fp, "\t\ttype     = " );
	switch( posp->p_type ){
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
		fprintf( fp, "unknown (%d)", posp->p_type );
		break;
	}
	fprintf( fp, "\n" );

	fprintf( fp, "\t\tlineno   = %d\n", posp->p_lineno );
	fprintf( fp, "\t\ttag      = '%s'\n",
		posp->p_tag ? posp->p_tag : "(No tag)" );
	fprintf( fp, "\t\tdindex   = %d\n", posp->p_dindex );
	fprintf( fp, "\t\tl2r      = %s\n",
		posp->p_addr.a_l2r ? "TRUE" : "FALSE" );
	fprintf( fp, "\t\toffset   = %d\n", posp->p_addr.a_offset );
	fprintf( fp, "\t}\n" );
}

void	RM_dump_sites( fp )
FILE	*fp;
{
	int	i, j, n_sites;
	SITE_T	*sp;
	POS_T	*posp;
	PAIRSET_T	*ps;

	for( n_sites = 0, sp = rm_sites; sp; sp = sp->s_next )
		n_sites++;
	fprintf( fp, "SITES: %3d sites.\n", n_sites );
	for( i = 0, sp = rm_sites; sp; sp = sp->s_next, i++ ){
		fprintf( fp, "site[%2d] = {\n", i + 1 );
		fprintf( fp, "\tnpos    = %3d\n", sp->s_n_pos );
		for( posp = sp->s_pos, j = 0; j < sp->s_n_pos; j++, posp++ )
			RM_dump_pos( fp, j, posp );	
		fprintf( fp, "\tpairset = " );
		RM_dump_pairset( fp, sp->s_pairset );
		fprintf( fp, "\n" );
		fprintf( fp, "}\n" );
	}
}

static	void	print_hierarchy( fp, lev, prefix, fd, descr )
FILE	*fp;
int	lev;
char	prefix[];
int	fd;
STREL_T	descr[];
{
	int	d, nd, s;
	STREL_T	*stp, *stp1, *stp2;
	char	prefix1[ 100 ], prefix2[ 100 ];;

	for( d = fd; ; d = nd ){
		stp = &descr[ d ];
		print_1_element( fp, prefix, stp );
		mk_prefix( stp, prefix, prefix1 ); 
		for( s = 1; s < stp->s_n_scopes; s++ ){
			stp1 = stp->s_scopes[ s - 1 ];
			stp2 = stp->s_scopes[ s ];
			print_hierarchy( fp, lev+2, prefix1, 
				stp1->s_index+1, descr );
			mk_prefix( stp2, prefix, prefix2 ); 
			print_1_element( fp, prefix2, stp2 );
		}
		stp1 = stp->s_next;
		if( stp1 == NULL )
			return;
		else
			nd = stp1->s_index;
	} 
}

static	void	print_1_element( fp, prefix, stp )
FILE	*fp;
char	prefix[];
STREL_T	*stp;
{
	int	start, stop, l2r;
	char	name[ 20 ], tstr[ 20 ];
	char	*bp, buf[ 200 ];

	bp = buf;
	sprintf( bp, "%4d", stp->s_index );
	bp += strlen( bp );

	sprintf( bp, " %5d", stp->s_minlen );
	bp += strlen( bp );

	if( stp->s_maxlen == UNBOUNDED )
		sprintf( bp, " UNBND" );
	else
		sprintf( bp, " %5d", stp->s_maxlen );
	bp += strlen( bp );

	if( stp->s_minglen == UNDEF )
		sprintf( bp, " UNDEF" );
	else
		sprintf( bp, " %5d", stp->s_minglen );
	bp += strlen( bp );

	if( stp->s_maxglen == UNBOUNDED )
		sprintf( bp, " UNBND" );
	else if( stp->s_maxglen == UNDEF )
		sprintf( bp, " UNDEF" );
	else
		sprintf( bp, " %5d", stp->s_maxglen );
	bp += strlen( bp );

	if( stp->s_minilen == UNDEF )
		sprintf( bp, " UNDEF" );
	else
		sprintf( bp, " %5d", stp->s_minilen );
	bp += strlen( bp );

	if( stp->s_maxilen == UNBOUNDED )
		sprintf( bp, " UNBND" );
	else if( stp->s_maxilen == UNDEF )
		sprintf( bp, " UNDEF" );
	else
		sprintf( bp, " %5d", stp->s_maxilen );
	bp += strlen( bp );

	sprintf( tstr, "%s%d", !stp->s_start.a_l2r ? "$-" : "",
		stp->s_start.a_offset );
	sprintf( bp, " %5s", tstr );
	bp += strlen( bp );

	sprintf( tstr, "%s%d", !stp->s_stop.a_l2r ? "$-" : "",
		stp->s_stop.a_offset );
	sprintf( bp, " %5s", tstr );
	bp += strlen( bp );

	mk_strel_name( stp, name );
	sprintf( bp, "  %s%s", prefix, name );
	bp += strlen( bp );

	if( stp->s_scope == 0 ){
		sprintf( bp, "+--+" );
		bp += strlen( bp );
	}
	sprintf( bp, "\n" );
	bp += strlen( bp );

	fputs( buf, fp );
}

static	void	mk_prefix( stp, prefix, prefix1 )
STREL_T	*stp;
char	prefix[];
char	prefix1[];
{
	char	*pp, *pp1;
	int	plen;
	STREL_T	*stp0;
	int	inner, next, first;

	strcpy( prefix1, prefix );
	plen = strlen( prefix1 );
	pp = &prefix1[ plen - 1 ];

	inner = stp->s_inner != NULL;
	if( inner )
		first = stp->s_scope == 0;
	else
		first = 0;
	next = stp->s_next != NULL;
	if( next )
		*pp = '|';
	else if( *pp != '|' )
		*pp = ' ';
	if( first )
		strcat( prefix1, "  |" );
	strcat( prefix1, "  +" );
}

static	void	mk_strel_name( stp, name )
STREL_T	*stp;
char	name[];
{

	switch( stp->s_type ){
	case SYM_SS :
		strcpy( name, "ss" );
		break;

	case SYM_H5 :
		strcpy( name, "h5" );
		break;
	case SYM_H3 :
		strcpy( name, "h3" );
		break;

	case SYM_P5 :
		strcpy( name, "p5" );
		break;
	case SYM_P3 :
		strcpy( name, "p3" );
		break;

	case SYM_T1 :
		strcpy( name, "t1" );
		break;
	case SYM_T2 :
		strcpy( name, "t2" );
		break;
	case SYM_T3 :
		strcpy( name, "t3" );
		break;

	case SYM_Q1 :
		strcpy( name, "q1" );
		break;
	case SYM_Q2 :
		strcpy( name, "q2" );
		break;
	case SYM_Q3 :
		strcpy( name, "q3" );
		break;
	case SYM_Q4 :
		strcpy( name, "q4" );
		break;
	}
}
