#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rmdefs.h"
#include "rnamot.h"
#include "y.tab.h"

extern	INCDIR_T	*rm_idlist;

extern	IDENT_T	*rm_global_ids;
extern	int	rm_n_global_ids;

extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
extern	STREL_T	*rm_lctx;
extern	int	rm_lctx_explicit;
extern	STREL_T	*rm_rctx;
extern	int	rm_rctx_explicit;

extern	SITE_T	*rm_sites;

extern	char	rm_bc2b[];

extern	SEARCH_T	**rm_searches;
extern	int	rm_n_searches;

void	RM_dump_gids( FILE *, IDENT_T *, int );
void	RM_dump_id( FILE *, IDENT_T *, int );
void	RM_dump_pairset( FILE *, PAIRSET_T * );
void	RM_dump_pair( FILE *, PAIR_T * );
void	RM_dump_pairmat( FILE *, PAIRSET_T * );
void	RM_dump_descr( FILE *, STREL_T * );
void	RM_dump_pos( FILE *, int, POS_T * );
void	RM_dump_sites( FILE * );
void	RM_strel_name( STREL_T *, char [] );

static	char	*attr2str( signed char [] );
static	void	print_hierarchy( FILE *, int, char [], int, STREL_T [] );
static	void	print_1_element( FILE *, char [], STREL_T * );
static	void	mk_prefix( STREL_T *, char [], char [] );
static	void	print_searches( FILE *, int, SEARCH_T *[] );

void	RM_dump( FILE *fp,
	int d_parms, int d_descr, int d_sites, int d_hierarchy )
{
	int	i;
	STREL_T	*stp;
	char	prefix[ 100 ];
	INCDIR_T	*id1;

	if( rm_idlist != NULL ){
		for( i = 0, id1 = rm_idlist; id1; id1 = id1->i_next )
			i++;
		fprintf( fp, "INCLUDES: %3d dirs.\n", i );
		for( id1 = rm_idlist; id1; id1 = id1->i_next )
			fprintf( fp, "\t%s\n", id1->i_name );
	}

	if( d_parms ){
		fprintf( fp, "PARMS: %3d global symbols.\n", rm_n_global_ids );
		RM_dump_gids( fp, rm_global_ids, d_parms );
	}

	if( d_descr ){
		fprintf( fp, "DESCR: %3d structure elements.\n", rm_n_descr );
		for( stp = rm_descr, i = 0; i < rm_n_descr; i++, stp++ )
			RM_dump_descr( fp, stp );
		if( rm_lctx != NULL ){
			fprintf( fp, "DESCR: left context (%s).\n",
				rm_lctx_explicit ? "Explicit" : "Implicit" );
			RM_dump_descr( fp, rm_lctx );
		}
		if( rm_rctx != NULL ){
			fprintf( fp, "DESCR: right context (%s).\n",
				rm_rctx_explicit ? "Explicit" : "Implicit" );
			RM_dump_descr( fp, rm_rctx );
		}
	}

	if( d_sites ){
		RM_dump_sites( fp );
	}

	if( d_hierarchy ){
		fprintf( fp,
	"desc# minl  maxl  mngl  mxgl  mnil  mxil start  stop  descr\n" );
		strcpy( prefix, "+" );
		print_hierarchy( fp, 0, prefix, 0, rm_descr );
		print_searches( fp, rm_n_searches, rm_searches );
	}
}

void	RM_dump_gids( FILE *fp, IDENT_T *ip, int fmt )
{

	if( ip != NULL ){
		RM_dump_gids( fp, ip->i_left, fmt );
		RM_dump_id( fp, ip, fmt );
		RM_dump_gids( fp, ip->i_right, fmt );
	}
}

void	RM_dump_id( FILE *fp, IDENT_T *ip, int fmt )
{
	PAIRSET_T	*ps;

	if( fmt == 1 )
		fprintf( fp, "%s (%s) = {\n", ip->i_name,
			ip->i_reinit ? "RW" : "RO" );
	else
		fprintf( fp, "%-16s (%s) = ", ip->i_name,
			ip->i_reinit ? "RW" : "RO" );

	if( fmt == 1 ){
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
		case T_PAIRSET :
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

		fprintf( fp, "\treinit= %d\n", ip->i_reinit);
		fprintf( fp, "\tvalue = " );
	}

	switch( ip->i_val.v_type ){
	case T_UNDEF :
		fprintf( fp, "UNDEF\n" );
		break;
	case T_INT :
		fprintf( fp, "%d\n", ip->i_val.v_value.v_ival );
		break;
	case T_FLOAT :
		fprintf( fp, "%lg\n", ip->i_val.v_value.v_dval );
		break;
	case T_STRING :
		fprintf( fp, "'%s'\n", ip->i_val.v_value.v_pval ?
					ip->i_val.v_value.v_pval : "NULL" );
		break;
	case T_PAIRSET :
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
		
	if( fmt ==1 )
		fprintf( fp, "}\n" );
}

void	RM_dump_pairset( FILE *fp, PAIRSET_T *ps )
{
	PAIR_T	*pp;
	int	i;

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

void	RM_dump_pair( FILE *fp, PAIR_T *pp )
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

void	RM_dump_pairmat( FILE *fp, PAIRSET_T *ps )
{
	PAIR_T	*pp;
	int	nb, np;
	int	i1, i2, i3, i4;
	int	b1, b2, b3, b4;
	BP_MAT_T	*bpmatp;
	BT_MAT_T	*btmatp;
	BQ_MAT_T	*bqmatp;

	pp = ps->ps_pairs;
	nb = pp->p_n_bases;
	fprintf( fp, "{ " );
	if( nb == 2 ){
		bpmatp = ps->ps_mat[ 0 ];
		for( np = 0, i1 = 0; i1 < N_BCODES; i1++ ){
		    for( i2 = 0; i2 < N_BCODES; i2++ ){
			if( (*bpmatp)[i1][i2] ){
			    b1 = rm_bc2b[ i1 ];
			    b2 = rm_bc2b[ i2 ];
			    if( np > 0 )
				fprintf( fp, ", " );
			    fprintf( fp, "%c:%c", b1, b2 );
			    np++;
			}
		    }
		}
	}else if( nb == 3 ){
		btmatp = ps->ps_mat[ 1 ];
		for( np = 0, i1 = 0; i1 < N_BCODES; i1++ ){
		    for( i2 = 0; i2 < N_BCODES; i2++ ){
			for( i3 = 0; i3 < N_BCODES; i3++ ){
			    if( (*btmatp)[i1][i2][i3] ){
				b1 = rm_bc2b[ i1 ];
				b2 = rm_bc2b[ i2 ];
				b3 = rm_bc2b[ i3 ];
				if( np > 0 )
				    fprintf( fp, ", " );
				fprintf( fp, "%c:%c:%c", b1, b2, b3 );
				np++;
			    }
			}
		    }
		}
	}else if( nb == 4 ){
		bqmatp = ps->ps_mat[ 1 ];
		for( np = 0, i1 = 0; i1 < N_BCODES; i1++ ){
		    for( i2 = 0; i2 < N_BCODES; i2++ ){
			for( i3 = 0; i3 < N_BCODES; i3++ ){
			    for( i4 = 0; i4 < N_BCODES; i4++ ){
				if( (*bqmatp)[i1][i2][i3][i4] ){
				    b1 = rm_bc2b[ i1 ];
				    b2 = rm_bc2b[ i2 ];
				    b3 = rm_bc2b[ i3 ];
				    b4 = rm_bc2b[ i4 ];
				    if( np > 0 )
					fprintf( fp, ", " );
				    fprintf( fp, "%c:%c:%c:%c", b1, b2, b3, b4);
				    np++;
				}
			    }
			}
		    }
		}
	}
	fprintf( fp, " }\n" );
}

void	RM_dump_descr( FILE *fp, STREL_T *stp )
{
	int	i;
	STREL_T	*stp1;

	fprintf( fp, "descr[%3d] = {\n", stp->s_index );
	fprintf( fp, "\ttype     = " );
	switch( stp->s_type ){
	case SYM_CTX :
		fprintf( fp, "ctx" );
		break;
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

	fprintf( fp, "\tattr     = %s", attr2str( stp->s_attr ) );
	fprintf( fp, "\n" );

	fprintf( fp, "\tlineno   = %d\n", stp->s_lineno );

	fprintf( fp, "\tsearchno = " );
	if( stp->s_searchno == UNDEF )
		fprintf( fp, "UNDEF" );
	else
		fprintf( fp, "%d", stp->s_searchno );
	fprintf( fp, "\n" );

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
	fprintf( fp, "\tmatchfrac= " );
	if( stp->s_matchfrac == UNDEF )
		fprintf( fp, "UNDEF\n" );
	else
		fprintf( fp, "%5.3lf\n", stp->s_matchfrac );

	fprintf( fp, "\tmispair  = " );
	if( stp->s_mispair == UNDEF )
		fprintf( fp, "UNDEF\n" );
	else
		fprintf( fp, "%d\n", stp->s_mispair );
	fprintf( fp, "\tpairfrac = " );
	if( stp->s_pairfrac == UNDEF )
		fprintf( fp, "UNDEF\n" );
	else
		fprintf( fp, "%5.3lf\n", stp->s_pairfrac );

	fprintf( fp, "\tpair     = " );
	if( stp->s_pairset != NULL )
		RM_dump_pairset( fp, stp->s_pairset );
	else
		fprintf( fp, "(None)" );
	fprintf( fp, "\n" );

	fprintf( fp, "}\n" );
}

void	RM_dump_pos( FILE *fp, int p, POS_T *posp )
{
	STREL_T	*stp;

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
	stp = posp->p_descr;
/*
	fprintf( fp, "\t\tdindex   = %d\n", posp->p_descr->s_index );
*/
	fprintf( fp, "\t\tdindex   = %d\n", stp->s_index );
	fprintf( fp, "\t\tl2r      = %s\n",
		posp->p_addr.a_l2r ? "TRUE" : "FALSE" );
	fprintf( fp, "\t\toffset   = %d\n", posp->p_addr.a_offset );
	fprintf( fp, "\t}\n" );
}

void	RM_dump_sites( FILE *fp )
{
	int	i, j, n_sites;
	SITE_T	*sp;
	POS_T	*posp;

	for( n_sites = 0, sp = rm_sites; sp; sp = sp->s_next )
		n_sites++;
	fprintf( fp, "SITES: %4d sites.\n", n_sites );
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

void	RM_strel_name( STREL_T *stp, char name[] )
{

	switch( stp->s_type ){
	case SYM_CTX :
		strcpy( name, "ctx" );
		break;

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

static	char	*attr2str( signed char attr[] )
{
	int	acnt = 0;
	static	char	astr[ 256 ];

	strcpy( astr, "{ " );
	if( attr[ SA_PROPER ] ){
		if( acnt > 0 )
			strcat( astr, "," );
		strcat( astr, "P" );
		acnt++;
	}
	if( attr[ SA_ENDS ] ){
		if( attr[ SA_ENDS ] & SA_5PAIRED ){
			if( acnt > 0 )
				strcat( astr, "," );
			strcat( astr, "p5" );
			acnt++;
		}
		if( attr[ SA_ENDS ] & SA_3PAIRED ){
			if( acnt > 0 )
				strcat( astr, "," );
			strcat( astr, "p3" );
			acnt++;
		}
	}
	if( attr[ SA_STRICT ] ){
		if( attr[ SA_STRICT ] & SA_5STRICT ){
			if( acnt > 0 )
				strcat( astr, "," );
			strcat( astr, "s5" );
			acnt++;
		}
		if( attr[ SA_STRICT ] & SA_3STRICT ){
			if( acnt > 0 )
				strcat( astr, "," );
			strcat( astr, "s3" );
			acnt++;
		}
	}
	return( strcat( astr, " }" ) );
}

static	void	print_hierarchy( FILE *fp, int lev, char prefix[],
	int fd, STREL_T descr[] )
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
			if( stp1->s_index + 1 < stp2->s_index )
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

static	void	print_1_element( FILE *fp, char prefix[], STREL_T *stp )
{
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

	RM_strel_name( stp, name );
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

static	void	mk_prefix( STREL_T *stp, char prefix[], char prefix1[] )
{
	char	*pp;
	int	plen;
	int	inner, next, first;

	strcpy( prefix1, prefix );
	plen = strlen( prefix1 );
	pp = &prefix1[ plen - 1 ];

	inner = stp->s_inner != NULL;
	if( inner )
		first = stp->s_scope == 0;
	else
		first = FALSE;
	next = stp->s_next != NULL;
	if( next )
		*pp = '|';
	else if( *pp != '|' )
		*pp = ' ';
	if( first )
		strcat( prefix1, "  |" );
	strcat( prefix1, "  +" );
}

static	void	print_searches( FILE *fp, int n_searches, SEARCH_T *searches[] )
{
	int	s;
	STREL_T	*stp, *stp1;
	char	name[ 20 ];

	fprintf( fp, "total search depth: %3d\n", n_searches );
	fprintf( fp, "srch# desc# type  forward  backup\n" );
	for( s = 0; s < n_searches; s++ ){
		stp = searches[ s ]->s_descr;
		RM_strel_name( stp, name );
		fprintf( fp, "%4d %5d %5s", s, stp->s_index, name );
		stp1 = searches[ s ]->s_forward;
		if( stp1 != NULL )
			fprintf( fp, " %8d", stp1->s_index );
		else
			fprintf( fp, "   (None)" );
		stp1 = searches[ s ]->s_backup;
		if( stp1 != NULL )
			fprintf( fp, " %7d", stp1->s_index );
		else
			fprintf( fp, "  (None)" );
		fprintf( fp, "\n" );
	}
}
